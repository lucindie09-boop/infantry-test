#include "unit_model.hpp"

#include <cmath>

namespace infantry {

UnitModel::UnitModel() {
}

void UnitModel::reset(const MapModel &map) {
	columns_ = map.get_columns();
	rows_ = map.get_rows();
	tile_size_ = map.get_province_size();
	margin_ = map.get_margin();

	const int grid = map.get_grid();
	const int middle = grid > 1 ? grid / 2 - 1 : 0;

	units_.clear();
	units_.reserve(static_cast<size_t>(map.get_country_count()));

	for (int value = 0; value < map.get_country_count(); ++value) {
		Unit unit;
		unit.value = value;
		unit.column = value * grid + middle;
		unit.row = middle;
		unit.max_hp = DEFAULT_MAX_HP;
		unit.hp = DEFAULT_MAX_HP;
		unit.attack = DEFAULT_ATTACK;
		unit.alive = true;

		// The first two countries share a border, so their dudes start on the
		// inner edge of their own square, one step away from each other.
		if (value == 0) {
			unit.column = grid - 1;
		} else if (value == 1) {
			unit.column = grid;
		}

		units_.push_back(unit);
	}

	selected_ = 0;
}

bool UnitModel::is_valid(int index) const {
	return index >= 0 && index < get_count();
}

bool UnitModel::is_alive(int index) const {
	return is_valid(index) && units_[static_cast<size_t>(index)].alive;
}

void UnitModel::set_selected(int index) {
	if (is_alive(index)) {
		selected_ = index;
	}
}

void UnitModel::select_next_alive() {
	for (int index = 0; index < get_count(); ++index) {
		if (units_[static_cast<size_t>(index)].alive) {
			selected_ = index;
			return;
		}
	}
}

float UnitModel::get_center_x(int index) const {
	return margin_ + (static_cast<float>(get(index).column) + 0.5f) * tile_size_;
}

float UnitModel::get_center_y(int index) const {
	return margin_ + (static_cast<float>(get(index).row) + 0.5f) * tile_size_;
}

int UnitModel::column_at(float x) const {
	if (tile_size_ <= 0.0f) {
		return -1;
	}
	return static_cast<int>(std::floor((x - margin_) / tile_size_));
}

int UnitModel::row_at(float y) const {
	if (tile_size_ <= 0.0f) {
		return -1;
	}
	return static_cast<int>(std::floor((y - margin_) / tile_size_));
}

bool UnitModel::is_inside(int column, int row) const {
	return column >= 0 && column < columns_ && row >= 0 && row < rows_;
}

bool UnitModel::is_bordering(int p_from_column, int p_from_row, int p_to_column, int p_to_row) const {
	const int dcol = std::abs(p_to_column - p_from_column);
	const int drow = std::abs(p_to_row - p_from_row);
	return dcol + drow == 1;
}

int UnitModel::unit_at(int column, int row) const {
	for (int index = 0; index < get_count(); ++index) {
		const Unit &unit = units_[static_cast<size_t>(index)];
		if (unit.alive && unit.column == column && unit.row == row) {
			return index;
		}
	}
	return -1;
}

MoveOutcome UnitModel::try_move(int index, int column, int row) {
	if (!is_valid(index)) {
		return MOVE_UNAVAILABLE;
	}

	Unit &mover = units_[static_cast<size_t>(index)];
	if (!mover.alive) {
		return MOVE_UNAVAILABLE;
	}
	if (!is_inside(column, row)) {
		return MOVE_OFF_MAP;
	}
	if (!is_bordering(mover.column, mover.row, column, row)) {
		return MOVE_NOT_ADJACENT;
	}

	const int target = unit_at(column, row);
	if (target != -1 && target != index) {
		Unit &defender = units_[static_cast<size_t>(target)];
		defender.hp -= mover.attack;
		if (defender.hp <= 0) {
			defender.hp = 0;
			defender.alive = false;
			mover.column = column;
			mover.row = row;
			select_next_alive();
			return MOVE_DESTROYED;
		}
		return MOVE_ATTACKED;
	}

	mover.column = column;
	mover.row = row;
	return MOVE_OK;
}

MoveOutcome UnitModel::try_move_by(int index, int dcol, int drow) {
	if (!is_alive(index)) {
		return MOVE_UNAVAILABLE;
	}
	const Unit &unit = units_[static_cast<size_t>(index)];
	return try_move(index, unit.column + dcol, unit.row + drow);
}

} // namespace infantry
