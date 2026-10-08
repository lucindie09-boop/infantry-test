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

	const Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
	if (font.is_valid()) {
		float y = 24.0f;
		const float step = 19.0f;

		draw_string(font, Vector2(16.0f, y),
				"Middle-drag: pan     Left-click: step or attack (a battle)     1 / 2: select     Arrows / WASD: step",
				HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 13, HINT_COLOR);
		y += step;

		for (int index = 0; index < units_.get_count(); ++index) {
			draw_string(font, Vector2(16.0f, y), summary_line(index), HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 13, HUD_COLOR);
			y += step;
			draw_string(font, Vector2(16.0f, y), combat_line(index), HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 13, HUD_COLOR);
			y += step;
		}

		if (battle_.is_active()) {
			draw_string(font, Vector2(16.0f, y), battle_header(), HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 13, BATTLE_COLOR);
			y += step;
			draw_string(font, Vector2(16.0f, y), battle_attack_line(infantry::ATTACKER),
					HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 13, BATTLE_COLOR);
			y += step;
			draw_string(font, Vector2(16.0f, y), battle_attack_line(infantry::DEFENDER),
					HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 13, BATTLE_COLOR);
			y += step;
		}

		draw_string(font, Vector2(16.0f, y), status_, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 13, STATUS_COLOR);
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

String MapView::summary_line(int index) const {
	const String name = side_name(index);
	if (!units_.is_alive(index)) {
		return name + String("  destroyed");
	}

	const infantry::Unit &unit = units_.get(index);
	const infantry::DivisionStats &block = unit.stats;
	return name + String("  ") + String(infantry::division_name(unit.battalions)) +
			String("  ") + String::num_int64(unit.battalions.infantry) + String(" inf / ") +
			String::num_int64(unit.battalions.artillery) + String(" arty / ") +
			String::num_int64(unit.battalions.tanks) + String(" tk") +
			String("   org ") + fmt1(unit.organisation) + String("/") + fmt1(block.max_organisation) +
			String("   strength ") + fmt1(unit.strength) + String("/") + fmt1(block.max_strength) +
			String("   width ") + fmt1(block.combat_width) +
			String("   supply ") + fmt2(block.supply_use) +
			String("   hardness ") + fmt2(block.hardness);
}

String MapView::combat_line(int index) const {
	if (!units_.is_alive(index)) {
		return String("");
	}

	const infantry::DivisionStats &block = units_.get(index).stats;
	return String("       soft atk ") + fmt1(block.soft_attack) +
			String("   hard atk ") + fmt1(block.hard_attack) +
			String("   air atk ") + fmt1(block.air_attack) +
			String("   defence ") + fmt1(block.defence) +
			String("   breakthrough ") + fmt1(block.breakthrough) +
			String("   armour ") + fmt1(block.armour) +
			String("   piercing ") + fmt1(block.piercing) +
			String("   terrain here: ") + get_terrain_name(units_.get(index).column, units_.get(index).row);
}

String MapView::battle_header() const {
	return String("BATTLE hour ") + String::num_int64(battle_.get_hours()) +
			String(" in ") + cell_text(battle_.get_defender()) +
			String("   ") + String(battle_.get_terrain().name) +
			String(" (attacker x") + fmt2(battle_.get_terrain().attack_modifier) + String(")") +
			String("   attacker: ") + side_name(battle_.get_attacker()) +
			String("   defender: ") + side_name(battle_.get_defender());
}

String MapView::battle_attack_line(int p_side) const {
	const infantry::BattleSideState &state = battle_.get_side(p_side);
	if (!units_.is_valid(state.unit)) {
		return String("-");
	}
	const int target = p_side == infantry::ATTACKER ? battle_.get_defender() : battle_.get_attacker();

	return String("   ") + side_name(state.unit) + String(" -> ") + side_name(target) +
			String("   ") + fmt1(state.hits) + String(" hits (roll ") + String::num_int64(state.dice) + String(")") +
			String("   ") + fmt1(state.hits_defended) + String(" parried / ") +
			fmt1(state.hits_undefended) + String(" through") +
			String("   ->   target org -") + fmt2(state.org_damage) +
			String("   strength -") + fmt2(state.strength_damage);
}

void MapView::generate_map() {
	model_.generate();
	units_.reset(model_);
	battle_.stop();
	battle_clock_ = 0.0;
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
