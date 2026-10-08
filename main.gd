extends Node

# The map, the dudes and the battles are a C++ GDExtension node (see
# src/map_view.cpp). Every division runs on the stat model in src/division_stats
# and the tunable numbers in src/combat_defines.hpp.
#
# Controls:
#   middle mouse + drag - pan the map
#   left click          - step the selected dude onto a bordering province, or
#                         attack the enemy standing on one
#   1 / 2               - select the blue / red dude
#   arrows or WASD      - step the selected dude one province
#
# Walking into the enemy starts a battle. Combat then runs an hour at a time:
# the attacker's hits are scaled by terrain, what the defender cannot parry with
# its defence (or breakthrough, when it is the attacker) lands at the
# undefended multiplier, and the result is split between organisation and
# strength. Whoever breaks on organisation first loses the province.

func _ready() -> void:
	var map := MapView.new()
	map.name = "MapView"
	add_child(map)

	print("MapView: %d countries, %d provinces, %d dudes" % [
		map.get_country_count(), map.get_province_count(), map.get_unit_count(),
	])
	for index in map.get_unit_count():
		print("  dude %d (value %d) on province %d,%d  [%s]" % [
			index, map.get_unit_value(index), map.get_unit_column(index), map.get_unit_row(index),
			map.get_unit_division(index),
		])
		var stats: Dictionary = map.get_unit_stats(index)
		var keys := stats.keys()
		keys.sort()
		for key in keys:
			print("      %-22s %s" % [key, stats[key]])
