#ifndef INFANTRY_MAP_MODEL_HPP
#define INFANTRY_MAP_MODEL_HPP

#include <vector>

namespace infantry {

// Ownership value stored on every province.
enum ProvinceValue {
	BLUE = 0, // the Blue country
	RED = 1,  // the Red country
	VALUE_NONE = -1,
};

// One tile of a country's province layer. Provinces are squares in map space.
struct Province {
	int value = VALUE_NONE; // 0 = Blue country, 1 = Red country
	int gx = 0;             // column inside the country's own grid
	int gy = 0;             // row inside the country's own grid
	float x = 0.0f;         // top-left corner in map space
	float y = 0.0f;
	float size = 0.0f; // width == height
};

// A square country subdivided into a grid x grid layer of provinces.
struct Country {
	int id = VALUE_NONE; // matches the value of every province it owns
	float x = 0.0f;
	float y = 0.0f;
	float size = 0.0f; // side length of the square, in map units
	std::vector<Province> provinces;
};

// Geometry and ownership of the whole map. Deliberately free of engine types so
// the layout rules can be exercised without booting Godot.
class MapModel {
public:
	// A gap of 0 puts the two countries right next to each other so they share a
	// border, which also makes the two squares one continuous province field.
	MapModel(int grid = 8, float province_size = 48.0f, float gap = 0.0f, float margin = 96.0f);

	// Rebuilds both countries from the configured grid and size.
	void generate();

	int get_grid() const { return grid_; }
	int get_country_count() const { return static_cast<int>(countries_.size()); }
	int get_province_count() const;
	float get_province_size() const { return province_size_; }
	float get_margin() const { return margin_; }

	// The two countries sit side by side, so together they cover this many
	// columns and rows of provinces.
	int get_columns() const { return grid_ * get_country_count(); }
	int get_rows() const { return grid_; }

	// Flat access over the map: country 0's provinces first, then country 1's.
	int get_province_value(int index) const;
	const Province *get_province(int index) const;

	// Province of the continuous field, or nullptr when the column/row falls
	// outside the map.
	const Province *province_at_grid(int column, int row) const;
	// Province under a point in map space, or nullptr outside the map.
	const Province *province_at_point(float x, float y) const;

	const std::vector<Country> &get_countries() const { return countries_; }
	const Country &get_country(int id) const { return countries_[id]; }

	// Full extent of the map including the outer margin, in map units.
	float get_width() const;
	float get_height() const;

private:
	int grid_;
	float province_size_;
	float gap_;
	float margin_;
	std::vector<Country> countries_;
};

} // namespace infantry

#endif // INFANTRY_MAP_MODEL_HPP
