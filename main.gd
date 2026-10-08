extends Node

# The map and the dudes are a C++ GDExtension node (see src/map_view.cpp).
# Controls:
#   middle mouse + drag - pan the map
#   left click          - step the selected dude onto a bordering province,
#                         or attack the enemy dude standing on it
#                         (clicking a dude out of reach just selects it)
#   1 / 2               - select the blue / red dude
#   arrows or WASD      - step the selected dude one province
#
# Dudes may only move to a bordering province. Stepping onto the province held
# by the other dude attacks it; when its hp reaches 0 it is destroyed and the
# attacker takes the province.

func _ready() -> void:
	var map := MapView.new()
	map.name = "MapView"
	add_child(map)

	var blue := 0
	var red := 0
	for index in map.get_province_count():
		if map.get_province_value(index) == 0:
			blue += 1
		else:
			red += 1
	print("MapView: %d countries, %d provinces (%d blue / %d red), %d dudes" % [
		map.get_country_count(), map.get_province_count(), blue, red, map.get_unit_count(),
	])
	for index in map.get_unit_count():
		print("  dude %d (value %d) on province %d,%d  hp %d/%d  attack %d" % [
			index, map.get_unit_value(index), map.get_unit_column(index), map.get_unit_row(index),
			map.get_unit_hp(index), map.get_unit_max_hp(index), map.get_unit_attack(index),
		])
