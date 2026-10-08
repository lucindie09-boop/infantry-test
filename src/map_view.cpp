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
#include <godot_cpp/variant/dictionary.hpp>
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

// Dudes are tinted so they stay readable on top of their own country.
const Color BLUE_UNIT_COLOR(0.36f, 0.62f, 1.0f);
const Color RED_UNIT_COLOR(1.0f, 0.45f, 0.37f);
const Color STRENGTH_COLOR(0.93f, 0.94f, 0.97f);
const Color UNIT_OUTLINE(0.03f, 0.04f, 0.07f);
const Color SELECTION_RING(1.0f, 1.0f, 1.0f, 0.9f);
const Color BAR_BACKGROUND(0.04f, 0.05f, 0.08f, 0.9f);
const Color HINT_COLOR(1.0f, 1.0f, 1.0f, 0.72f);
const Color HUD_COLOR(0.93f, 0.94f, 0.97f, 0.92f);
const Color BATTLE_COLOR(1.0f, 0.62f, 0.30f, 0.95f);
const Color STATUS_COLOR(1.0f, 0.85f, 0.45f, 0.95f);

const float TAU = 6.283185307179586f;
const float SELECTION_GAP = 5.0f;
const float BAR_HEIGHT = 3.0f;
const float BAR_GAP = 1.0f;

String fmt2(float p_value) {
	return String::num(static_cast<double>(p_value), 2);
}

String fmt1(float p_value) {
	return String::num(static_cast<double>(p_value), 1);
}

// The battle bubble and the panel it opens.
const Color WIN_COLOR(0.32f, 0.80f, 0.36f, 1.0f);
const Color LOSS_COLOR(0.92f, 0.30f, 0.24f, 1.0f);
const Color EVEN_COLOR(0.95f, 0.74f, 0.26f, 1.0f);
const Color BUBBLE_MARK(0.98f, 0.98f, 0.98f, 1.0f);
const Color PANEL_BACKGROUND(0.05f, 0.06f, 0.09f, 0.96f);
const Color PANEL_BORDER(0.34f, 0.37f, 0.44f, 1.0f);
const Color PANEL_TITLE(1.0f, 0.90f, 0.70f, 1.0f);
const Color PANEL_TEXT(0.92f, 0.94f, 0.97f, 1.0f);
const Color PANEL_DIM(0.72f, 0.76f, 0.83f, 1.0f);
const Color PANEL_OUTLINE(0.01f, 0.02f, 0.03f, 0.9f);

// The bubble sits between the two dudes rather than over either of them, so it
// is sized to fit the gap between two neighbouring divisions.
const float BUBBLE_RADIUS = 9.0f;
// Only used when the two dudes share a column, where the health bars hanging
// below them would otherwise sit under the marker.
const float BUBBLE_VERTICAL_LIFT = 10.0f;
// How far ahead one side has to be on organisation share before the bubble
// commits to calling it a win or a loss.
const float BATTLE_WIN_MARGIN = 0.02f;

// Panel layout: four stat rows per side, and a label column wide enough for the
// longest label so every value lines up down the column.
const int PANEL_STAT_ROWS = 4;
// Width of a stat row's label column, in pixels, so every value in a column
// starts at the same x.
const float PANEL_LABEL_WIDTH = 118.0f;

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
	ClassDB::bind_method(D_METHOD("is_unit_alive", "index"), &MapView::is_unit_alive);
	ClassDB::bind_method(D_METHOD("get_unit_at_grid", "column", "row"), &MapView::get_unit_at_grid);
	ClassDB::bind_method(D_METHOD("get_unit_division", "index"), &MapView::get_unit_division);
	ClassDB::bind_method(D_METHOD("get_unit_organisation", "index"), &MapView::get_unit_organisation);
	ClassDB::bind_method(D_METHOD("get_unit_max_organisation", "index"), &MapView::get_unit_max_organisation);
	ClassDB::bind_method(D_METHOD("get_unit_strength", "index"), &MapView::get_unit_strength);
	ClassDB::bind_method(D_METHOD("get_unit_max_strength", "index"), &MapView::get_unit_max_strength);
	ClassDB::bind_method(D_METHOD("get_unit_effective_attack", "index", "target"), &MapView::get_unit_effective_attack);
	ClassDB::bind_method(D_METHOD("get_unit_stats", "index"), &MapView::get_unit_stats);
	ClassDB::bind_method(D_METHOD("get_selected_unit"), &MapView::get_selected_unit);
	ClassDB::bind_method(D_METHOD("set_selected_unit", "index"), &MapView::set_selected_unit);
	ClassDB::bind_method(D_METHOD("try_move_unit", "index", "column", "row"), &MapView::try_move_unit);
	ClassDB::bind_method(D_METHOD("move_unit_by", "index", "dcol", "drow"), &MapView::move_unit_by);
	ClassDB::bind_method(D_METHOD("start_battle", "attacker", "defender"), &MapView::start_battle);
	ClassDB::bind_method(D_METHOD("tick_battle"), &MapView::tick_battle);
	ClassDB::bind_method(D_METHOD("recover_units", "hours"), &MapView::recover_units);
	ClassDB::bind_method(D_METHOD("is_battle_active"), &MapView::is_battle_active);
	ClassDB::bind_method(D_METHOD("get_battle_hours"), &MapView::get_battle_hours);
	ClassDB::bind_method(D_METHOD("get_battle_attacker"), &MapView::get_battle_attacker);
	ClassDB::bind_method(D_METHOD("get_battle_defender"), &MapView::get_battle_defender);
	ClassDB::bind_method(D_METHOD("get_battle_terrain"), &MapView::get_battle_terrain);
	ClassDB::bind_method(D_METHOD("get_battle_attack_modifier"), &MapView::get_battle_attack_modifier);
	ClassDB::bind_method(D_METHOD("get_battle_side_dice", "side"), &MapView::get_battle_side_dice);
	ClassDB::bind_method(D_METHOD("get_battle_side_hits", "side"), &MapView::get_battle_side_hits);
	ClassDB::bind_method(D_METHOD("get_battle_side_defended", "side"), &MapView::get_battle_side_defended);
	ClassDB::bind_method(D_METHOD("get_battle_side_undefended", "side"), &MapView::get_battle_side_undefended);
	ClassDB::bind_method(D_METHOD("get_battle_side_org_damage", "side"), &MapView::get_battle_side_org_damage);
	ClassDB::bind_method(D_METHOD("get_battle_side_strength_damage", "side"), &MapView::get_battle_side_strength_damage);
	ClassDB::bind_method(D_METHOD("get_terrain_name", "column", "row"), &MapView::get_terrain_name);
	ClassDB::bind_method(D_METHOD("is_battle_panel_open"), &MapView::is_battle_panel_open);
	ClassDB::bind_method(D_METHOD("set_battle_panel_open", "open"), &MapView::set_battle_panel_open);
	ClassDB::bind_method(D_METHOD("toggle_battle_panel"), &MapView::toggle_battle_panel);
	ClassDB::bind_method(D_METHOD("get_battle_bubble_position"), &MapView::get_battle_bubble_position);
	ClassDB::bind_method(D_METHOD("get_battle_local_side"), &MapView::get_battle_local_side);
	ClassDB::bind_method(D_METHOD("get_battle_local_margin"), &MapView::get_battle_local_margin);
	ClassDB::bind_method(D_METHOD("is_battle_local_winning"), &MapView::is_battle_local_winning);
	ClassDB::bind_method(D_METHOD("get_battle_estimated_hours"), &MapView::get_battle_estimated_hours);
	ClassDB::bind_method(D_METHOD("get_battle_verdict"), &MapView::get_battle_verdict);
	ClassDB::bind_method(D_METHOD("get_last_outcome"), &MapView::get_last_outcome);
	ClassDB::bind_method(D_METHOD("get_last_battle_outcome"), &MapView::get_last_battle_outcome);
	ClassDB::bind_method(D_METHOD("get_status"), &MapView::get_status);
	ClassDB::bind_method(D_METHOD("is_panning"), &MapView::is_panning);
	ClassDB::bind_method(D_METHOD("get_country_color", "value"), &MapView::get_country_color);
	ClassDB::bind_method(D_METHOD("get_unit_color", "value"), &MapView::get_unit_color);
	ClassDB::bind_method(D_METHOD("get_pan_offset"), &MapView::get_pan_offset);
	ClassDB::bind_method(D_METHOD("set_pan_offset", "offset"), &MapView::set_pan_offset);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "pan_offset"), "set_pan_offset", "get_pan_offset");

	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_OK", infantry::MOVE_OK);
	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_ATTACK_STARTED", infantry::MOVE_ATTACK_STARTED);
	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_NOT_ADJACENT", infantry::MOVE_NOT_ADJACENT);
	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_OFF_MAP", infantry::MOVE_OFF_MAP);
	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_UNAVAILABLE", infantry::MOVE_UNAVAILABLE);
	ClassDB::bind_integer_constant(get_class_static(), "", "MOVE_IN_BATTLE", infantry::MOVE_IN_BATTLE);
	ClassDB::bind_integer_constant(get_class_static(), "", "BATTLE_ONGOING", infantry::BATTLE_ONGOING);
	ClassDB::bind_integer_constant(get_class_static(), "", "BATTLE_ATTACKER_WON", infantry::BATTLE_ATTACKER_WON);
	ClassDB::bind_integer_constant(get_class_static(), "", "BATTLE_DEFENDER_WON", infantry::BATTLE_DEFENDER_WON);
	ClassDB::bind_integer_constant(get_class_static(), "", "SIDE_ATTACKER", infantry::ATTACKER);
	ClassDB::bind_integer_constant(get_class_static(), "", "SIDE_DEFENDER", infantry::DEFENDER);
}

void MapView::_ready() {
	set_process(true);
	set_process_unhandled_input(true);
	generate_map();
	center_map();
}

void MapView::_process(double p_delta) {
	// Game time runs faster than wall-clock time: one hour per tick.
	const double tick_seconds = infantry::defines::BATTLE_TICK_SECONDS;
	battle_clock_ += p_delta;

	const int hours = static_cast<int>(battle_clock_ / tick_seconds);
	if (hours <= 0) {
		if (battle_.is_active()) {
			queue_redraw();
		}
		return;
	}
	battle_clock_ -= static_cast<double>(hours) * tick_seconds;

	int consumed = 0;
	if (battle_.is_active()) {
		while (consumed < hours && battle_.is_active()) {
			tick_battle();
			++consumed;
		}
	}

	// Whatever is left of the elapsed time passes peacefully, and morale recovers.
	if (consumed < hours) {
		recover_units(static_cast<float>(hours - consumed));
	}
}

void MapView::draw_bar(const Rect2 &p_rect, float p_ratio, const Color &p_color) {
	draw_rect(p_rect, BAR_BACKGROUND, true);

	float ratio = p_ratio;
	if (ratio < 0.0f) {
		ratio = 0.0f;
	} else if (ratio > 1.0f) {
		ratio = 1.0f;
	}
	if (ratio > 0.0f) {
		draw_rect(Rect2(p_rect.position, Vector2(p_rect.size.x * ratio, p_rect.size.y)), p_color, true);
	}
	draw_rect(p_rect, UNIT_OUTLINE, false, 1.0f);
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
		const infantry::Unit &unit = units_.get(index);

		draw_circle(position, radius, get_unit_color(unit.value));
		draw_arc(position, radius, 0.0f, TAU, 48, UNIT_OUTLINE, 3.0f, true);
		if (index == units_.get_selected()) {
			draw_arc(position, radius + SELECTION_GAP, 0.0f, TAU, 48, SELECTION_RING, 2.0f, true);
		}

		// Organisation on top, strength underneath. Morale breaks long before a
		// division runs out of men, so the top bar is usually the one that ends
		// the fight.
		const float bar_width = radius * 2.0f;
		float bar_y = position.y + radius + 1.0f;
		draw_bar(Rect2(Vector2(position.x - radius, bar_y), Vector2(bar_width, BAR_HEIGHT)),
				unit.stats.organisation_ratio(unit.organisation), get_unit_color(unit.value));
		bar_y += BAR_HEIGHT + BAR_GAP;
		draw_bar(Rect2(Vector2(position.x - radius, bar_y), Vector2(bar_width, BAR_HEIGHT)),
				unit.stats.max_strength > 0.0f ? unit.strength / unit.stats.max_strength : 0.0f,
				STRENGTH_COLOR);
	}

	if (battle_.is_active()) {
		draw_battle_bubble();
	}

	const Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
	if (font.is_valid()) {
		float y = 24.0f;
		const float step = 19.0f;

		draw_string(font, Vector2(16.0f, y),
				"Pan: Middle Drag   -   Act: Left Click   -   Select: 1 / 2   -   Move: Arrows / WASD",
				HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 12, HINT_COLOR);
		y += step;

		for (int index = 0; index < units_.get_count(); ++index) {
			draw_string(font, Vector2(16.0f, y), summary_line(index), HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 12, HUD_COLOR);
			y += step;
			// Indented in pixels rather than with leading spaces, so the second
			// line of a dude's block hangs under the first by a fixed amount.
			draw_string(font, Vector2(32.0f, y), combat_line(index), HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 12, HUD_COLOR);
			y += step;
		}

		if (battle_.is_active()) {
			draw_emphasis_text(font, Vector2(16.0f, y), battle_header(), -1.0f, 12, BATTLE_COLOR,
					HORIZONTAL_ALIGNMENT_LEFT);
			y += step;
			draw_string(font, Vector2(16.0f, y), battle_attack_line(infantry::ATTACKER),
					HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 12, BATTLE_COLOR);
			y += step;
			draw_string(font, Vector2(16.0f, y), battle_attack_line(infantry::DEFENDER),
					HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 12, BATTLE_COLOR);
			y += step;
		}

		draw_emphasis_text(font, Vector2(16.0f, y), status_, -1.0f, 12, STATUS_COLOR, HORIZONTAL_ALIGNMENT_LEFT);

		if (battle_panel_open_ && battle_.is_active()) {
			draw_battle_panel(font);
		}
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
			const Vector2 screen_point = button->get_position();
			if (bubble_clicked(screen_point)) {
				toggle_battle_panel();
			} else {
				handle_left_click(screen_point - pan_offset_);
			}
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
	// Bordering dudes fall through and get attacked by the call below.
	if (occupant != -1) {
		const infantry::Unit &mover = units_.get(selected);
		if (!units_.is_bordering(mover.column, mover.row, column, row)) {
			set_selected_unit(occupant);
			status_ = String("Selected the ") + side_name(occupant) +
					String(" dude - step into a bordering province to attack it.");
			queue_redraw();
			return;
		}
	}

	move_or_attack(selected, column, row);
}

bool MapView::handle_key(int p_key) {
	switch (p_key) {
		case KEY_ESCAPE:
			if (battle_panel_open_) {
				set_battle_panel_open(false);
				return true;
			}
			return false;
		case KEY_1:
		case KEY_2: {
			const int wanted = p_key == KEY_1 ? 0 : 1;
			const int before = units_.get_selected();
			set_selected_unit(wanted);
			if (units_.get_selected() != before) {
				status_ = String("Selected the ") + side_name(wanted) + String(" dude.");
			} else {
				status_ = String("That dude is not on the map.");
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
		report_outcome(infantry::MOVE_UNAVAILABLE, selected);
		return true;
	}

	const infantry::Unit &mover = units_.get(selected);
	move_or_attack(selected, mover.column + p_dcol, mover.row + p_drow);
	return true;
}

void MapView::move_or_attack(int index, int column, int row) {
	if (is_committed(index)) {
		status_ = String("The ") + side_name(index) + String(" dude is already fighting - let the battle finish.");
		queue_redraw();
		return;
	}

	const infantry::MoveOutcome outcome = units_.try_move(index, column, row);
	if (outcome == infantry::MOVE_ATTACK_STARTED) {
		start_battle(index, units_.unit_at(column, row));
		return;
	}
	report_outcome(outcome, index);
}

bool MapView::is_committed(int index) const {
	if (!battle_.is_active()) {
		return false;
	}
	return index == battle_.get_attacker() || index == battle_.get_defender();
}

void MapView::report_outcome(infantry::MoveOutcome p_outcome, int p_mover) {
	last_outcome_ = static_cast<int>(p_outcome);

	switch (p_outcome) {
		case infantry::MOVE_OK:
			status_ = String("Stepped to province ") + cell_text(p_mover) + String(".");
			break;
		case infantry::MOVE_NOT_ADJACENT:
			status_ = String("Dudes may only move to a bordering province.");
			break;
		case infantry::MOVE_OFF_MAP:
			status_ = String("There is no province there.");
			break;
		default:
			status_ = String("No living dude to move.");
			break;
	}

	queue_redraw();
}

void MapView::report_battle_end(int p_outcome) {
	const String attacker = side_name(battle_.get_attacker());
	const String defender = side_name(battle_.get_defender());
	const String hours = String::num_int64(battle_.get_hours());

	if (p_outcome == infantry::BATTLE_ATTACKER_WON) {
		status_ = attacker + String(" broke the defence after ") + hours + String(" hours and took province ") +
				cell_text(battle_.get_attacker()) + String(".");
	} else {
		status_ = defender + String(" held the province after ") + hours + String(" hours; the ") + attacker +
				String(" dude withdrew.");
	}
}

String MapView::side_name(int index) const {
	if (!units_.is_valid(index)) {
		return String("unknown");
	}
	return units_.get(index).value == infantry::BLUE ? String("blue") : String("red");
}

String MapView::cell_text(int index) const {
	if (!units_.is_valid(index)) {
		return String("(?)");
	}
	return String("(") + String::num_int64(units_.get(index).column) + String(", ") +
			String::num_int64(units_.get(index).row) + String(")");
}

String MapView::side_label(int index) const {
	if (!units_.is_valid(index)) {
		return String("Unknown");
	}
	return units_.get(index).value == infantry::BLUE ? String("Blue") : String("Red");
}

String MapView::summary_line(int index) const {
	if (!units_.is_alive(index)) {
		return side_label(index) + String("  -  Destroyed");
	}

	const infantry::Unit &unit = units_.get(index);
	const infantry::DivisionStats &block = unit.stats;
	return side_label(index) + String("  -  ") + String(infantry::division_name(unit.battalions)) +
			String("  -  ") + String::num_int64(unit.battalions.infantry) + String(" inf / ") +
			String::num_int64(unit.battalions.artillery) + String(" arty / ") +
			String::num_int64(unit.battalions.tanks) + String(" tk") +
			String("  -  Organisation ") + fmt1(unit.organisation) + String(" / ") + fmt1(block.max_organisation) +
			String("  -  Strength ") + fmt1(unit.strength) + String(" / ") + fmt1(block.max_strength);
}

String MapView::combat_line(int index) const {
	if (!units_.is_alive(index)) {
		return String("");
	}

	const infantry::DivisionStats &block = units_.get(index).stats;
	return String("Soft Attack ") + fmt1(block.soft_attack) +
			String("  -  Hard Attack ") + fmt1(block.hard_attack) +
			String("  -  Air Attack ") + fmt1(block.air_attack) +
			String("  -  Defence ") + fmt1(block.defence) +
			String("  -  Breakthrough ") + fmt1(block.breakthrough) +
			String("  -  Armour ") + fmt1(block.armour) +
			String("  -  Piercing ") + fmt1(block.piercing) +
			String("  -  Width ") + fmt1(block.combat_width) +
			String("  -  Supply ") + fmt2(block.supply_use) +
			String("  -  Hardness ") + fmt2(block.hardness);
}

String MapView::battle_header() const {
	return String("Battle  -  Hour ") + String::num_int64(battle_.get_hours()) +
			String("  -  Province ") + cell_text(battle_.get_defender()) +
			String("  -  ") + String(battle_.get_terrain().name) +
			String("  -  Attacker x") + fmt2(battle_.get_terrain().attack_modifier) +
			String("  -  ") + side_label(battle_.get_attacker()) + String(" attacks, ") +
			side_label(battle_.get_defender()) + String(" defends");
}

String MapView::battle_attack_line(int p_side) const {
	const infantry::BattleSideState &state = battle_.get_side(p_side);
	if (!units_.is_valid(state.unit)) {
		return String("-");
	}
	const int target = p_side == infantry::ATTACKER ? battle_.get_defender() : battle_.get_attacker();

	return side_label(state.unit) + String("  ->  ") + side_label(target) +
			String("  -  ") + fmt1(state.hits) + String(" Hits (Roll ") + String::num_int64(state.dice) + String(")") +
			String("  -  ") + fmt1(state.hits_defended) + String(" Parried, ") +
			fmt1(state.hits_undefended) + String(" Through") +
			String("  -  Target: Organisation -") + fmt2(state.org_damage) +
			String(", Strength -") + fmt2(state.strength_damage);
}

void MapView::generate_map() {
	model_.generate();
	units_.reset(model_);
	battle_.stop();
	battle_clock_ = 0.0;
	battle_panel_open_ = false;
	status_ = String("Dudes move to a bordering province only - walk into the enemy to start a battle.");
	last_outcome_ = infantry::MOVE_OK;
	last_battle_outcome_ = infantry::BATTLE_ONGOING;
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

bool MapView::is_unit_alive(int index) const {
	return units_.is_alive(index);
}

int MapView::get_unit_at_grid(int column, int row) const {
	return units_.unit_at(column, row);
}

String MapView::get_unit_division(int index) const {
	if (!units_.is_valid(index)) {
		return String("");
	}
	return String(infantry::division_name(units_.get(index).battalions));
}

float MapView::get_unit_organisation(int index) const {
	return units_.is_valid(index) ? units_.get(index).organisation : 0.0f;
}

float MapView::get_unit_max_organisation(int index) const {
	return units_.is_valid(index) ? units_.get(index).stats.max_organisation : 0.0f;
}

float MapView::get_unit_strength(int index) const {
	return units_.is_valid(index) ? units_.get(index).strength : 0.0f;
}

float MapView::get_unit_max_strength(int index) const {
	return units_.is_valid(index) ? units_.get(index).stats.max_strength : 0.0f;
}

float MapView::get_unit_effective_attack(int index, int target) const {
	if (!units_.is_valid(index) || !units_.is_valid(target)) {
		return 0.0f;
	}
	return infantry::effective_attack(units_.get(index).stats, units_.get(target).stats);
}

Dictionary MapView::get_unit_stats(int index) const {
	Dictionary stats;
	if (!units_.is_valid(index)) {
		return stats;
	}

	const infantry::Unit &unit = units_.get(index);
	const infantry::DivisionStats &block = unit.stats;

	stats["division"] = String(infantry::division_name(unit.battalions));
	stats["infantry_battalions"] = unit.battalions.infantry;
	stats["artillery_battalions"] = unit.battalions.artillery;
	stats["tank_battalions"] = unit.battalions.tanks;
	stats["organisation"] = unit.organisation;
	stats["max_organisation"] = block.max_organisation;
	stats["strength"] = unit.strength;
	stats["max_strength"] = block.max_strength;
	stats["soft_attack"] = block.soft_attack;
	stats["hard_attack"] = block.hard_attack;
	stats["air_attack"] = block.air_attack;
	stats["defence"] = block.defence;
	stats["breakthrough"] = block.breakthrough;
	stats["armour"] = block.armour;
	stats["piercing"] = block.piercing;
	stats["hardness"] = block.hardness;
	stats["combat_width"] = block.combat_width;
	stats["supply_use"] = block.supply_use;
	return stats;
}

int MapView::get_selected_unit() const {
	return units_.get_selected();
}

void MapView::set_selected_unit(int index) {
	units_.set_selected(index);
	queue_redraw();
}

int MapView::try_move_unit(int index, int column, int row) {
	if (is_committed(index)) {
		last_outcome_ = static_cast<int>(infantry::MOVE_IN_BATTLE);
		status_ = String("The ") + side_name(index) + String(" dude is already fighting.");
		queue_redraw();
		return last_outcome_;
	}

	const infantry::MoveOutcome outcome = units_.try_move(index, column, row);
	if (outcome == infantry::MOVE_ATTACK_STARTED) {
		start_battle(index, units_.unit_at(column, row));
		return last_outcome_;
	}
	report_outcome(outcome, index);
	return last_outcome_;
}

int MapView::move_unit_by(int index, int dcol, int drow) {
	if (!units_.is_valid(index)) {
		report_outcome(infantry::MOVE_UNAVAILABLE, index);
		return last_outcome_;
	}
	const infantry::Unit &mover = units_.get(index);
	return try_move_unit(index, mover.column + dcol, mover.row + drow);
}

bool MapView::start_battle(int p_attacker, int p_defender) {
	if (battle_.is_active() || !units_.is_alive(p_attacker) || !units_.is_alive(p_defender)) {
		return false;
	}

	const infantry::Unit &attacker = units_.get(p_attacker);
	const infantry::Unit &defender = units_.get(p_defender);
	if (!units_.is_bordering(attacker.column, attacker.row, defender.column, defender.row)) {
		return false;
	}

	// The battle is fought in the defender's province, so its ground is what
	// the attacker has to cross.
	battle_.start(p_attacker, p_defender, infantry::terrain_at(defender.column, defender.row));
	battle_clock_ = 0.0;
	battle_panel_open_ = false;
	last_battle_outcome_ = infantry::BATTLE_ONGOING;
	status_ = String("Battle for province ") + cell_text(p_defender) + String(" - ") +
			String(battle_.get_terrain().name) + String(".");
	queue_redraw();
	return true;
}

int MapView::tick_battle() {
	if (!battle_.is_active()) {
		return infantry::BATTLE_ONGOING;
	}

	const infantry::BattleOutcome outcome = battle_.tick(units_);
	last_battle_outcome_ = static_cast<int>(outcome);
	if (outcome != infantry::BATTLE_ONGOING) {
		// The panel has nothing live to show once the fighting stops.
		battle_panel_open_ = false;
		report_battle_end(static_cast<int>(outcome));
	}
	queue_redraw();
	return last_battle_outcome_;
}

void MapView::recover_units(float p_hours) {
	if (p_hours <= 0.0f) {
		return;
	}

	for (int index = 0; index < units_.get_count(); ++index) {
		if (is_committed(index)) {
			continue;
		}
		units_.recover(index, p_hours);
	}
	queue_redraw();
}

bool MapView::is_battle_active() const {
	return battle_.is_active();
}

int MapView::get_battle_hours() const {
	return battle_.get_hours();
}

int MapView::get_battle_attacker() const {
	return battle_.get_attacker();
}

int MapView::get_battle_defender() const {
	return battle_.get_defender();
}

String MapView::get_battle_terrain() const {
	return String(battle_.get_terrain().name);
}

float MapView::get_battle_attack_modifier() const {
	return battle_.get_terrain().attack_modifier;
}

int MapView::get_battle_side_dice(int side) const {
	return battle_.get_side(side).dice;
}

float MapView::get_battle_side_hits(int side) const {
	return battle_.get_side(side).hits;
}

float MapView::get_battle_side_defended(int side) const {
	return battle_.get_side(side).hits_defended;
}

float MapView::get_battle_side_undefended(int side) const {
	return battle_.get_side(side).hits_undefended;
}

float MapView::get_battle_side_org_damage(int side) const {
	return battle_.get_side(side).org_damage;
}

float MapView::get_battle_side_strength_damage(int side) const {
	return battle_.get_side(side).strength_damage;
}

String MapView::get_terrain_name(int column, int row) const {
	return String(infantry::terrain_at(column, row).name);
}

bool MapView::is_battle_panel_open() const {
	return battle_panel_open_;
}

void MapView::set_battle_panel_open(bool p_open) {
	battle_panel_open_ = p_open && battle_.is_active();
	queue_redraw();
}

void MapView::toggle_battle_panel() {
	set_battle_panel_open(!battle_panel_open_);
}

Vector2 MapView::battle_bubble_centre() const {
	if (!units_.is_valid(battle_.get_attacker()) || !units_.is_valid(battle_.get_defender())) {
		return Vector2();
	}

	// Halfway between the two dudes, so the marker belongs to neither of them.
	Vector2 centre(
			(units_.get_center_x(battle_.get_attacker()) + units_.get_center_x(battle_.get_defender())) * 0.5f,
			(units_.get_center_y(battle_.get_attacker()) + units_.get_center_y(battle_.get_defender())) * 0.5f);

	if (units_.get(battle_.get_attacker()).column == units_.get(battle_.get_defender()).column) {
		// Stacked in a column: the health bars hang below both dudes, so lift the
		// marker clear of them.
		centre.y -= BUBBLE_VERTICAL_LIFT;
	}

	return centre + pan_offset_;
}

Vector2 MapView::get_battle_bubble_position() const {
	return battle_bubble_centre();
}

bool MapView::bubble_clicked(const Vector2 &p_screen_point) const {
	if (!battle_.is_active()) {
		return false;
	}
	return battle_bubble_centre().distance_to(p_screen_point) <= BUBBLE_RADIUS + 3.0f;
}

int MapView::get_battle_local_side() const {
	if (!battle_.is_active()) {
		return -1;
	}
	// While a battle runs the two combatants are the only dudes on the map, so
	// the selected dude is always one of them.
	return units_.get_selected() == battle_.get_defender() ? infantry::DEFENDER : infantry::ATTACKER;
}

float MapView::get_battle_local_margin() const {
	const int local = get_battle_local_side();
	if (local < 0) {
		return 0.0f;
	}

	const int other = local == infantry::ATTACKER ? infantry::DEFENDER : infantry::ATTACKER;
	const int mine_index = battle_.get_side(local).unit;
	const int theirs_index = battle_.get_side(other).unit;
	if (!units_.is_valid(mine_index) || !units_.is_valid(theirs_index)) {
		return 0.0f;
	}

	const infantry::Unit &mine = units_.get(mine_index);
	const infantry::Unit &theirs = units_.get(theirs_index);
	return mine.stats.organisation_ratio(mine.organisation) -
			theirs.stats.organisation_ratio(theirs.organisation);
}

bool MapView::is_battle_local_winning() const {
	return get_battle_local_margin() > 0.0f;
}

Color MapView::battle_state_colour() const {
	const float margin = get_battle_local_margin();
	if (margin > BATTLE_WIN_MARGIN) {
		return WIN_COLOR;
	}
	if (margin < -BATTLE_WIN_MARGIN) {
		return LOSS_COLOR;
	}
	return EVEN_COLOR;
}

int MapView::get_battle_estimated_hours() const {
	if (!battle_.is_active()) {
		return -1;
	}

	// The fighting ends when the first side breaks, so the shortest run to the
	// retreat threshold is the estimate.
	float shortest = -1.0f;
	for (int side = 0; side < 2; ++side) {
		const infantry::BattleSideState &state = battle_.get_side(side);
		const infantry::BattleSideState &opponent =
				battle_.get_side(side == infantry::ATTACKER ? infantry::DEFENDER : infantry::ATTACKER);
		if (!units_.is_valid(state.unit)) {
			continue;
		}

		const infantry::Unit &unit = units_.get(state.unit);
		const float remaining = unit.organisation - infantry::defines::ORG_RETREAT_THRESHOLD;
		if (remaining <= 0.0f) {
			return 0;
		}
		if (opponent.org_damage <= 0.0f) {
			continue;
		}

		const float hours = remaining / opponent.org_damage;
		if (shortest < 0.0f || hours < shortest) {
			shortest = hours;
		}
	}

	if (shortest < 0.0f) {
		return -1;
	}
	const int estimate = static_cast<int>(shortest + 0.999f);
	return estimate < 1 ? 1 : estimate;
}

String MapView::get_battle_verdict() const {
	const float margin = get_battle_local_margin();
	if (margin > BATTLE_WIN_MARGIN) {
		return String("You are currently winning this battle.");
	}
	if (margin < -BATTLE_WIN_MARGIN) {
		return String("You are currently losing this battle.");
	}
	return String("The battle is even so far.");
}

String MapView::battle_estimate_line() const {
	const int hours = get_battle_estimated_hours();
	if (hours < 0) {
		return String("The line is not being worn down.");
	}
	if (hours == 0) {
		return String("A division is about to break.");
	}
	return String("Estimated to last another ") + String::num_int64(hours) + String(" hours.");
}

// Draws p_text twice, the second copy one pixel across, which thickens the
// glyphs enough to read as bold. This avoids depending on the project shipping
// a bold face for the interface font.
void MapView::draw_emphasis_text(const Ref<Font> &p_font, const Vector2 &p_position, const String &p_text,
		float p_width, int p_size, const Color &p_colour, HorizontalAlignment p_alignment) {
	if (!p_font.is_valid()) {
		return;
	}

	draw_string(p_font, p_position, p_text, p_alignment, p_width, p_size, p_colour);
	draw_string(p_font, p_position + Vector2(1.0f, 0.0f), p_text, p_alignment, p_width, p_size, p_colour);
}

String MapView::panel_title() const {
	return String("Battle of ") + cell_text(battle_.get_defender());
}

String MapView::panel_subtitle() const {
	return String(battle_.get_terrain().name) + String("  -  Hour ") + String::num_int64(battle_.get_hours()) +
			String("  -  Attacker Terrain x") + fmt2(battle_.get_terrain().attack_modifier);
}

String MapView::panel_hint() const {
	return String("Click the bubble or press Escape to close");
}

String MapView::battle_column_header(int p_side) const {
	return p_side == infantry::ATTACKER ? String("ATTACKER") : String("DEFENDER");
}

String MapView::battle_column_subtitle(int p_side) const {
	const int index = battle_.get_side(p_side).unit;
	if (!units_.is_valid(index)) {
		return String("-");
	}

	const infantry::Unit &unit = units_.get(index);
	return side_label(index) + String("   ") + String(infantry::division_name(unit.battalions)) +
			String("   (") + String::num_int64(unit.battalions.infantry) + String(" inf / ") +
			String::num_int64(unit.battalions.artillery) + String(" arty / ") +
			String::num_int64(unit.battalions.tanks) + String(" tk)");
}

String MapView::battle_stat_label(int p_row) const {
	switch (p_row) {
		case 0:
			return String("Organisation");
		case 1:
			return String("Strength");
		case 2:
			return String("Hits Landed");
		default:
			return String("Parries With");
	}
}

String MapView::battle_stat_value(int p_side, int p_row) const {
	const infantry::BattleSideState &state = battle_.get_side(p_side);
	if (!units_.is_valid(state.unit)) {
		return String("-");
	}

	const infantry::Unit &unit = units_.get(state.unit);
	const infantry::DivisionStats &block = unit.stats;

	switch (p_row) {
		case 0:
			return fmt1(unit.organisation) + String(" / ") + fmt1(block.max_organisation) +
					String("   (") + fmt1(block.organisation_ratio(unit.organisation) * 100.0f) + String("%)");
		case 1:
			return fmt1(unit.strength) + String(" / ") + fmt1(block.max_strength);
		case 2: {
			const int other_side = p_side == infantry::ATTACKER ? infantry::DEFENDER : infantry::ATTACKER;
			const infantry::Unit &other = units_.get(battle_.get_side(other_side).unit);
			return fmt1(infantry::effective_attack(unit.stats, other.stats)) + String("   (Roll ") +
					String::num_int64(state.dice) + String(")");
		}
		default:
			return String(p_side == infantry::ATTACKER ? "Breakthrough" : "Defence") + String("   ") +
					fmt1(p_side == infantry::ATTACKER ? block.breakthrough : block.defence);
	}
}

void MapView::draw_battle_bubble() {
	const Vector2 centre = battle_bubble_centre();

	draw_circle(centre, BUBBLE_RADIUS, battle_state_colour());
	draw_arc(centre, BUBBLE_RADIUS, 0.0f, TAU, 32, UNIT_OUTLINE, 2.0f, true);

	// Crossed swords, so the marker reads as a battle rather than a unit.
	draw_line(centre + Vector2(-4.0f, -4.0f), centre + Vector2(4.0f, 4.0f), BUBBLE_MARK, 2.0f, true);
	draw_line(centre + Vector2(-4.0f, 4.0f), centre + Vector2(4.0f, -4.0f), BUBBLE_MARK, 2.0f, true);
}

void MapView::draw_battle_panel(const Ref<Font> &p_font) {
	const Vector2 viewport = get_viewport_rect().size;
	const float width = 620.0f;
	const float height = 258.0f;
	const Rect2 panel(Vector2(viewport.x - width - 16.0f, viewport.y - height - 16.0f), Vector2(width, height));

	// A dark rim behind the panel body, so the map cannot bleed through the edge.
	draw_rect(panel, PANEL_OUTLINE, true);
	draw_rect(Rect2(panel.position + Vector2(2.0f, 2.0f), panel.size - Vector2(4.0f, 4.0f)), PANEL_BACKGROUND, true);
	draw_rect(panel, PANEL_BORDER, false, 2.0f);

	const float left = panel.position.x + 16.0f;
	const float inner = width - 32.0f;
	const float column_width = inner * 0.5f - 8.0f;
	const float attacker_x = left;
	const float defender_x = left + inner * 0.5f + 6.0f;
	const float step = 19.0f;
	float y = panel.position.y + 30.0f;

	// One heading block, centred, then a rule to separate it from the columns.
	draw_emphasis_text(p_font, Vector2(left, y), panel_title(), inner, 16, PANEL_TITLE, HORIZONTAL_ALIGNMENT_CENTER);
	y += step + 2.0f;
	draw_string(p_font, Vector2(left, y), panel_subtitle(), HORIZONTAL_ALIGNMENT_CENTER, inner, 12, PANEL_DIM);
	y += step + 8.0f;
	draw_line(Vector2(left, y - 6.0f), Vector2(panel.position.x + width - 16.0f, y - 6.0f), PANEL_BORDER, 1.0f);

	const float header_y = y;

	// Column header in the side's own colour, so the two halves read apart.
	for (int side = 0; side < 2; ++side) {
		const int index = battle_.get_side(side).unit;
		const Color header_colour = units_.is_valid(index) && units_.get(index).value == infantry::BLUE
				? BLUE_UNIT_COLOR
				: RED_UNIT_COLOR;
		draw_emphasis_text(p_font, Vector2(side == infantry::ATTACKER ? attacker_x : defender_x, y),
				battle_column_header(side), column_width, 13, header_colour, HORIZONTAL_ALIGNMENT_LEFT);
	}
	y += step + 2.0f;

	for (int side = 0; side < 2; ++side) {
		draw_string(p_font, Vector2(side == infantry::ATTACKER ? attacker_x : defender_x, y),
				battle_column_subtitle(side), HORIZONTAL_ALIGNMENT_LEFT, column_width, 12, PANEL_DIM);
	}
	y += step;

	// Label and value are drawn separately, with the value always starting at the
	// same offset, so the numbers line up down each column.
	for (int row = 0; row < PANEL_STAT_ROWS; ++row) {
		for (int side = 0; side < 2; ++side) {
			const float column_x = side == infantry::ATTACKER ? attacker_x : defender_x;
			draw_string(p_font, Vector2(column_x, y), battle_stat_label(row), HORIZONTAL_ALIGNMENT_LEFT,
					PANEL_LABEL_WIDTH - 8.0f, 12, PANEL_DIM);
			draw_string(p_font, Vector2(column_x + PANEL_LABEL_WIDTH, y), battle_stat_value(side, row),
					HORIZONTAL_ALIGNMENT_LEFT, column_width - PANEL_LABEL_WIDTH, 12, PANEL_TEXT);
		}
		y += step;
	}

	// A rule between the two columns, so two blocks of numbers do not read as one.
	const float column_rule_x = defender_x - 6.0f;
	draw_line(Vector2(column_rule_x, header_y - 12.0f), Vector2(column_rule_x, y - 8.0f), PANEL_BORDER, 1.0f);
	draw_line(Vector2(left, y - 8.0f), Vector2(panel.position.x + width - 16.0f, y - 8.0f), PANEL_BORDER, 1.0f);

	// The verdict and the estimate are the lines you actually read, so they get
	// the centre line and the emphasis.
	draw_emphasis_text(p_font, Vector2(left, y + 6.0f), get_battle_verdict(), inner, 14, battle_state_colour(),
			HORIZONTAL_ALIGNMENT_CENTER);
	y += step + 8.0f;
	draw_string(p_font, Vector2(left, y), battle_estimate_line(), HORIZONTAL_ALIGNMENT_CENTER, inner, 13, PANEL_TEXT);
	y += step;
	draw_string(p_font, Vector2(left, y), panel_hint(), HORIZONTAL_ALIGNMENT_CENTER, inner, 11, HINT_COLOR);
}

int MapView::get_last_outcome() const {
	return last_outcome_;
}

int MapView::get_last_battle_outcome() const {
	return last_battle_outcome_;
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
