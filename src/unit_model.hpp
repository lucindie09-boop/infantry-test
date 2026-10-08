#ifndef INFANTRY_UNIT_MODEL_HPP
#define INFANTRY_UNIT_MODEL_HPP

#include <vector>

#include "map_model.hpp"

namespace infantry {

// A little circular dude standing on exactly one province.
struct Unit {
	int value = BLUE; // 0 = blue dude, 1 = red dude
	int column = 0;   // column in the continuous province field
	int row = 0;      // row in the continuous province field
	int hp = 0;
	int max_hp = 0;
	int attack = 0; // damage dealt to the dude standing on the target province
	bool alive = false;
};

// What came out of a step/attack attempt.
enum MoveOutcome {
	MOVE_OK = 0,           // stepped into an empty bordering province
	MOVE_ATTACKED = 1,     // hit the enemy standing on a bordering province
	MOVE_DESTROYED = 2,    // killed the enemy and took its province
	MOVE_NOT_ADJACENT = 3, // the target province does not border the dude
	MOVE_OFF_MAP = 4,      // the target province is outside the province field
	MOVE_UNAVAILABLE = 5,  // no such dude, or that dude is dead
};

// The movable pieces on the map. The geometry is cached from the MapModel so
// the movement and combat rules stay free of engine types.
class UnitModel {
public:
	static const int DEFAULT_MAX_HP = 10;
	static const int DEFAULT_ATTACK = 4;

	UnitModel();

	// Re-places one dude per country, full health, on the inner edge facing the
	// neighbouring country across the shared border.
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

	// Steps a dude onto a bordering province, or attacks the enemy standing
	// there. Returns what happened.
	MoveOutcome try_move(int index, int column, int row);
	MoveOutcome try_move_by(int index, int dcol, int drow);

private:
	// Keeps a living dude selected after a kill.
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
