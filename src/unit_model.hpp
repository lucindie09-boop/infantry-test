#ifndef INFANTRY_UNIT_MODEL_HPP
#define INFANTRY_UNIT_MODEL_HPP

#include <vector>

#include "division_stats.hpp"
#include "map_model.hpp"

namespace infantry {

// A little circular dude standing on exactly one province, fielding one
// division's worth of stats.
struct Unit {
	int value = BLUE; // 0 = blue dude, 1 = red dude
	int column = 0;   // column in the continuous province field
	int row = 0;      // row in the continuous province field

	// Two pools, as in the model this is inspired by: organisation is morale and
	// strength is men and equipment. Breaking on organisation means withdrawing
	// from the fight; running out of strength means being destroyed.
	float organisation = 0.0f;
	float strength = 0.0f;

	DivisionStats stats;
	BattalionCounts battalions;
	bool alive = false;
};

// What came out of a movement attempt.
enum MoveOutcome {
	MOVE_OK = 0,              // stepped into an empty bordering province
	MOVE_ATTACK_STARTED = 1,  // walked into the enemy, which starts a battle
	MOVE_NOT_ADJACENT = 2,    // the target province does not border the dude
	MOVE_OFF_MAP = 3,         // the target province is outside the province field
	MOVE_UNAVAILABLE = 4,     // no such dude, or that dude is dead
	MOVE_IN_BATTLE = 5,       // that dude is already committed to a battle
};

class UnitModel {
public:
	UnitModel();

	// Re-places one dude per country on the inner edge facing the neighbouring
	// country, at full organisation and strength. Country 0 fields an infantry
	// division, country 1 an armoured one.
	void reset(const MapModel &map);

	int get_count() const { return static_cast<int>(units_.size()); }
	bool is_valid(int index) const;
	bool is_alive(int index) const;
	const Unit &get(int index) const { return units_[static_cast<size_t>(index)]; }

	int get_columns() const { return columns_; }
	int get_rows() const { return rows_; }
	float get_radius() const { return radius_; }

	int get_selected() const { return selected_; }
	// Only a dude that is still alive can be selected.
	void set_selected(int index);

	// Centre of a dude, in map space.
	float get_center_x(int index) const;
	float get_center_y(int index) const;

	// Province under a point in map space.
	int column_at(float x) const;
	int row_at(float y) const;
	bool is_inside(int column, int row) const;
	// Bordering means sharing an edge, so diagonal steps are not allowed.
	bool is_bordering(int from_column, int from_row, int to_column, int to_row) const;

	// Index of the living dude standing on a province, or -1.
	int unit_at(int column, int row) const;

	// Steps onto a bordering province, or triggers a battle when the enemy is
	// standing there.
	MoveOutcome try_move(int index, int column, int row);
	MoveOutcome try_move_by(int index, int dcol, int drow);

	// Re-fields a dude with a new template, as a fresh division at full
	// organisation and strength.
	void set_template(int index, const BattalionCounts &p_counts);

	// Both pools are damaged by the battle model.
	void apply_damage(int index, float org_damage, float strength_damage);

	// Regains organisation, and whatever strength growth is configured, over the
	// given number of peaceful hours.
	void recover(int index, float hours);

	// Withdraws one province away from the given dude, or holds if hemmed in.
	void retreat(int index, int away_from);

private:
	// Keeps a living dude selected after a loss.
	void select_next_alive();

	std::vector<Unit> units_;
	int columns_ = 16;
	int rows_ = 8;
	float tile_size_ = 48.0f;
	float margin_ = 96.0f;
	float radius_ = 16.0f;
	int selected_ = 0;
};

} // namespace infantry

#endif // INFANTRY_UNIT_MODEL_HPP
