#pragma once

#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"

class MapObject;
class MapEditContext;

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
	void thingQuickAngle(Vec2f mouse_pos) const;

	// Copy / Paste
	void copy() const;
	void paste(Vec2f mouse_pos) const;
	void copyProperties(MapObject* object = nullptr);
	void pasteProperties();

	// Create / Delete
	void createObject(Vec2f pos) const;
	void createVertex(Vec2f pos) const;
	void createThing(Vec2f pos) const;
	void createSector(Vec2f pos) const;
	void deleteObject() const;
	void deleteVertex() const;
	void deleteLine() const;
	void deleteThing() const;
	void deleteSector() const;

private:
	MapEditContext& context_;

	// Object properties and copy/paste
	MapThing  copy_thing_;
	MapSector copy_sector_;
	MapSide   copy_side_front_;
	MapSide   copy_side_back_;
	MapLine   copy_line_;
	bool      line_copied_   = false;
	bool      sector_copied_ = false;
	bool      thing_copied_  = false;
};
