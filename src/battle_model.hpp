#ifndef INFANTRY_BATTLE_MODEL_HPP
#define INFANTRY_BATTLE_MODEL_HPP

#include "combat_defines.hpp"
#include "division_stats.hpp"

namespace infantry {

class UnitModel;

enum BattleSide {
	ATTACKER = 0,
	DEFENDER = 1,
};

enum BattleOutcome {
	BATTLE_ONGOING = 0,
	BATTLE_ATTACKER_WON = 1, // the defender's organisation broke and it withdrew
	BATTLE_DEFENDER_WON = 2, // the attacker broke, or ran out of time
};

// The ground a battle is fought over. Only the attacker is penalised by it.
struct Terrain {
	const char *name = "Plains";
	float attack_modifier = 1.0f;
};

// Terrain is derived from the province coordinates, so the same province always
// fights the same way.
Terrain terrain_at(int p_column, int p_row);

// What one side rolled and landed during the last hour of fighting.
struct BattleSideState {
	int unit = -1;
	int dice = 0;                  // 1..COMBAT_DICE damage roll
	float hits = 0.0f;             // attack value that landed
	float hits_defended = 0.0f;    // part of it the target parried
	float hits_undefended = 0.0f;  // part of it the target could not parry
	float org_damage = 0.0f;       // organisation taken off the target
	float strength_damage = 0.0f;  // men and equipment taken off the target
};

// One battle between two dudes that hold bordering provinces. It ticks an hour
// at a time; whichever side's organisation reaches the retreat threshold first
// loses the province.
class Battle {
public:
	Battle();

	void start(int p_attacker, int p_defender, const Terrain &p_terrain);
	void stop();

	bool is_active() const { return active_; }
	int get_attacker() const { return attacker_; }
	int get_defender() const { return defender_; }
	int get_hours() const { return hours_; }
	const Terrain &get_terrain() const { return terrain_; }
	const BattleSideState &get_side(int p_side) const { return sides_[p_side]; }

	// Runs one hour of combat and reports what happened in it.
	BattleOutcome tick(UnitModel &p_units);

private:
	void roll(BattleSide p_side, int p_target, const UnitModel &p_units);

	bool active_ = false;
	int attacker_ = -1;
	int defender_ = -1;
	int hours_ = 0;
	Terrain terrain_;
	BattleSideState sides_[2];
};

} // namespace infantry

#endif // INFANTRY_BATTLE_MODEL_HPP
