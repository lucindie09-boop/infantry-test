#ifndef INFANTRY_COMBAT_DEFINES_HPP
#define INFANTRY_COMBAT_DEFINES_HPP

// Every tunable number used by the battle maths lives here, in the same spirit
// as a Paradox-style defines table: the rules read these, nothing is hard-coded
// deep inside the algorithms, and rebalancing means editing this one file.
//
// The values are our own. They are picked so a battle between two similar
// divisions runs for roughly two days of battle time before one side's
// organisation breaks.

namespace infantry {
namespace defines {

// --- Battle pacing ----------------------------------------------------------
inline constexpr int BATTLE_TICK_HOURS = 1;      // one combat tick is one hour
inline constexpr float BATTLE_TICK_SECONDS = 0.35f; // ...and takes this long on screen
inline constexpr int COMBAT_DICE = 6;            // damage rolls 1..COMBAT_DICE
inline constexpr int MAX_BATTLE_HOURS = 240;     // ten days, then the attack fizzles out

// --- Direct fire ------------------------------------------------------------
// How many incoming hits one point of defence (or breakthrough, for the side
// that is attacking) can parry, and what a parried or unparried hit is worth.
// This cap is what makes a thin defence collapse non-linearly.
inline constexpr float DIRECT_FIRE_PER_DEFENCE = 1.0f;
inline constexpr float DEFENDED_HIT_MULTIPLIER = 0.5f;
inline constexpr float UNDEFENDED_HIT_MULTIPLIER = 1.5f;

// Hits are scaled into organisation/strength points with this conversion, so
// the whole pace of combat can be moved without touching the division stats.
inline constexpr float HITS_TO_DAMAGE = 0.030f;

// --- The two pools ----------------------------------------------------------
// Damage splits between organisation (morale - retreats at zero) and strength
// (men and equipment - destroyed at zero). The stronger a unit's organisation
// still is, the larger the share that goes into organisation.
inline constexpr float ORG_DAMAGE_SHARE_BASE = 0.70f;
inline constexpr float ORG_DAMAGE_SHARE_ORG_SCALED = 0.10f;
inline constexpr float ORG_RETREAT_THRESHOLD = 25.0f;

// Out of combat a division gets its morale back. Equipment is not modelled, so
// strength only ever falls: divisions are destroyed by accumulating damage over
// several battles, not by one bad afternoon.
inline constexpr float ORG_RECOVERY_PER_HOUR = 2.5f;
inline constexpr float STRENGTH_RECOVERY_PER_HOUR = 0.0f;

// --- Terrain ----------------------------------------------------------------
// Scales the attacker's hits only; the defender shoots back unpenalised.
inline constexpr float TERRAIN_PLAINS = 1.00f;
inline constexpr float TERRAIN_FOREST = 0.80f;
inline constexpr float TERRAIN_HILLS = 0.70f;
inline constexpr float TERRAIN_MOUNTAINS = 0.60f;
inline constexpr float TERRAIN_MARSH = 0.70f;
inline constexpr float TERRAIN_URBAN = 0.50f;

// --- Battalions -------------------------------------------------------------
// One battalion's worth of each stat. Division values are derived from these by
// summing strength, attacks, defence and width, and averaging organisation,
// armour, piercing and hardness, exactly the way a division template works.
inline constexpr float INFANTRY_STRENGTH = 25.0f;
inline constexpr float INFANTRY_ORG = 60.0f;
inline constexpr float INFANTRY_SOFT_ATTACK = 6.0f;
inline constexpr float INFANTRY_HARD_ATTACK = 1.0f;
inline constexpr float INFANTRY_AIR_ATTACK = 0.0f;
inline constexpr float INFANTRY_DEFENCE = 12.0f;
inline constexpr float INFANTRY_BREAKTHROUGH = 4.0f;
inline constexpr float INFANTRY_ARMOUR = 0.0f;
inline constexpr float INFANTRY_PIERCING = 4.0f;
inline constexpr float INFANTRY_HARDNESS = 0.0f;
inline constexpr float INFANTRY_WIDTH = 2.0f;
inline constexpr float INFANTRY_SUPPLY_USE = 0.06f;

inline constexpr float ARTILLERY_STRENGTH = 10.0f;
inline constexpr float ARTILLERY_ORG = 10.0f;
inline constexpr float ARTILLERY_SOFT_ATTACK = 25.0f;
inline constexpr float ARTILLERY_HARD_ATTACK = 2.0f;
inline constexpr float ARTILLERY_AIR_ATTACK = 0.0f;
inline constexpr float ARTILLERY_DEFENCE = 6.0f;
inline constexpr float ARTILLERY_BREAKTHROUGH = 8.0f;
inline constexpr float ARTILLERY_ARMOUR = 0.0f;
inline constexpr float ARTILLERY_PIERCING = 4.0f;
inline constexpr float ARTILLERY_HARDNESS = 0.0f;
inline constexpr float ARTILLERY_WIDTH = 3.0f;
inline constexpr float ARTILLERY_SUPPLY_USE = 0.12f;

inline constexpr float TANK_STRENGTH = 30.0f;
inline constexpr float TANK_ORG = 60.0f;
inline constexpr float TANK_SOFT_ATTACK = 20.0f;
inline constexpr float TANK_HARD_ATTACK = 18.0f;
inline constexpr float TANK_AIR_ATTACK = 0.0f;
inline constexpr float TANK_DEFENCE = 4.0f;
inline constexpr float TANK_BREAKTHROUGH = 30.0f;
inline constexpr float TANK_ARMOUR = 60.0f;
inline constexpr float TANK_PIERCING = 40.0f;
inline constexpr float TANK_HARDNESS = 0.9f;
inline constexpr float TANK_WIDTH = 2.0f;
inline constexpr float TANK_SUPPLY_USE = 0.20f;

} // namespace defines
} // namespace infantry

#endif // INFANTRY_COMBAT_DEFINES_HPP
