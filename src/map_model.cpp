#include "map_model.hpp"

#include <cmath>

namespace infantry {

MapModel::MapModel(int grid, float province_size, float gap, float margin)
		: grid_(grid), province_size_(province_size), gap_(gap), margin_(margin) {
	generate();
}

void MapModel::generate() {
	if (grid_ < 1) {
		grid_ = 1;
	}

	const float country_size = static_cast<float>(grid_) * province_size_;

	countries_.clear();
	countries_.reserve(2); // Blue on the left, Red on the right.

	for (int id = 0; id < 2; ++id) {
		Country country;
		country.id = id;
		country.size = country_size;
		country.x = margin_ + static_cast<float>(id) * (country_size + gap_);
		country.y = margin_;
		country.provinces.reserve(static_cast<size_t>(grid_) * static_cast<size_t>(grid_));

		for (int gy = 0; gy < grid_; ++gy) {
			for (int gx = 0; gx < grid_; ++gx) {
				Province province;
				province.value = id;
				province.gx = gx;
				province.gy = gy;
				province.size = province_size_;
				province.x = country.x + static_cast<float>(gx) * province_size_;
				province.y = country.y + static_cast<float>(gy) * province_size_;
				country.provinces.push_back(province);
			}
		}

		countries_.push_back(country);
	}
}

int MapModel::get_province_count() const {
	int total = 0;
	for (const Country &country : countries_) {
		total += static_cast<int>(country.provinces.size());
	}
	return total;
}

int MapModel::get_province_value(int index) const {
	const Province *province = get_province(index);
	return province != nullptr ? province->value : VALUE_NONE;
}

const Province *MapModel::get_province(int index) const {
	if (index < 0) {
		return nullptr;
	}
	for (const Country &country : countries_) {
		const int count = static_cast<int>(country.provinces.size());
		if (index < count) {
			return &country.provinces[static_cast<size_t>(index)];
		}
		index -= count;
	}
	return nullptr;
}

const Province *MapModel::province_at_grid(int column, int row) const {
	if (grid_ < 1 || column < 0 || column >= get_columns() || row < 0 || row >= grid_) {
		return nullptr;
	}

	const int country_index = column / grid_;
	if (country_index >= get_country_count()) {
		return nullptr;
	}

	// Provinces are stored row by row inside each country.
	const size_t local = static_cast<size_t>(row) * static_cast<size_t>(grid_) +
			static_cast<size_t>(column % grid_);
	return &countries_[static_cast<size_t>(country_index)].provinces[local];
}

const Province *MapModel::province_at_point(float x, float y) const {
	if (province_size_ <= 0.0f) {
		return nullptr;
	}
	const int column = static_cast<int>(std::floor((x - margin_) / province_size_));
	const int row = static_cast<int>(std::floor((y - margin_) / province_size_));
	return province_at_grid(column, row);
}

float MapModel::get_width() const {
	if (countries_.empty()) {
		return margin_ * 2.0f;
	}
	const Country &last = countries_.back();
	return last.x + last.size + margin_;
}

float MapModel::get_height() const {
	if (countries_.empty()) {
		return margin_ * 2.0f;
	}
	return countries_.front().size + margin_ * 2.0f;
}

} // namespace infantry
