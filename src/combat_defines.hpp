#ifndef INFANTRY_COMBAT_DEFINES_HPP
#define INFANTRY_COMBAT_DEFINES_HPP

// Every tunable number used by the battle maths lives here, in the same spirit
// as a Paradox-style defines table: the rules read these, nothing is hard-coded
// deep inside the algorithms, and rebalancing means editing this one file.
//
// The combat rules themselves follow the documented land battle model - the
// hourly tick, attacks measured against the target's defence (or breakthrough),
// the consumption of one defence per attack, the hit chances, the organisation
// and strength dice, and the armour/piercing checks. Where a number below is
// taken from that model it is marked as such; the ones marked as ours are
// pacing choices, and the battalion stats are our own test values.

namespace infantry {
namespace defines {

// --- Battle pacing ----------------------------------------------------------
inline constexpr int BATTLE_TICK_HOURS = 1;         // one combat tick is one hour
inline constexpr float BATTLE_TICK_SECONDS = 0.35f; // ...and takes this long on screen
// Ours, not the model's: there is no clock on a battle in the game this is
// modelled on, but a demo needs a way out of two divisions that cannot hurt
// each other. After this many hours the attack fizzles out.
inline constexpr int MAX_BATTLE_HOURS = 240;

// --- Hits -------------------------------------------------------------------
// Each attack is resolved on its own. A target that still has defences left
// shrugs off nine attacks in ten; once its defences are spent, only six in ten
// are shrugged off. This is the whole point of the defence and breakthrough
// stats, and it is what makes a thin defence collapse non-linearly.
inline constexpr float HIT_CHANCE_WITH_DEFENCES = 0.10f;
inline constexpr float HIT_CHANCE_WITHOUT_DEFENCES = 0.40f;

// --- Damage -----------------------------------------------------------------
// Per hit: a die for organisation and a die for strength, each scaled by this.
// A hit therefore costs 0.05 to 0.20 organisation, or 0.05 to 0.10 strength.
inline constexpr float COMBAT_DAMAGE_SCALE = 0.05f;
inline constexpr int ORG_DICE = 4;               // organisation die
inline constexpr int STR_DICE = 2;               // strength (men and equipment) die
// An armoured division whose armour the enemy cannot pierce rolls a bigger
// organisation die: it moves more freely under fire.
inline constexpr int ORG_DICE_ARMOUR_BONUS = 6;

// --- Armour and piercing ----------------------------------------------------
// An attack that cannot beat the target's armour lands at half strength: half
// the attacks are used at all, and each of those hits for half as much.
inline constexpr float UNPIERCED_ATTACK_MULTIPLIER = 0.5f;
inline constexpr float UNPIERCED_DAMAGE_MULTIPLIER = 0.5f;

// Damage output is scaled by the division's fighting strength, rounded down to
// whole steps of this size, so a division at 87% strength fights at 80%.
inline constexpr float FIGHTING_STRENGTH_STEP = 0.10f;

// --- The two pools ----------------------------------------------------------
// Organisation is morale: a division that runs out of it leaves the battle. It
// is deliberately not a partial threshold - a division fights at full effect
// until its organisation is gone.
//
// Ours: a division recovers this share of its maximum organisation per peaceful
// hour, and no strength, so divisions are destroyed by damage accumulated over
// several battles rather than recovering between them.
inline constexpr float ORG_RECOVERY_SHARE_PER_HOUR = 0.04f;
inline constexpr float STRENGTH_RECOVERY_PER_HOUR = 0.0f;

// --- Terrain ----------------------------------------------------------------
// The attacker's penalty: it scales both what the attacker can land and how
// well it can cover itself. The defender shoots back unpenalised.
inline constexpr float TERRAIN_PLAINS = 1.00f;   // no penalty
inline constexpr float TERRAIN_FOREST = 0.80f;   // -20%
inline constexpr float TERRAIN_HILLS = 0.70f;    // -30%
inline constexpr float TERRAIN_MARSH = 0.60f;    // -40%
inline constexpr float TERRAIN_MOUNTAINS = 0.40f; // -60%
inline constexpr float TERRAIN_URBAN = 0.70f;    // -30%

// --- Battalions -------------------------------------------------------------
// One battalion's worth of each stat: our own test values, in the shape the
// model uses. Division values are derived from these by summing strength,
// attacks, defence, breakthrough, width and supply, and averaging organisation,
// armour, piercing and hardness, exactly the way a division template works.
inline constexpr float INFANTRY_STRENGTH = 25.0f;
inline constexpr float INFANTRY_ORG = 60.0f;
inline constexpr float INFANTRY_SOFT_ATTACK = 6.0f;
inline constexpr float INFANTRY_HARD_ATTACK = 1.0f;
inline constexpr float INFANTRY_AIR_ATTACK = 0.0f;
inline constexpr float INFANTRY_DEFENCE = 22.0f;
inline constexpr float INFANTRY_BREAKTHROUGH = 6.0f;
inline constexpr float INFANTRY_ARMOUR = 0.0f;
inline constexpr float INFANTRY_PIERCING = 4.0f;
inline constexpr float INFANTRY_HARDNESS = 0.0f;
inline constexpr float INFANTRY_WIDTH = 2.0f;
inline constexpr float INFANTRY_SUPPLY_USE = 0.06f;

inline constexpr float ARTILLERY_STRENGTH = 10.0f;
inline constexpr float ARTILLERY_ORG = 10.0f;
inline constexpr float ARTILLERY_SOFT_ATTACK = 20.0f;
inline constexpr float ARTILLERY_HARD_ATTACK = 2.0f;
inline constexpr float ARTILLERY_AIR_ATTACK = 0.0f;
inline constexpr float ARTILLERY_DEFENCE = 6.0f;
inline constexpr float ARTILLERY_BREAKTHROUGH = 8.0f;
inline constexpr float ARTILLERY_ARMOUR = 0.0f;
inline constexpr float ARTILLERY_PIERCING = 4.0f;
inline constexpr float ARTILLERY_HARDNESS = 0.0f;
inline constexpr float ARTILLERY_WIDTH = 3.0f;
inline constexpr float ARTILLERY_SUPPLY_USE = 0.12f;

// Anti-tank: almost no soft attack and no staying power, but the piercing that
// lets everything else in the division bite through armour.
inline constexpr float ANTI_TANK_STRENGTH = 5.0f;
inline constexpr float ANTI_TANK_ORG = 6.0f;
inline constexpr float ANTI_TANK_SOFT_ATTACK = 2.0f;
inline constexpr float ANTI_TANK_HARD_ATTACK = 16.0f;
inline constexpr float ANTI_TANK_AIR_ATTACK = 0.0f;
inline constexpr float ANTI_TANK_DEFENCE = 6.0f;
inline constexpr float ANTI_TANK_BREAKTHROUGH = 2.0f;
inline constexpr float ANTI_TANK_ARMOUR = 0.0f;
inline constexpr float ANTI_TANK_PIERCING = 120.0f;
inline constexpr float ANTI_TANK_HARDNESS = 0.0f;
inline constexpr float ANTI_TANK_WIDTH = 1.0f;
inline constexpr float ANTI_TANK_SUPPLY_USE = 0.10f;

// Tanks: armoured and hard, so they absorb what cannot pierce them, and built
// to break a line rather than to hold one - note the breakthrough next to the
// miserable defence.
inline constexpr float TANK_STRENGTH = 30.0f;
inline constexpr float TANK_ORG = 60.0f;
inline constexpr float TANK_SOFT_ATTACK = 10.0f;
inline constexpr float TANK_HARD_ATTACK = 18.0f;
inline constexpr float TANK_AIR_ATTACK = 0.0f;
inline constexpr float TANK_DEFENCE = 4.0f;
inline constexpr float TANK_BREAKTHROUGH = 30.0f;
inline constexpr float TANK_ARMOUR = 45.0f;
inline constexpr float TANK_PIERCING = 40.0f;
inline constexpr float TANK_HARDNESS = 0.9f;
inline constexpr float TANK_WIDTH = 2.0f;
inline constexpr float TANK_SUPPLY_USE = 0.20f;

} // namespace defines
} // namespace infantry

#endif // INFANTRY_COMBAT_DEFINES_HPP
