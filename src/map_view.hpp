#ifndef INFANTRY_MAP_VIEW_HPP
#define INFANTRY_MAP_VIEW_HPP

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "battle_model.hpp"
#include "map_model.hpp"
#include "unit_model.hpp"

namespace godot {

// Draws the flat, top-down map of the two countries, the two dudes standing on
// it and, while it lasts, the battle between them.
//
//   middle mouse + drag : pan the map
//   left click          : step the selected dude, or attack the dude standing
//                         on a bordering province
//   1 / 2               : select the blue / red dude
//   arrows / WASD       : step the selected dude one province
//
// A dude may only move to a province that shares an edge with the one it holds.
// Walking into the enemy starts a battle, which then runs hour by hour until
// one side's organisation breaks.
class MapView : public Node2D {
	GDCLASS(MapView, Node2D)

	infantry::MapModel model_;
	infantry::UnitModel units_;
	infantry::Battle battle_;
	Vector2 pan_offset_;
	double battle_clock_ = 0.0;
	bool panning_ = false;
	bool battle_panel_open_ = false;
	String status_;
	int last_outcome_ = infantry::MOVE_OK;
	int last_battle_outcome_ = infantry::BATTLE_ONGOING;

protected:
	static void _bind_methods();

public:
	MapView();

	void _ready() override;
	void _process(double p_delta) override;
	void _draw() override;
	void _unhandled_input(const Ref<InputEvent> &p_event) override;

	// Rebuilds the province layer of both countries and re-places the dudes.
	void generate_map();
	// Pans the map so that it sits in the middle of the viewport.
	void center_map();

	int get_country_count() const;
	int get_province_count() const;
	// Value of the i-th province of the whole map: 0 = Blue, 1 = Red.
	int get_province_value(int index) const;

	int get_unit_count() const;
	int get_unit_value(int index) const;
	int get_unit_column(int index) const;
	int get_unit_row(int index) const;
	Vector2 get_unit_position(int index) const;
	bool is_unit_alive(int index) const;
	int get_unit_at_grid(int column, int row) const;

	// The division this dude fields, and the pools its stats feed.
	String get_unit_division(int index) const;
	float get_unit_organisation(int index) const;
	float get_unit_max_organisation(int index) const;
	float get_unit_strength(int index) const;
	float get_unit_max_strength(int index) const;
	// Attack this dude would land on the given target, once the target's
	// hardness decides how much of it is soft and how much is hard attack.
	float get_unit_effective_attack(int index, int target) const;
	// The whole stat block, plus the pools and the battalion counts.
	Dictionary get_unit_stats(int index) const;

	int get_selected_unit() const;
	void set_selected_unit(int index);

	// Re-fields a dude with a different division template, at full organisation
	// and strength. Refused while it is fighting, because a division's stats are
	// fixed for the duration of a battle.
	bool set_unit_template(int index, int infantry, int artillery, int anti_tank, int tanks);

	// Steps a dude onto a bordering province, or starts a battle with the enemy
	// standing there. Returns an infantry::MoveOutcome value.
	int try_move_unit(int index, int column, int row);
	int move_unit_by(int index, int dcol, int drow);

	// Battles. start_battle refuses when the two dudes do not border each other.
	bool start_battle(int attacker, int defender);
	// Runs one battle hour by hand. The view calls this itself on a timer; it is
	// exposed so a battle can also be stepped through deliberately.
	int tick_battle();
	// Advances peaceful hours, which restores the organisation of every dude that
	// is not currently fighting.
	void recover_units(float hours);
	bool is_battle_active() const;
	int get_battle_hours() const;
	int get_battle_attacker() const;
	int get_battle_defender() const;
	String get_battle_terrain() const;
	float get_battle_attack_modifier() const;
	float get_battle_side_attacks(int side) const;
	float get_battle_side_defences(int side) const;
	float get_battle_side_blocked(int side) const;
	float get_battle_side_unblocked(int side) const;
	float get_battle_side_hits(int side) const;
	int get_battle_side_org_die(int side) const;
	int get_battle_side_org_rolled(int side) const;
	int get_battle_side_strength_rolled(int side) const;
	bool get_battle_side_pierced(int side) const;
	float get_battle_side_org_damage(int side) const;
	float get_battle_side_strength_damage(int side) const;
	String get_terrain_name(int column, int row) const;

	// The battle bubble on the map, and the panel it opens.
	bool is_battle_panel_open() const;
	void set_battle_panel_open(bool open);
	void toggle_battle_panel();
	// Screen position of the bubble's centre.
	Vector2 get_battle_bubble_position() const;

	// Judged from the selected dude's point of view: the side with the greater
	// share of its organisation left is the one currently winning.
	int get_battle_local_side() const;
	float get_battle_local_margin() const;
	bool is_battle_local_winning() const;
	// How much of the fight is going the selected dude's way, 1 to 100, where 50
	// is dead level.
	int get_battle_local_score() const;
	// Hours until the first side breaks, or -1 when neither is being worn down.
	int get_battle_estimated_hours() const;
	String get_battle_verdict() const;

	int get_last_outcome() const;
	int get_last_battle_outcome() const;
	String get_status() const;

	bool is_panning() const;
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
	void move_or_attack(int index, int column, int row);
	bool is_committed(int index) const;
	void report_outcome(infantry::MoveOutcome p_outcome, int p_mover);
	void report_battle_end(int p_outcome);
	void draw_bar(const Rect2 &p_rect, float p_ratio, const Color &p_color);

	bool bubble_clicked(const Vector2 &p_screen_point) const;
	Vector2 battle_bubble_centre() const;
	Color battle_state_colour() const;
	void draw_battle_bubble();
	void draw_battle_panel(const Ref<Font> &p_font);
	void draw_emphasis_text(const Ref<Font> &p_font, const Vector2 &p_position, const String &p_text,
			float p_width, int p_size, const Color &p_colour, HorizontalAlignment p_alignment);

	String side_label(int index) const;
	String battle_estimate_line() const;
	String panel_title() const;
	String panel_subtitle() const;
	String panel_hint() const;
	String battle_column_header(int p_side) const;
	String battle_column_subtitle(int p_side) const;
	String battle_stat_label(int p_row) const;
	String battle_stat_value(int p_side, int p_row) const;

	String side_name(int index) const;
	String cell_text(int index) const;
	String summary_line(int index) const;
	String combat_line(int index) const;
	String battle_header() const;
	String battle_attack_line(int p_side) const;
};

} // namespace godot

#endif // INFANTRY_MAP_VIEW_HPP
