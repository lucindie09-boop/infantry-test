#include "map_view.hpp"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace {

const Color BACKGROUND_COLOR(0.07f, 0.09f, 0.12f);
const Color BLUE_COLOR(0.17f, 0.38f, 0.85f);
const Color RED_COLOR(0.85f, 0.22f, 0.19f);
const Color NEUTRAL_COLOR(0.45f, 0.47f, 0.52f);
const Color PROVINCE_OUTLINE(0.05f, 0.06f, 0.09f, 0.55f);
const Color COUNTRY_OUTLINE(0.02f, 0.03f, 0.05f);

// The dudes are tinted so they stay readable on top of their own country.
const Color BLUE_UNIT_COLOR(0.36f, 0.62f, 1.0f);
const Color RED_UNIT_COLOR(1.0f, 0.45f, 0.37f);
const Color UNIT_OUTLINE(0.03f, 0.04f, 0.07f);
const Color SELECTION_RING(1.0f, 1.0f, 1.0f, 0.9f);
const Color HEALTH_BACKGROUND(0.04f, 0.05f, 0.08f, 0.9f);
const Color HINT_COLOR(1.0f, 1.0f, 1.0f, 0.75f);
const Color HUD_COLOR(0.95f, 0.96f, 0.98f, 0.9f);
const Color STATUS_COLOR(1.0f, 0.85f, 0.45f, 0.95f);

const float TAU = 6.283185307179586f;
const float SELECTION_GAP = 5.0f;
const float HP_BAR_HEIGHT = 4.0f;
const float HP_BAR_GAP = 3.0f;

} // namespace

MapView::MapView() : model_(8, 48.0f, 0.0f, 96.0f) {
	units_.reset(model_);
}

void MapView::_bind_methods() {
	ClassDB::bind_method(D_METHOD("generate_map"), &MapView::generate_map);
	ClassDB::bind_method(D_METHOD("center_map"), &MapView::center_map);
	ClassDB::bind_method(D_METHOD("get_country_count"), &MapView::get_country_count);
	ClassDB::bind_method(D_METHOD("get_province_count"), &MapView::get_province_count);
	ClassDB::bind_method(D_METHOD("get_province_value", "index"), &MapView::get_province_value);
	ClassDB::bind_method(D_METHOD("get_unit_count"), &MapView::get_unit_count);
	ClassDB::bind_method(D_METHOD("get_unit_value", "index"), &MapView::get_unit_value);
	ClassDB::bind_method(D_METHOD("get_unit_column", "index"), &MapView::get_unit_column);
	ClassDB::bind_method(D_METHOD("get_unit_row", "index"), &MapView::get_unit_row);
	ClassDB::bind_method(D_METHOD("get_unit_position", "index"), &MapView::get_unit_position);
	ClassDB::bind_method(D_METHOD("get_unit_hp", "index"), &MapView::get_unit_hp);
	ClassDB::bind_method(D_METHOD("get_unit_max_hp", "index"), &MapView::get_unit_max_hp);
	ClassDB::bind_method(D_METHOD("get_unit_attack", "index"), &MapView::get_unit_attack);
	ClassDB::bind_method(D_METHOD("is_unit_alive", "index"), &MapView::is_unit_alive);
	ClassDB::bind_method(D_METHOD("get_unit_at_grid", "column", "row"), &MapView::get_unit_at_grid);
	ClassDB::bind_method(D_METHOD("get_selected_unit"), &MapView::get_selected_unit);
	ClassDB::bind_method(D_METHOD("set_selected_unit", "index"), &MapView::set_selected_unit);
	ClassDB::bind_method(D_METHOD("try_move_unit", "index", "column", "row"), &MapView::try_move_unit);
	ClassDB::bind_method(D_METHOD("move_unit_by", "index", "dcol", "drow"), &MapView::move_unit_by);
	ClassDB::bind_method(D_METHOD("get_last_outcome"), &MapView::get_last_outcome);
	ClassDB::bind_method(D_METHOD("get_status"), &MapView::get_status);
	ClassDB::bind_method(D_METHOD("is_panning"), &MapView::is_panning);
	ClassDB::bind_method(D_METHOD("get_country_color", "value"), &MapView::get_country_color);
	ClassDB::bind_method(D_METHOD("get_unit_color", "value"), &MapView::get_unit_color);
	ClassDB::bind_method(D_METHOD("get_pan_offset"), &MapView::get_pan_offset);
	ClassDB::bind_method(D_METHOD("set_pan_offset", "offset"), &MapView::set_pan_offset);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "pan_offset"), "set_pan_offset", "get_pan_offset");

	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_OK", infantry::MOVE_OK);
	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_ATTACKED", infantry::MOVE_ATTACKED);
	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_DESTROYED", infantry::MOVE_DESTROYED);
	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_NOT_ADJACENT", infantry::MOVE_NOT_ADJACENT);
	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_OFF_MAP", infantry::MOVE_OFF_MAP);
	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_UNAVAILABLE", infantry::MOVE_UNAVAILABLE);
}

void MapView::_ready() {
	set_process_unhandled_input(true);
	generate_map();
	center_map();
}

void MapView::_draw() {
	draw_rect(Rect2(Vector2(), get_viewport_rect().size), BACKGROUND_COLOR, true);

	for (const infantry::Country &country : model_.get_countries()) {
		for (const infantry::Province &province : country.provinces) {
			const Rect2 tile(Vector2(province.x, province.y) + pan_offset_,
					Vector2(province.size, province.size));
			draw_rect(tile, get_country_color(province.value), true);
			draw_rect(tile, PROVINCE_OUTLINE, false, 1.0f);
		}

		// The border sits on top of the province grid. With no gap between the
		// countries the two outlines land on the same line, so the neighbours
		// share a single border instead of showing a seam.
		const Rect2 bounds(Vector2(country.x, country.y) + pan_offset_,
				Vector2(country.size, country.size));
		draw_rect(bounds, COUNTRY_OUTLINE, false, 4.0f);
	}

	for (int index = 0; index < units_.get_count(); ++index) {
		if (!units_.is_alive(index)) {
			continue;
		}

		const Vector2 centre(units_.get_center_x(index), units_.get_center_y(index));
		const Vector2 position = centre + pan_offset_;
		const float radius = units_.get_radius();

		draw_circle(position, radius, get_unit_color(units_.get(index).value));
		draw_arc(position, radius, 0.0f, TAU, 48, UNIT_OUTLINE, 3.0f, true);
		if (index == units_.get_selected()) {
			draw_arc(position, radius + SELECTION_GAP, 0.0f, TAU, 48, SELECTION_RING, 2.0f, true);
		}

		// Health bar: full width is full hp, so the remaining length is readable
		// at a glance in the middle of a fight.
		const infantry::Unit &unit = units_.get(index);
		const Rect2 bar(Vector2(position.x - radius, position.y + radius + HP_BAR_GAP),
				Vector2(radius * 2.0f, HP_BAR_HEIGHT));
		draw_rect(bar, HEALTH_BACKGROUND, true);

		float ratio = 0.0f;
		if (unit.max_hp > 0) {
			ratio = static_cast<float>(unit.hp) / static_cast<float>(unit.max_hp);
		}
		draw_rect(Rect2(bar.position, Vector2(bar.size.x * ratio, bar.size.y)), get_unit_color(unit.value), true);
		draw_rect(bar, UNIT_OUTLINE, false, 1.0f);
	}

	const Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
	if (font.is_valid()) {
		draw_string(font, Vector2(16.0f, 28.0f),
				"Middle-drag: pan    Left-click: step or attack   1 / 2: select dude   Arrows / WASD: step",
				HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 16, HINT_COLOR);

		String line;
		for (int index = 0; index < units_.get_count(); ++index) {
			if (index > 0) {
				line += "        ";
			}
			line += unit_line(index);
		}
		line += "        selected: " + side_name(units_.get_selected());
		draw_string(font, Vector2(16.0f, 52.0f), line, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 16, HUD_COLOR);

		draw_string(font, Vector2(16.0f, 76.0f), status_, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 16, STATUS_COLOR);
	}
}

void MapView::_unhandled_input(const Ref<InputEvent> &p_event) {
	InputEventMouseButton *button = Object::cast_to<InputEventMouseButton>(p_event.ptr());
	if (button != nullptr) {
		if (button->get_button_index() == MOUSE_BUTTON_MIDDLE) {
			panning_ = button->is_pressed();
			Input::get_singleton()->set_default_cursor_shape(panning_ ? Input::CURSOR_DRAG : Input::CURSOR_ARROW);
			get_viewport()->set_input_as_handled();
			return;
		}
		if (button->get_button_index() == MOUSE_BUTTON_LEFT && button->is_pressed()) {
			handle_left_click(button->get_position() - pan_offset_);
			get_viewport()->set_input_as_handled();
			return;
		}
	}

	InputEventMouseMotion *motion = Object::cast_to<InputEventMouseMotion>(p_event.ptr());
	if (panning_ && motion != nullptr) {
		set_pan_offset(pan_offset_ + motion->get_relative());
		get_viewport()->set_input_as_handled();
		return;
	}

	InputEventKey *key = Object::cast_to<InputEventKey>(p_event.ptr());
	if (key != nullptr && key->is_pressed() && handle_key(key->get_keycode())) {
		get_viewport()->set_input_as_handled();
	}
}

void MapView::handle_left_click(const Vector2 &p_map_point) {
	const int column = units_.column_at(p_map_point.x);
	const int row = units_.row_at(p_map_point.y);
	if (!units_.is_inside(column, row)) {
		return; // clicked the background
	}

	const int occupant = units_.unit_at(column, row);
	const int selected = units_.get_selected();
	if (occupant == selected) {
		return; // the selected dude is already there
	}

	// A dude out of reach cannot be attacked, so clicking it just selects it.
	// Bordering dudes fall through and get attacked by the step below.
	if (occupant != -1) {
		const infantry::Unit &mover = units_.get(selected);
		if (!units_.is_bordering(mover.column, mover.row, column, row)) {
			set_selected_unit(occupant);
			status_ = String("Selected the ") + side_name(occupant) +
					String(" dude - step into a bordering province to fight.");
			queue_redraw();
			return;
		}
	}

	report_outcome(units_.try_move(selected, column, row), selected, occupant);
}

bool MapView::handle_key(int p_key) {
	switch (p_key) {
		case KEY_1:
		case KEY_2: {
			const int wanted = p_key == KEY_1 ? 0 : 1;
			const int before = units_.get_selected();
			set_selected_unit(wanted);
			if (units_.get_selected() != before) {
				status_ = String("Selected the ") + side_name(wanted) + String(" dude.");
			} else {
				status_ = "That dude is not on the map.";
			}
			queue_redraw();
			return true;
		}
		case KEY_LEFT:
		case KEY_A:
			return step_selected(-1, 0);
		case KEY_RIGHT:
		case KEY_D:
			return step_selected(1, 0);
		case KEY_UP:
		case KEY_W:
			return step_selected(0, -1);
		case KEY_DOWN:
		case KEY_S:
			return step_selected(0, 1);
		default:
			return false;
	}
}

bool MapView::step_selected(int p_dcol, int p_drow) {
	const int selected = units_.get_selected();
	if (!units_.is_alive(selected)) {
		report_outcome(infantry::MOVE_UNAVAILABLE, selected, -1);
		return true;
	}

	const infantry::Unit &mover = units_.get(selected);
	const int column = mover.column + p_dcol;
	const int row = mover.row + p_drow;
	report_outcome(units_.try_move(selected, column, row), selected, units_.unit_at(column, row));
	return true;
}

void MapView::report_outcome(infantry::MoveOutcome p_outcome, int p_mover, int p_target) {
	last_outcome_ = static_cast<int>(p_outcome);

	switch (p_outcome) {
		case infantry::MOVE_OK:
			status_ = String("Stepped to province ") + cell_text(p_mover) + String(".");
			break;
		case infantry::MOVE_ATTACKED:
			status_ = String("Attack! The ") + side_name(p_target) + String(" dude is down to ") +
					health_text(p_target) + String(".");
			break;
		case infantry::MOVE_DESTROYED:
			status_ = String("The ") + side_name(p_target) + String(" dude was destroyed - its province is taken.");
			break;
		case infantry::MOVE_NOT_ADJACENT:
			status_ = "Dudes may only move to a bordering province.";
			break;
		case infantry::MOVE_OFF_MAP:
			status_ = "There is no province there.";
			break;
		default:
			status_ = "No living dude to move.";
			break;
	}

	queue_redraw();
}

String MapView::side_name(int index) const {
	if (!units_.is_valid(index)) {
		return "unknown";
	}
	return units_.get(index).value == infantry::BLUE ? "blue" : "red";
}

String MapView::cell_text(int index) const {
	if (!units_.is_valid(index)) {
		return "(?)";
	}
	return String("(") + String::num_int64(units_.get(index).column) + String(", ") +
			String::num_int64(units_.get(index).row) + String(")");
}

String MapView::health_text(int index) const {
	if (!units_.is_valid(index)) {
		return "0/0";
	}
	const infantry::Unit &unit = units_.get(index);
	return String::num_int64(unit.hp) + "/" + String::num_int64(unit.max_hp);
}

String MapView::unit_line(int index) const {
	const String name = side_name(index);
	if (!units_.is_alive(index)) {
		return name + String(": destroyed");
	}
	const infantry::Unit &unit = units_.get(index);
	return name + String(": ") + health_text(index) + String(" hp, atk ") + String::num_int64(unit.attack);
}

void MapView::generate_map() {
	model_.generate();
	units_.reset(model_);
	status_ = "Dudes move to a bordering province only - step into the enemy to attack.";
	last_outcome_ = infantry::MOVE_OK;
	queue_redraw();
}

int MapView::get_country_count() const {
	return model_.get_country_count();
}

int MapView::get_province_count() const {
	return model_.get_province_count();
}

int MapView::get_province_value(int index) const {
	return model_.get_province_value(index);
}

int MapView::get_unit_count() const {
	return units_.get_count();
}

int MapView::get_unit_value(int index) const {
	return units_.is_valid(index) ? units_.get(index).value : infantry::VALUE_NONE;
}

int MapView::get_unit_column(int index) const {
	return units_.is_valid(index) ? units_.get(index).column : -1;
}

int MapView::get_unit_row(int index) const {
	return units_.is_valid(index) ? units_.get(index).row : -1;
}

Vector2 MapView::get_unit_position(int index) const {
	if (!units_.is_valid(index)) {
		return Vector2();
	}
	return Vector2(units_.get_center_x(index), units_.get_center_y(index));
}

int MapView::get_unit_hp(int index) const {
	return units_.is_valid(index) ? units_.get(index).hp : 0;
}

int MapView::get_unit_max_hp(int index) const {
	return units_.is_valid(index) ? units_.get(index).max_hp : 0;
}

int MapView::get_unit_attack(int index) const {
	return units_.is_valid(index) ? units_.get(index).attack : 0;
}

bool MapView::is_unit_alive(int index) const {
	return units_.is_alive(index);
}

int MapView::get_unit_at_grid(int column, int row) const {
	return units_.unit_at(column, row);
}

int MapView::get_selected_unit() const {
	return units_.get_selected();
}

void MapView::set_selected_unit(int index) {
	units_.set_selected(index);
	queue_redraw();
}

int MapView::try_move_unit(int index, int column, int row) {
	report_outcome(units_.try_move(index, column, row), index, units_.unit_at(column, row));
	return last_outcome_;
}

int MapView::move_unit_by(int index, int dcol, int drow) {
	if (units_.is_valid(index)) {
		const infantry::Unit &mover = units_.get(index);
		return try_move_unit(index, mover.column + dcol, mover.row + drow);
	}
	report_outcome(infantry::MOVE_UNAVAILABLE, index, -1);
	return last_outcome_;
}

int MapView::get_last_outcome() const {
	return last_outcome_;
}

String MapView::get_status() const {
	return status_;
}

bool MapView::is_panning() const {
	return panning_;
}

void MapView::center_map() {
	const Vector2 viewport = get_viewport_rect().size;
	set_pan_offset(Vector2((viewport.x - model_.get_width()) * 0.5f,
			(viewport.y - model_.get_height()) * 0.5f));
}

Vector2 MapView::get_pan_offset() const {
	return pan_offset_;
}

void MapView::set_pan_offset(const Vector2 &offset) {
	pan_offset_ = offset;
	queue_redraw();
}

Color MapView::get_country_color(int value) const {
	switch (value) {
		case infantry::BLUE:
			return BLUE_COLOR;
		case infantry::RED:
			return RED_COLOR;
		default:
			return NEUTRAL_COLOR;
	}
}

Color MapView::get_unit_color(int value) const {
	switch (value) {
		case infantry::BLUE:
			return BLUE_UNIT_COLOR;
		case infantry::RED:
			return RED_UNIT_COLOR;
		default:
			return NEUTRAL_COLOR;
	}
}
