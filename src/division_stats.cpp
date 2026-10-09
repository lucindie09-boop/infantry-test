#include "division_stats.hpp"

#include <cmath>

#include "combat_defines.hpp"

namespace infantry {

namespace {

using namespace defines;

float average_or_zero(float p_total, int p_count) {
	return p_count > 0 ? p_total / static_cast<float>(p_count) : 0.0f;
}

void apply_infantry(DivisionStats &r_stats, float &r_armour, float &r_piercing, float &r_hardness) {
	r_stats.max_strength += INFANTRY_STRENGTH;
	r_stats.max_organisation += INFANTRY_ORG;
	r_stats.soft_attack += INFANTRY_SOFT_ATTACK;
	r_stats.hard_attack += INFANTRY_HARD_ATTACK;
	r_stats.air_attack += INFANTRY_AIR_ATTACK;
	r_stats.defence += INFANTRY_DEFENCE;
	r_stats.breakthrough += INFANTRY_BREAKTHROUGH;
	r_stats.combat_width += INFANTRY_WIDTH;
	r_stats.supply_use += INFANTRY_SUPPLY_USE;
	r_armour += INFANTRY_ARMOUR;
	r_piercing += INFANTRY_PIERCING;
	r_hardness += INFANTRY_HARDNESS;
}

void apply_artillery(DivisionStats &r_stats, float &r_armour, float &r_piercing, float &r_hardness) {
	r_stats.max_strength += ARTILLERY_STRENGTH;
	r_stats.max_organisation += ARTILLERY_ORG;
	r_stats.soft_attack += ARTILLERY_SOFT_ATTACK;
	r_stats.hard_attack += ARTILLERY_HARD_ATTACK;
	r_stats.air_attack += ARTILLERY_AIR_ATTACK;
	r_stats.defence += ARTILLERY_DEFENCE;
	r_stats.breakthrough += ARTILLERY_BREAKTHROUGH;
	r_stats.combat_width += ARTILLERY_WIDTH;
	r_stats.supply_use += ARTILLERY_SUPPLY_USE;
	r_armour += ARTILLERY_ARMOUR;
	r_piercing += ARTILLERY_PIERCING;
	r_hardness += ARTILLERY_HARDNESS;
}

void apply_anti_tank(DivisionStats &r_stats, float &r_armour, float &r_piercing, float &r_hardness) {
	r_stats.max_strength += ANTI_TANK_STRENGTH;
	r_stats.max_organisation += ANTI_TANK_ORG;
	r_stats.soft_attack += ANTI_TANK_SOFT_ATTACK;
	r_stats.hard_attack += ANTI_TANK_HARD_ATTACK;
	r_stats.air_attack += ANTI_TANK_AIR_ATTACK;
	r_stats.defence += ANTI_TANK_DEFENCE;
	r_stats.breakthrough += ANTI_TANK_BREAKTHROUGH;
	r_stats.combat_width += ANTI_TANK_WIDTH;
	r_stats.supply_use += ANTI_TANK_SUPPLY_USE;
	r_armour += ANTI_TANK_ARMOUR;
	r_piercing += ANTI_TANK_PIERCING;
	r_hardness += ANTI_TANK_HARDNESS;
}

void apply_tank(DivisionStats &r_stats, float &r_armour, float &r_piercing, float &r_hardness) {
	r_stats.max_strength += TANK_STRENGTH;
	r_stats.max_organisation += TANK_ORG;
	r_stats.soft_attack += TANK_SOFT_ATTACK;
	r_stats.hard_attack += TANK_HARD_ATTACK;
	r_stats.air_attack += TANK_AIR_ATTACK;
	r_stats.defence += TANK_DEFENCE;
	r_stats.breakthrough += TANK_BREAKTHROUGH;
	r_stats.combat_width += TANK_WIDTH;
	r_stats.supply_use += TANK_SUPPLY_USE;
	r_armour += TANK_ARMOUR;
	r_piercing += TANK_PIERCING;
	r_hardness += TANK_HARDNESS;
}

} // namespace

float DivisionStats::organisation_ratio(float p_organisation) const {
	if (max_organisation <= 0.0f) {
		return 0.0f;
	}
	const float ratio = p_organisation / max_organisation;
	if (ratio < 0.0f) {
		return 0.0f;
	}
	return ratio > 1.0f ? 1.0f : ratio;
}

DivisionStats build_division(const BattalionCounts &p_counts) {
	DivisionStats stats;

	const int infantry = p_counts.infantry > 0 ? p_counts.infantry : 0;
	const int artillery = p_counts.artillery > 0 ? p_counts.artillery : 0;
	const int anti_tank = p_counts.anti_tank > 0 ? p_counts.anti_tank : 0;
	const int tanks = p_counts.tanks > 0 ? p_counts.tanks : 0;
	const int line = infantry + artillery + anti_tank + tanks;
	if (line == 0) {
		return stats;
	}

	// Summed across the battalions.
	float armour = 0.0f;
	float piercing = 0.0f;
	float hardness = 0.0f;

	for (int i = 0; i < infantry; ++i) {
		apply_infantry(stats, armour, piercing, hardness);
	}
	for (int i = 0; i < artillery; ++i) {
		apply_artillery(stats, armour, piercing, hardness);
	}
	for (int i = 0; i < anti_tank; ++i) {
		apply_anti_tank(stats, armour, piercing, hardness);
	}
	for (int i = 0; i < tanks; ++i) {
		apply_tank(stats, armour, piercing, hardness);
	}

	// Averaged across the battalions: a division's morale, armour, piercing and
	// hardness are dragged down by every unarmoured battalion riding along with
	// it. This is why a couple of anti-tank guns in a big infantry division lift
	// its piercing only so far, and why tanks in an infantry division are not
	// armour.
	stats.max_organisation = average_or_zero(stats.max_organisation, line);
	stats.armour = average_or_zero(armour, line);
	stats.piercing = average_or_zero(piercing, line);
	stats.hardness = average_or_zero(hardness, line);

	return stats;
}

float effective_attack(const DivisionStats &p_attacker, const DivisionStats &p_target) {
	float hardness = p_target.hardness;
	if (hardness < 0.0f) {
		hardness = 0.0f;
	} else if (hardness > 1.0f) {
		hardness = 1.0f;
	}
	return p_attacker.soft_attack * (1.0f - hardness) + p_attacker.hard_attack * hardness;
}

bool pierces(const DivisionStats &p_attacker, const DivisionStats &p_target) {
	return p_attacker.piercing >= p_target.armour;
}

float fighting_strength(const DivisionStats &p_stats, float p_strength) {
	if (p_stats.max_strength <= 0.0f || p_strength <= 0.0f) {
		return 0.0f;
	}

	float ratio = p_strength / p_stats.max_strength;
	if (ratio > 1.0f) {
		ratio = 1.0f;
	}

	// Rounded down to whole steps, and never below one step: a division that is
	// still in the field still shoots.
	const float stepped = std::floor(ratio / FIGHTING_STRENGTH_STEP) * FIGHTING_STRENGTH_STEP;
	return stepped < FIGHTING_STRENGTH_STEP ? FIGHTING_STRENGTH_STEP : stepped;
}

BattalionCounts infantry_division() {
	BattalionCounts counts;
	counts.infantry = 7;
	counts.artillery = 4;
	counts.anti_tank = 3;
	return counts;
}

BattalionCounts armoured_division() {
	BattalionCounts counts;
	counts.infantry = 4;
	counts.tanks = 6;
	return counts;
}

const char *division_name(const BattalionCounts &p_counts) {
	if (p_counts.tanks > 0 && p_counts.infantry > 0) {
		return "Armoured";
	}
	if (p_counts.tanks > 0) {
		return "Armour";
	}
	return "Infantry";
}

} // namespace infantry
