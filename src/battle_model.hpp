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
	BATTLE_ATTACKER_WON = 1, // the defender's organisation ran out and it withdrew
	BATTLE_DEFENDER_WON = 2, // the attacker's did, or the attack ran out of time
};

// The ground a battle is fought over. Only the attacker is penalised by it.
struct Terrain {
	const char *name = "Plains";
	float attack_modifier = 1.0f;
};

// Terrain is derived from the province coordinates, so the same province always
// fights the same way.
Terrain terrain_at(int p_column, int p_row);

// What one side threw and landed during the last hour of fighting.
struct BattleSideState {
	int unit = -1;
	float attacks = 0.0f;         // attacks thrown, after hardness, terrain and armour
	float target_defences = 0.0f; // defences the target had to spend on them
	float blocked = 0.0f;         // attacks a defence absorbed: one hit in ten
	float unblocked = 0.0f;       // attacks past the defences: four hits in ten
	float hits = 0.0f;            // how many of them landed
	int org_die = 0;              // die rolled per hit for organisation damage
	int str_die = 0;              // die rolled per hit for strength damage
	int org_rolled = 0;           // the organisation dice total
	int strength_rolled = 0;      // the strength dice total
	bool pierced = true;          // whether these attacks beat the target's armour
	float org_damage = 0.0f;      // organisation taken off the target
	float strength_damage = 0.0f; // men and equipment taken off the target
};

// One battle between two dudes that hold bordering provinces. It ticks an hour
// at a time; whichever side runs out of organisation first loses the province.
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
	void resolve(BattleSide p_side, int p_target, const UnitModel &p_units);

	bool active_ = false;
	int attacker_ = -1;
	int defender_ = -1;
	int hours_ = 0;
	Terrain terrain_;
	BattleSideState sides_[2];
};

} // namespace infantry

#endif // INFANTRY_BATTLE_MODEL_HPP
