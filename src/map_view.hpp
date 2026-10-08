#ifndef INFANTRY_MAP_VIEW_HPP
#define INFANTRY_MAP_VIEW_HPP

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "map_model.hpp"
#include "unit_model.hpp"

namespace godot {

// Draws the flat, top-down map of the two countries plus the two dudes standing
// on it, and owns the interactions:
//   middle mouse + drag : pan the map
//   left click          : step the selected dude, or attack the dude standing
//                         on a bordering province
//   1 / 2               : select the blue / red dude
//   arrows / WASD       : step the selected dude one province
//
// A dude may only move to a province that shares an edge with the one it holds.
// Stepping onto the province held by the other dude attacks it instead.
class MapView : public Node2D {
	GDCLASS(MapView, Node2D)

	infantry::MapModel model_;
	infantry::UnitModel units_;
	Vector2 pan_offset_;
	bool panning_ = false;
	String status_;
	int last_outcome_ = infantry::MOVE_OK;

protected:
	static void _bind_methods();

public:
	MapView();

	void _ready() override;
	void _draw() override;
	void _unhandled_input(const Ref<InputEvent> &p_event) override;

	// Rebuilds the province layer of both countries and re-places the dudes.
	void generate_map();

	int get_country_count() const;
	int get_province_count() const;
	// Value of the i-th province of the whole map: 0 = Blue, 1 = Red.
	int get_province_value(int index) const;

	int get_unit_count() const;
	int get_unit_value(int index) const;
	int get_unit_column(int index) const;
	int get_unit_row(int index) const;
	Vector2 get_unit_position(int index) const;
	int get_unit_hp(int index) const;
	int get_unit_max_hp(int index) const;
	int get_unit_attack(int index) const;
	bool is_unit_alive(int index) const;
	// Dude standing on the given province of the continuous field, or -1.
	int get_unit_at_grid(int column, int row) const;

	int get_selected_unit() const;
	void set_selected_unit(int index);

	// Steps a dude onto a bordering province, or attacks the enemy standing
	// there. Returns an infantry::MoveOutcome value:
	//   MOVE_OK 0, MOVE_ATTACKED 1, MOVE_DESTROYED 2,
	//   MOVE_NOT_ADJACENT 3, MOVE_OFF_MAP 4, MOVE_UNAVAILABLE 5
	int try_move_unit(int index, int column, int row);
	int move_unit_by(int index, int dcol, int drow);

	// Outcome of the last step/attack, and the line describing it on screen.
	int get_last_outcome() const;
	String get_status() const;

	bool is_panning() const;

	// Pans the map so that it sits in the middle of the viewport.
	void center_map();

	Vector2 get_pan_offset() const;
	void set_pan_offset(const Vector2 &offset);

	// Colour used for a province holding the given ownership value.
	Color get_country_color(int value) const;
	// Colour used for a dude belonging to the given value.
	Color get_unit_color(int value) const;

private:
	void handle_left_click(const Vector2 &p_map_point);
	bool handle_key(int p_key);
	bool step_selected(int p_dcol, int p_drow);
	void report_outcome(infantry::MoveOutcome p_outcome, int p_mover, int p_target);

	String side_name(int index) const;
	String cell_text(int index) const;
	String health_text(int index) const;
	String unit_line(int index) const;
};

} // namespace godot

#endif // INFANTRY_MAP_VIEW_HPP
