#pragma once

// Forward declarations
namespace slade
{
class MapThing;
class MapSector;
class MapSide;
class MapLine;
class MapObject;

namespace mapeditor
{
	class MapEditContext;
}
} // namespace slade

namespace slade::mapeditor
{
class Edit2D
{
public:
	Edit2D(MapEditContext& context);

	// General Editing
	void mirror(bool x_axis) const;
	void editObjectProperties();

	// Lines
	void splitLine(double x, double y, double min_dist = 64) const;
	void flipLines(bool sides = true) const;
	void correctLineSectors() const;

	// Sectors
	void changeSectorHeight(int amount, bool floor = true, bool ceiling = true) const;
	void changeSectorLight(bool up, bool fine) const;
	void changeSectorTexture() const;
	void joinSectors(bool remove_lines) const;

	// Things
	void changeThingType() const;
	void thingQuickAngle(const Vec2d& mouse_pos) const;

	// Copy / Paste
	void copy() const;
	void paste(const Vec2d& mouse_pos) const;
	void copyProperties();
	void pasteProperties() const;

	// Create / Delete
	void createObject(const Vec2d& pos) const;
	void createVertex(Vec2d pos) const;
	void createThing(Vec2d pos) const;
	void createSector(const Vec2d& pos) const;
	void deleteObject() const;
	void deleteVertex() const;
	void deleteLine() const;
	void deleteThing() const;
	void deleteSector() const;

private:
	MapEditContext* context_ = nullptr;

	// Object properties and copy/paste
	unique_ptr<MapThing>  copy_thing_;
	unique_ptr<MapSector> copy_sector_;
	unique_ptr<MapSide>   copy_side_front_;
	unique_ptr<MapSide>   copy_side_back_;
	unique_ptr<MapLine>   copy_line_;
	bool                  line_copied_   = false;
	bool                  sector_copied_ = false;
	bool                  thing_copied_  = false;
};
} // namespace slade::mapeditor
