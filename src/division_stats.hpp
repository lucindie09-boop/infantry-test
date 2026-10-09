#ifndef INFANTRY_DIVISION_STATS_HPP
#define INFANTRY_DIVISION_STATS_HPP

namespace infantry {

// The stat block a division template produces. Names follow the vocabulary a
// grand-strategy player would recognise; the numbers come from combat_defines.
struct DivisionStats {
	float max_strength = 0.0f;      // men and equipment
	float max_organisation = 0.0f;  // morale, averaged over the battalions
	float soft_attack = 0.0f;       // damage against unhardened targets
	float hard_attack = 0.0f;       // damage against hardened (armoured) targets
	float air_attack = 0.0f;
	float defence = 0.0f;           // hits parried while defending
	float breakthrough = 0.0f;      // hits parried while attacking
	float armour = 0.0f;
	float piercing = 0.0f;
	float hardness = 0.0f;          // 0 = soft target, 1 = fully armoured
	float combat_width = 0.0f;      // frontage the division takes up in a battle
	float supply_use = 0.0f;

	// Organisation as a fraction of its maximum, used to decide how much of the
	// incoming damage lands on morale rather than on men and equipment.
	float organisation_ratio(float p_organisation) const;
};

// How many of each battalion a division template contains.
struct BattalionCounts {
	int infantry = 0;
	int artillery = 0;
	int anti_tank = 0;
	int tanks = 0;
	int total() const { return infantry + artillery + anti_tank + tanks; }
};

// Builds a division's stat block from its composition: strength, attacks,
// defence, breakthrough, armour, piercing, width and supply are summed across
// battalions, while organisation, armour, piercing and hardness are averaged.
DivisionStats build_division(const BattalionCounts &p_counts);

// Attack actually useful against a given target: soft attack covers the
// unhardened part of the target, hard attack the hardened part. This is why
// soft attack alone struggles against armour.
float effective_attack(const DivisionStats &p_attacker, const DivisionStats &p_target);

// Whether an attack can beat the target's armour. If it cannot, most of the
// attack is wasted: half the attacks are used and each lands for half damage.
bool pierces(const DivisionStats &p_attacker, const DivisionStats &p_target);

// Damage output scales with how much of the division is still standing, rounded
// down to whole steps of FIGHTING_STRENGTH_STEP.
float fighting_strength(const DivisionStats &p_stats, float p_strength);

// A little catalogue so the two dudes can field different divisions.
BattalionCounts infantry_division();
BattalionCounts armoured_division();
const char *division_name(const BattalionCounts &p_counts);

} // namespace infantry

#endif // INFANTRY_DIVISION_STATS_HPP
