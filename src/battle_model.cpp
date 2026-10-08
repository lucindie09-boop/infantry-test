#include "battle_model.hpp"

#include <cstdlib>

#include "unit_model.hpp"

namespace infantry {

namespace {

using namespace defines;

int roll_dice() {
	return 1 + (rand() % COMBAT_DICE);
}

float average_roll() {
	return (static_cast<float>(COMBAT_DICE) + 1.0f) * 0.5f;
}

} // namespace

Terrain terrain_at(int p_column, int p_row) {
	// Mostly open ground with rougher patches, so terrain is a real but not
	// constant factor in every fight.
	static const Terrain table[] = {
		{ "Plains", TERRAIN_PLAINS },
		{ "Forest", TERRAIN_FOREST },
		{ "Hills", TERRAIN_HILLS },
		{ "Marshes", TERRAIN_MARSH },
		{ "Urban", TERRAIN_URBAN },
		{ "Mountains", TERRAIN_MOUNTAINS },
	};
	static const int weights[] = { 4, 3, 2, 1, 1, 1 };
	static const int kinds = 6;
	static const int total = 12;

	const unsigned int hash = static_cast<unsigned int>(p_column) * 73856093u ^
			static_cast<unsigned int>(p_row) * 19349663u;

	int pick = static_cast<int>(hash % static_cast<unsigned int>(total));
	for (int i = 0; i < kinds; ++i) {
		pick -= weights[i];
		if (pick < 0) {
			return table[i];
		}
	}
	return table[0];
}

Battle::Battle() {
}

void Battle::start(int p_attacker, int p_defender, const Terrain &p_terrain) {
	active_ = true;
	attacker_ = p_attacker;
	defender_ = p_defender;
	hours_ = 0;
	terrain_ = p_terrain;

	sides_[ATTACKER] = BattleSideState();
	sides_[ATTACKER].unit = p_attacker;
	sides_[DEFENDER] = BattleSideState();
	sides_[DEFENDER].unit = p_defender;
}

void Battle::stop() {
	active_ = false;
}

void Battle::roll(BattleSide p_side, int p_target, const UnitModel &p_units) {
	BattleSideState &state = sides_[p_side];
	const Unit &attacker = p_units.get(state.unit);
	const Unit &target = p_units.get(p_target);

	state.dice = roll_dice();

	float hits = effective_attack(attacker.stats, target.stats);
	if (p_side == ATTACKER) {
		hits *= terrain_.attack_modifier;
	}

	// How much of that the target can parry: defence while it is the one holding
	// the province, breakthrough while it is the one attacking. Everything past
	// the cap lands unparried and hurts far more.
	const float parry = (p_side == ATTACKER ? target.stats.defence : target.stats.breakthrough) *
			DIRECT_FIRE_PER_DEFENCE;
	const float defended = hits < parry ? hits : parry;
	const float undefended = hits - defended;

	state.hits = hits;
	state.hits_defended = defended;
	state.hits_undefended = undefended;

	const float raw_damage = (defended * DEFENDED_HIT_MULTIPLIER +
									 undefended * UNDEFENDED_HIT_MULTIPLIER) *
			HITS_TO_DAMAGE * (static_cast<float>(state.dice) / average_roll());

	// A division that still has its morale absorbs most of the beating in
	// organisation; once it is shaken, the losses start hitting its men.
	const float org_share = ORG_DAMAGE_SHARE_BASE +
			ORG_DAMAGE_SHARE_ORG_SCALED * target.stats.organisation_ratio(target.organisation);

	state.org_damage = raw_damage * org_share;
	state.strength_damage = raw_damage * (1.0f - org_share);
}

BattleOutcome Battle::tick(UnitModel &p_units) {
	if (!active_) {
		return BATTLE_ONGOING;
	}

	++hours_;

	if (!p_units.is_alive(attacker_) || !p_units.is_alive(defender_)) {
		active_ = false;
		if (!p_units.is_alive(defender_)) {
			// A destroyed division holds no ground, so the attacker walks in.
			if (p_units.is_alive(attacker_)) {
				const Unit &dead = p_units.get(defender_);
				p_units.try_move(attacker_, dead.column, dead.row);
			}
			return BATTLE_ATTACKER_WON;
		}
		return BATTLE_DEFENDER_WON;
	}

	// Both sides shoot in the same hour, so the rolls are made before either
	// side's damage lands.
	roll(ATTACKER, defender_, p_units);
	roll(DEFENDER, attacker_, p_units);

	p_units.apply_damage(defender_, sides_[ATTACKER].org_damage, sides_[ATTACKER].strength_damage);
	p_units.apply_damage(attacker_, sides_[DEFENDER].org_damage, sides_[DEFENDER].strength_damage);

	const Unit &attacker = p_units.get(attacker_);
	const Unit &defender = p_units.get(defender_);

	// Organisation decides battles: a division that cannot keep fighting
	// withdraws long before it is destroyed.
	const bool attacker_broke = attacker.organisation <= ORG_RETREAT_THRESHOLD;
	const bool defender_broke = defender.organisation <= ORG_RETREAT_THRESHOLD;

	if (!attacker_broke && !defender_broke && hours_ < MAX_BATTLE_HOURS) {
		return BATTLE_ONGOING;
	}

	active_ = false;

	if (defender_broke && !attacker_broke) {
		// The defender gives up the province and the attacker walks in.
		const int column = defender.column;
		const int row = defender.row;
		p_units.retreat(defender_, attacker_);
		p_units.try_move(attacker_, column, row);
		return BATTLE_ATTACKER_WON;
	}

	// Either the attacker's organisation broke, or it ran out of time.
	p_units.retreat(attacker_, defender_);
	return BATTLE_DEFENDER_WON;
}

} // namespace infantry
