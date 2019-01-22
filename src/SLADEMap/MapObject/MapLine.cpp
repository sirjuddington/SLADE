
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapLine.cpp
// Description: MapLine class, represents a line object in a map
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "MapLine.h"
#include "MapSide.h"
#include "MapVertex.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/MathStuff.h"
#include "Utility/Parser.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
const string MapLine::PROP_V1      = "v1";
const string MapLine::PROP_V2      = "v2";
const string MapLine::PROP_S1      = "sidefront";
const string MapLine::PROP_S2      = "sideback";
const string MapLine::PROP_SPECIAL = "special";
const string MapLine::PROP_ID      = "id";


// -----------------------------------------------------------------------------
//
// MapLine Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapLine class constructor
// -----------------------------------------------------------------------------
MapLine::MapLine(MapVertex* v1, MapVertex* v2, MapSide* s1, MapSide* s2, int special) :
	MapObject(Type::Line),
	vertex1_{ v1 },
	vertex2_{ v2 },
	side1_{ s1 },
	side2_{ s2 },
	special_{ special }
{
	// Connect to vertices
	if (v1)
		v1->connectLine(this);
	if (v2)
		v2->connectLine(this);

	// Connect to sides
	if (s1)
		s1->parent_ = this;
	if (s2)
		s2->parent_ = this;
}

// -----------------------------------------------------------------------------
// MapLine class constructor from UDMF definition
// -----------------------------------------------------------------------------
MapLine::MapLine(MapVertex* v1, MapVertex* v2, MapSide* s1, MapSide* s2, ParseTreeNode* udmf_def) :
	MapObject(Type::Line),
	vertex1_{ v1 },
	vertex2_{ v2 },
	side1_{ s1 },
	side2_{ s2 }
{
	// Connect to vertices
	if (v1)
		v1->connectLine(this);
	if (v2)
		v2->connectLine(this);

	// Connect to sides
	if (s1)
		s1->parent_ = this;
	if (s2)
		s2->parent_ = this;

	// Set properties from UDMF definition
	ParseTreeNode* prop;
	for (unsigned a = 0; a < udmf_def->nChildren(); a++)
	{
		prop = udmf_def->childPTN(a);

		// Skip required properties
		if (prop->nameIsCI(PROP_V1) || prop->nameIsCI(PROP_V2) || prop->nameIsCI(PROP_S1) || prop->nameIsCI(PROP_S2))
			continue;

		if (prop->nameIsCI(PROP_SPECIAL))
			special_ = prop->intValue();
		else if (prop->nameIsCI(PROP_ID))
			line_id_ = prop->intValue();
		else
			properties_[prop->nameRef()] = prop->value();
	}
}

// -----------------------------------------------------------------------------
// Returns the sector on the front side of the line (if any)
// -----------------------------------------------------------------------------
MapSector* MapLine::frontSector() const
{
	return side1_ ? side1_->sector_ : nullptr;
}

// -----------------------------------------------------------------------------
// Returns the sector on the back side of the line (if any)
// -----------------------------------------------------------------------------
MapSector* MapLine::backSector() const
{
	return side2_ ? side2_->sector_ : nullptr;
}

// -----------------------------------------------------------------------------
// Returns the x coordinate of the first vertex
// -----------------------------------------------------------------------------
double MapLine::x1() const
{
	return vertex1_->xPos();
}

// -----------------------------------------------------------------------------
// Returns the y coordinate of the first vertex
// -----------------------------------------------------------------------------
double MapLine::y1() const
{
	return vertex1_->yPos();
}

// -----------------------------------------------------------------------------
// Returns the x coordinate of the second vertex
// -----------------------------------------------------------------------------
double MapLine::x2() const
{
	return vertex2_->xPos();
}

// -----------------------------------------------------------------------------
// Returns the y coordinate of the second vertex
// -----------------------------------------------------------------------------
double MapLine::y2() const
{
	return vertex2_->yPos();
}

// -----------------------------------------------------------------------------
// Returns the index of the first vertex, or -1 if none
// -----------------------------------------------------------------------------
int MapLine::v1Index() const
{
	return vertex1_ ? vertex1_->index() : -1;
}

// -----------------------------------------------------------------------------
// Returns the index of the second vertex, or -1 if none
// -----------------------------------------------------------------------------
int MapLine::v2Index() const
{
	return vertex2_ ? vertex2_->index() : -1;
}

// -----------------------------------------------------------------------------
// Returns the index of the front side, or -1 if none
// -----------------------------------------------------------------------------
int MapLine::s1Index() const
{
	return side1_ ? side1_->index() : -1;
}

// -----------------------------------------------------------------------------
// Returns the index of the back side, or -1 if none
// -----------------------------------------------------------------------------
int MapLine::s2Index() const
{
	return side2_ ? side2_->index() : -1;
}

// -----------------------------------------------------------------------------
// Returns the value of the boolean property matching [key].
// Can be prefixed with 'side1.' or 'side2.' to get bool properties from the
// front and back sides respectively
// -----------------------------------------------------------------------------
bool MapLine::boolProperty(const string& key)
{
	if (key.StartsWith("side1.") && side1_)
		return side1_->boolProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2_)
		return side2_->boolProperty(key.Mid(6));
	else
		return MapObject::boolProperty(key);
}

// -----------------------------------------------------------------------------
// Returns the value of the integer property matching [key].
// Can be prefixed with 'side1.' or 'side2.' to get int properties from the
// front and back sides respectively
// -----------------------------------------------------------------------------
int MapLine::intProperty(const string& key)
{
	if (key.StartsWith("side1.") && side1_)
		return side1_->intProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2_)
		return side2_->intProperty(key.Mid(6));
	else if (key == PROP_V1)
		return v1Index();
	else if (key == PROP_V2)
		return v2Index();
	else if (key == PROP_S1)
		return s1Index();
	else if (key == PROP_S2)
		return s2Index();
	else if (key == PROP_SPECIAL)
		return special_;
	else if (key == PROP_ID)
		return line_id_;
	else
		return MapObject::intProperty(key);
}

// -----------------------------------------------------------------------------
// Returns the value of the float property matching [key].
// Can be prefixed with 'side1.' or 'side2.' to get float properties from the
// front and back sides respectively
// -----------------------------------------------------------------------------
double MapLine::floatProperty(const string& key)
{
	if (key.StartsWith("side1.") && side1_)
		return side1_->floatProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2_)
		return side2_->floatProperty(key.Mid(6));
	else
		return MapObject::floatProperty(key);
}

// -----------------------------------------------------------------------------
// Returns the value of the string property matching [key].
// Can be prefixed with 'side1.' or 'side2.' to get string properties from the
// front and back sides respectively
// -----------------------------------------------------------------------------
string MapLine::stringProperty(const string& key)
{
	if (key.StartsWith("side1.") && side1_)
		return side1_->stringProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2_)
		return side2_->stringProperty(key.Mid(6));
	else
		return MapObject::stringProperty(key);
}

// -----------------------------------------------------------------------------
// Sets the boolean value of the property [key] to [value].
// Can be prefixed with 'side1.' or 'side2.' to set bool properties on the front
// and back sides respectively.
// -----------------------------------------------------------------------------
void MapLine::setBoolProperty(const string& key, bool value)
{
	// Front side property
	if (key.StartsWith("side1."))
	{
		if (side1_)
			return side1_->setBoolProperty(key.Mid(6), value);
	}

	// Back side property
	else if (key.StartsWith("side2."))
	{
		if (side2_)
			return side2_->setBoolProperty(key.Mid(6), value);
	}

	// Line property
	else
		MapObject::setBoolProperty(key, value);
}

// -----------------------------------------------------------------------------
// Sets the integer value of the property [key] to [value].
// Can be prefixed with 'side1.' or 'side2.' to set int properties on the front
// and back sides respectively.
// -----------------------------------------------------------------------------
void MapLine::setIntProperty(const string& key, int value)
{
	// Front side property
	if (key.StartsWith("side1."))
	{
		if (side1_)
			side1_->setIntProperty(key.Mid(6), value);
		return;
	}

	// Back side property
	else if (key.StartsWith("side2."))
	{
		if (side2_)
			side2_->setIntProperty(key.Mid(6), value);
		return;
	}

	// Mark as modified only if a line prop, not a side prop, is changing
	setModified();

	// Vertices
	if (key == PROP_V1)
	{
		MapVertex* vertex;
		if ((vertex = parent_map_->vertex(value)))
		{
			vertex1_->disconnectLine(this);
			vertex1_ = vertex;
			vertex1_->connectLine(this);
			resetInternals();
		}
	}
	else if (key == PROP_V2)
	{
		MapVertex* vertex;
		if ((vertex = parent_map_->vertex(value)))
		{
			vertex2_->disconnectLine(this);
			vertex2_ = vertex;
			vertex2_->connectLine(this);
			resetInternals();
		}
	}

	// Sides
	else if (key == PROP_S1)
	{
		auto side = parent_map_->side(value);
		if (side)
			parent_map_->setLineSide(this, side, true);
	}
	else if (key == PROP_S2)
	{
		auto side = parent_map_->side(value);
		if (side)
			parent_map_->setLineSide(this, side, false);
	}

	// Special
	else if (key == PROP_SPECIAL)
		special_ = value;

	// Id
	else if (key == PROP_ID)
		line_id_ = value;

	// Line property
	else
		MapObject::setIntProperty(key, value);
}

// -----------------------------------------------------------------------------
// Sets the float value of the property [key] to [value].
// Can be prefixed with 'side1.' or 'side2.' to set float properties on the
// front and back sides respectively.
// -----------------------------------------------------------------------------
void MapLine::setFloatProperty(const string& key, double value)
{
	// Front side property
	if (key.StartsWith("side1."))
	{
		if (side1_)
			return side1_->setFloatProperty(key.Mid(6), value);
	}

	// Back side property
	else if (key.StartsWith("side2."))
	{
		if (side2_)
			return side2_->setFloatProperty(key.Mid(6), value);
	}

	// Line property
	else
		MapObject::setFloatProperty(key, value);
}

// -----------------------------------------------------------------------------
// Sets the string value of the property [key] to [value].
// Can be prefixed with 'side1.' or 'side2.' to set string properties on the
// front and back sides respectively.
// -----------------------------------------------------------------------------
void MapLine::setStringProperty(const string& key, const string& value)
{
	// Front side property
	if (key.StartsWith("side1."))
	{
		if (side1_)
			return side1_->setStringProperty(key.Mid(6), value);
	}

	// Back side property
	else if (key.StartsWith("side2."))
	{
		if (side2_)
			return side2_->setStringProperty(key.Mid(6), value);
	}

	// Line property
	else
		MapObject::setStringProperty(key, value);
}

// -----------------------------------------------------------------------------
// Returns true if the property [key] can be modified via script
// -----------------------------------------------------------------------------
bool MapLine::scriptCanModifyProp(const string& key)
{
	return !(key == PROP_V1 || key == PROP_V2 || key == PROP_S1 || key == PROP_S2);
}

// -----------------------------------------------------------------------------
// Sets the front side of the line to [side]
// -----------------------------------------------------------------------------
void MapLine::setS1(MapSide* side)
{
	if (!side1_ && parent_map_)
		parent_map_->setLineSide(this, side, true);
}

// -----------------------------------------------------------------------------
// Sets the back side of the line to [side]
// -----------------------------------------------------------------------------
void MapLine::setS2(MapSide* side)
{
	if (!side2_ && parent_map_)
		parent_map_->setLineSide(this, side, false);
}

// -----------------------------------------------------------------------------
// Sets the start vertex of the line to [vertex]
// -----------------------------------------------------------------------------
void MapLine::setV1(MapVertex* vertex)
{
	if (!vertex)
		return;

	setModified();
	vertex1_->disconnectLine(this);
	vertex1_ = vertex;
	vertex1_->connectLine(this);
	resetInternals();
}

// -----------------------------------------------------------------------------
// Sets the end vertex of the line to [vertex]
// -----------------------------------------------------------------------------
void MapLine::setV2(MapVertex* vertex)
{
	if (!vertex)
		return;

	setModified();
	vertex2_->disconnectLine(this);
	vertex2_ = vertex;
	vertex2_->connectLine(this);
	resetInternals();
}

// -----------------------------------------------------------------------------
// Returns the object point [point].
// Currently for lines this is always the mid point
// -----------------------------------------------------------------------------
Vec2f MapLine::getPoint(Point point)
{
	// if (point == MOBJ_POINT_MID || point == MOBJ_POINT_WITHIN)
	return point1() + (point2() - point1()) * 0.5;
}

// -----------------------------------------------------------------------------
// Returns the point at the first vertex.
// -----------------------------------------------------------------------------
Vec2f MapLine::point1() const
{
	return vertex1_->position();
}

// -----------------------------------------------------------------------------
// Returns the point at the second vertex.
// -----------------------------------------------------------------------------
Vec2f MapLine::point2() const
{
	return vertex2_->position();
}

// -----------------------------------------------------------------------------
// Returns this line as a segment.
// -----------------------------------------------------------------------------
Seg2f MapLine::seg() const
{
	return { vertex1_->position(), vertex2_->position() };
}

// -----------------------------------------------------------------------------
// Returns the length of the line
// -----------------------------------------------------------------------------
double MapLine::length()
{
	if (!vertex1_ || !vertex2_)
		return -1;

	if (length_ < 0)
	{
		length_ = this->seg().length();
		ca_     = (vertex2_->xPos() - vertex1_->xPos()) / length_;
		sa_     = (vertex2_->yPos() - vertex1_->yPos()) / length_;
	}

	return length_;
}

// -----------------------------------------------------------------------------
// Returns true if the line has the same sector on both sides
// -----------------------------------------------------------------------------
bool MapLine::doubleSector() const
{
	// Check both sides exist
	if (!side1_ || !side2_)
		return false;

	return (side1_->sector() == side2_->sector());
}

// -----------------------------------------------------------------------------
// Returns the vector perpendicular to the front side of the line
// -----------------------------------------------------------------------------
Vec2f MapLine::frontVector()
{
	// Check if vector needs to be recalculated
	if (front_vec_.x == 0 && front_vec_.y == 0)
	{
		front_vec_.set(-(vertex2_->yPos() - vertex1_->yPos()), vertex2_->xPos() - vertex1_->xPos());
		front_vec_.normalize();
	}

	return front_vec_;
}

// -----------------------------------------------------------------------------
// Calculates and returns the end point of the 'direction tab' for the line
// (used as a front side indicator for 2d map display)
// -----------------------------------------------------------------------------
Vec2f MapLine::dirTabPoint(double tab_length)
{
	// Calculate midpoint
	Vec2f mid(x1() + ((x2() - x1()) * 0.5), y1() + ((y2() - y1()) * 0.5));

	// Calculate tab length
	if (tab_length == 0)
	{
		tab_length = length() * 0.1;
		if (tab_length > 16)
			tab_length = 16;
		if (tab_length < 2)
			tab_length = 2;
	}

	// Calculate tab endpoint
	if (front_vec_.x == 0 && front_vec_.y == 0)
		frontVector();
	return { mid.x - front_vec_.x * tab_length, mid.y - front_vec_.y * tab_length };
}

// -----------------------------------------------------------------------------
// Returns the minimum distance from the point to the line
// -----------------------------------------------------------------------------
double MapLine::distanceTo(Vec2f point)
{
	// Update length data if needed
	if (length_ < 0)
	{
		length_ = this->seg().length();
		if (length_ != 0)
		{
			ca_ = (vertex2_->xPos() - vertex1_->xPos()) / length_;
			sa_ = (vertex2_->yPos() - vertex1_->yPos()) / length_;
		}
	}

	// Calculate intersection point
	double mx, ix, iy;
	mx = (-vertex1_->xPos() + point.x) * ca_ + (-vertex1_->yPos() + point.y) * sa_;
	if (mx <= 0)
		mx = 0.00001; // Clip intersection to line (but not exactly on endpoints)
	else if (mx >= length_)
		mx = length_ - 0.00001; // ^^
	ix = vertex1_->xPos() + mx * ca_;
	iy = vertex1_->yPos() + mx * sa_;

	// Calculate distance to line
	return sqrt((ix - point.x) * (ix - point.x) + (iy - point.y) * (iy - point.y));
}

// Minimum gap between planes for a texture to be considered missing
static const float EPSILON = 0.001f;

// -----------------------------------------------------------------------------
// Returns a flag set of any parts of the line that require a texture
// -----------------------------------------------------------------------------
int MapLine::needsTexture() const
{
	// Check line is valid
	if (!frontSector())
		return 0;

	// If line is 1-sided, it only needs front middle
	if (!backSector())
		return Part::FrontMiddle;

	// Get sector planes
	auto floor_front   = frontSector()->floor().plane;
	auto ceiling_front = frontSector()->ceiling().plane;
	auto floor_back    = backSector()->floor().plane;
	auto ceiling_back  = backSector()->ceiling().plane;

	double front_height, back_height;

	int tex = 0;

	// Check the floor
	front_height = floor_front.heightAt(x1(), y1());
	back_height  = floor_back.heightAt(x1(), y1());

	if (front_height - back_height > EPSILON)
		tex |= Part::BackLower;
	if (back_height - front_height > EPSILON)
		tex |= Part::FrontLower;

	front_height = floor_front.heightAt(x2(), y2());
	back_height  = floor_back.heightAt(x2(), y2());

	if (front_height - back_height > EPSILON)
		tex |= Part::BackLower;
	if (back_height - front_height > EPSILON)
		tex |= Part::FrontLower;

	// Check the ceiling
	front_height = ceiling_front.heightAt(x1(), y1());
	back_height  = ceiling_back.heightAt(x1(), y1());

	if (back_height - front_height > EPSILON)
		tex |= Part::BackUpper;
	if (front_height - back_height > EPSILON)
		tex |= Part::FrontUpper;

	front_height = ceiling_front.heightAt(x2(), y2());
	back_height  = ceiling_back.heightAt(x2(), y2());

	if (back_height - front_height > EPSILON)
		tex |= Part::BackUpper;
	if (front_height - back_height > EPSILON)
		tex |= Part::FrontUpper;

	return tex;
}

// -----------------------------------------------------------------------------
// Returns true if this line overlaps with [other]
// (ie. both lines share the same vertices)
// -----------------------------------------------------------------------------
bool MapLine::overlaps(MapLine* other) const
{
	return other != this
		   && (vertex1_ == other->vertex1_ && vertex2_ == other->vertex2_
			   || vertex2_ == other->vertex1_ && vertex1_ == other->vertex2_);
}

// -----------------------------------------------------------------------------
// Returns true if this line intersects with [other].
// If an intersection occurs, [intersect_point] is set to the intersection point
// -----------------------------------------------------------------------------
bool MapLine::intersects(MapLine* other, Vec2f& intersect_point) const
{
	return MathStuff::linesIntersect(seg(), other->seg(), intersect_point);
}

// -----------------------------------------------------------------------------
// Clears any textures not needed on the line
// (eg. a front upper texture that would be invisible)
// -----------------------------------------------------------------------------
void MapLine::clearUnneededTextures()
{
	// Check needed textures
	int tex = needsTexture();

	// Clear any unneeded textures
	if (side1_)
	{
		if ((tex & Part::FrontMiddle) == 0)
			setStringProperty("side1.texturemiddle", "-");
		if ((tex & Part::FrontUpper) == 0)
			setStringProperty("side1.texturetop", "-");
		if ((tex & Part::FrontLower) == 0)
			setStringProperty("side1.texturebottom", "-");
	}
	if (side2_)
	{
		if ((tex & Part::BackMiddle) == 0)
			setStringProperty("side2.texturemiddle", "-");
		if ((tex & Part::BackUpper) == 0)
			setStringProperty("side2.texturetop", "-");
		if ((tex & Part::BackLower) == 0)
			setStringProperty("side2.texturebottom", "-");
	}
}

// -----------------------------------------------------------------------------
// Resets all calculated internal values for the line and sectors
// -----------------------------------------------------------------------------
void MapLine::resetInternals()
{
	// Reset line internals
	length_ = -1;
	front_vec_.set(0, 0);

	// Reset front sector internals
	auto s1 = frontSector();
	if (s1)
	{
		s1->resetPolygon();
		s1->resetBBox();
	}

	// Reset back sector internals
	auto s2 = backSector();
	if (s2)
	{
		s2->resetPolygon();
		s2->resetBBox();
	}
}

// -----------------------------------------------------------------------------
// Flips the line. If [sides] is true front and back sides are also swapped
// -----------------------------------------------------------------------------
void MapLine::flip(bool sides)
{
	setModified();

	// Flip vertices
	auto v   = vertex1_;
	vertex1_ = vertex2_;
	vertex2_ = v;

	// Flip sides if needed
	if (sides)
	{
		auto s = side1_;
		side1_ = side2_;
		side2_ = s;
	}

	resetInternals();
	if (parent_map_)
		parent_map_->setGeometryUpdated();
}

// -----------------------------------------------------------------------------
// Write all line info to a Backup struct
// -----------------------------------------------------------------------------
void MapLine::writeBackup(Backup* backup)
{
	// Vertices
	backup->props_internal[PROP_V1] = vertex1_->objId();
	backup->props_internal[PROP_V2] = vertex2_->objId();

	// Sides
	if (side1_)
		backup->props_internal["s1"] = side1_->objId();
	else
		backup->props_internal["s1"] = 0;
	if (side2_)
		backup->props_internal["s2"] = side2_->objId();
	else
		backup->props_internal["s2"] = 0;

	// Special
	backup->props_internal[PROP_SPECIAL] = special_;
	backup->props_internal[PROP_ID]      = line_id_;
}

// -----------------------------------------------------------------------------
// Reads all line info from a Backup struct
// -----------------------------------------------------------------------------
void MapLine::readBackup(Backup* backup)
{
	// Vertices
	auto v1 = parent_map_->mapData().getObjectById(backup->props_internal[PROP_V1]);
	auto v2 = parent_map_->mapData().getObjectById(backup->props_internal[PROP_V2]);
	if (v1)
	{
		vertex1_->disconnectLine(this);
		vertex1_ = dynamic_cast<MapVertex*>(v1);
		vertex1_->connectLine(this);
		resetInternals();
	}
	if (v2)
	{
		vertex2_->disconnectLine(this);
		vertex2_ = dynamic_cast<MapVertex*>(v2);
		vertex2_->connectLine(this);
		resetInternals();
	}

	// Sides
	auto s1 = parent_map_->mapData().getObjectById(backup->props_internal["s1"]);
	auto s2 = parent_map_->mapData().getObjectById(backup->props_internal["s2"]);
	side1_  = dynamic_cast<MapSide*>(s1);
	side2_  = dynamic_cast<MapSide*>(s2);
	if (side1_)
		side1_->parent_ = this;
	if (side2_)
		side2_->parent_ = this;

	// Special
	special_ = backup->props_internal[PROP_SPECIAL];
	line_id_ = backup->props_internal[PROP_ID];
}

// -----------------------------------------------------------------------------
// Copies another map object [c]
// -----------------------------------------------------------------------------
void MapLine::copy(MapObject* c)
{
	if (objType() != c->objType())
		return;

	MapObject::copy(c);

	auto l = dynamic_cast<MapLine*>(c);

	if (side1_ && l->side1_)
		side1_->copy(l->side1_);

	if (side2_ && l->side2_)
		side2_->copy(l->side2_);

	special_ = l->special_;
	line_id_ = l->line_id_;
}

// -----------------------------------------------------------------------------
// Writes the line as a UDMF text definition to [def]
// -----------------------------------------------------------------------------
void MapLine::writeUDMF(string& def)
{
	def = S_FMT("linedef//#%u\n{\n", index_);

	// Basic properties
	def += S_FMT("v1=%d;\nv2=%d;\nsidefront=%d;\n", v1Index(), v2Index(), s1Index());
	if (s2())
		def += S_FMT("sideback=%d;\n", s2Index());
	if (special_ != 0)
		def += S_FMT("special=%d;\n", special_);
	if (line_id_ != 0)
		def += S_FMT("id=%d;\n", line_id_);

	// Other properties
	if (!properties_.isEmpty())
		def += properties_.toString(true);

	def += "}\n\n";
}
