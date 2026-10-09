#include "battle_model.hpp"

#include <cmath>
#include <cstdlib>

#include "unit_model.hpp"

namespace infantry {

namespace {

using namespace defines;

int random_die(int p_size) {
	return 1 + (rand() % p_size);
}

bool roll_chance(float p_chance) {
	return static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) + 1.0f) < p_chance;
}

int roll_hits(float p_attacks, float p_chance) {
	int hits = 0;
	const float whole = std::floor(p_attacks);
	for (int i = 0; i < static_cast<int>(whole); ++i) {
		if (roll_chance(p_chance)) {
			++hits;
		}
	}
	// A trailing fraction is worth one more roll against its own share of the
	// chance, so the expected number of hits stays proportional to the attacks.
	if (roll_chance((p_attacks - whole) * p_chance)) {
		++hits;
	}
	return hits;
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

void Battle::resolve(BattleSide p_side, int p_target, const UnitModel &p_units) {
	BattleSideState &state = sides_[p_side];
	const Unit &shooter = p_units.get(state.unit);
	const Unit &target = p_units.get(p_target);

	// Only the attacker fights under the terrain's penalty, and it suffers it
	// twice over: in what it can land, and in how well it can cover itself while
	// attacking. The target is the attacker exactly when this side is defending.
	const float own_terrain = p_side == ATTACKER ? terrain_.attack_modifier : 1.0f;
	const float target_terrain = p_side == ATTACKER ? 1.0f : terrain_.attack_modifier;

	// Attacks are split by the target's hardness, cut down by terrain, and then
	// wasted entirely if they cannot beat the target's armour.
	state.pierced = pierces(shooter.stats, target.stats);
	float attacks = effective_attack(shooter.stats, target.stats) * own_terrain;
	if (!state.pierced) {
		attacks *= UNPIERCED_ATTACK_MULTIPLIER;
	}
	state.attacks = attacks;

	// The target spends one defence per attack and then runs out of them. It
	// parries with its defence while it holds the province, and with its
	// breakthrough while it is the one attacking.
	const float parry_stat = p_side == ATTACKER ? target.stats.defence : target.stats.breakthrough;
	const float defences = parry_stat * target_terrain;
	state.target_defences = defences;

	state.blocked = attacks < defences ? attacks : defences;
	state.unblocked = attacks - state.blocked;

	// Every attack is rolled for on its own: one that a defence absorbed lands
	// one time in ten, one past the defences four times in ten.
	state.hits = static_cast<float>(roll_hits(state.blocked, HIT_CHANCE_WITH_DEFENCES) +
			roll_hits(state.unblocked, HIT_CHANCE_WITHOUT_DEFENCES));

	// An armoured division the enemy cannot pierce rolls a bigger organisation
	// die: it can move more freely under fire.
	state.org_die = target.stats.piercing < shooter.stats.armour ? ORG_DICE_ARMOUR_BONUS : ORG_DICE;
	state.str_die = STR_DICE;

	// One die for organisation and one for strength per hit...
	int org_rolled = 0;
	int strength_rolled = 0;
	for (int hit = 0; hit < static_cast<int>(state.hits); ++hit) {
		org_rolled += random_die(state.org_die);
		strength_rolled += random_die(state.str_die);
	}
	state.org_rolled = org_rolled;
	state.strength_rolled = strength_rolled;

	// ...scaled by the damage constant, by how much of the shooter is still
	// standing, and halved once more if its attacks could not pierce.
	float scale = COMBAT_DAMAGE_SCALE * fighting_strength(shooter.stats, shooter.strength);
	if (!state.pierced) {
		scale *= UNPIERCED_DAMAGE_MULTIPLIER;
	}

	state.org_damage = static_cast<float>(org_rolled) * scale;
	state.strength_damage = static_cast<float>(strength_rolled) * scale;
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

	// Both sides shoot in the same hour, so every roll is made before either
	// side's damage lands.
	resolve(ATTACKER, defender_, p_units);
	resolve(DEFENDER, attacker_, p_units);

	p_units.apply_damage(defender_, sides_[ATTACKER].org_damage, sides_[ATTACKER].strength_damage);
	p_units.apply_damage(attacker_, sides_[DEFENDER].org_damage, sides_[DEFENDER].strength_damage);

	const Unit &attacker = p_units.get(attacker_);
	const Unit &defender = p_units.get(defender_);

	// Organisation decides battles: a division fights at full effect right up
	// until it has none left, and then it leaves the field.
	const bool attacker_broke = attacker.organisation <= 0.0f;
	const bool defender_broke = defender.organisation <= 0.0f;

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

	// Either the attacker's organisation ran out, or the attack ran out of time.
	p_units.retreat(attacker_, defender_);
	return BATTLE_DEFENDER_WON;
}

} // namespace infantry
