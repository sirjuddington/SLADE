
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapLine.cpp
 * Description: MapLine class, represents a line object in a map
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "MapLine.h"
#include "MapSide.h"
#include "MapVertex.h"
#include "SLADEMap.h"
#include "Utility/MathStuff.h"


/*******************************************************************
 * MAPLINE CLASS FUNCTIONS
 *******************************************************************/

/* MapLine::MapLine
 * MapLine class constructor
 *******************************************************************/
MapLine::MapLine(SLADEMap* parent) : MapObject(Type::Line, parent)
{
}

/* MapLine::MapLine
 * MapLine class constructor
 *******************************************************************/
MapLine::MapLine(MapVertex* v1, MapVertex* v2, MapSide* s1, MapSide* s2, SLADEMap* parent) :
	MapObject(Type::Line, parent),
	vertex1_{ v1 },
	vertex2_{ v2 },
	side1_{ s1 },
	side2_{ s2 }
{
	// Connect to vertices
	if (v1) v1->connectLine(this);
	if (v2) v2->connectLine(this);

	// Connect to sides
	if (s1) s1->parent_line_ = this;
	if (s2) s2->parent_line_ = this;
}

/* MapLine::~MapLine
 * MapLine class destructor
 *******************************************************************/
MapLine::~MapLine()
{
}

/* MapLine::frontSector
 * Returns the sector on the front side of the line (if any)
 *******************************************************************/
MapSector* MapLine::frontSector()
{
	if (side1_)
		return side1_->sector_;
	else
		return nullptr;
}

/* MapLine::backSector
 * Returns the sector on the back side of the line (if any)
 *******************************************************************/
MapSector* MapLine::backSector()
{
	if (side2_)
		return side2_->sector_;
	else
		return nullptr;
}

/* MapLine::x1
 * Returns the x coordinate of the first vertex
 *******************************************************************/
double MapLine::x1()
{
	return vertex1_->xPos();
}

/* MapLine::y1
 * Returns the y coordinate of the first vertex
 *******************************************************************/
double MapLine::y1()
{
	return vertex1_->yPos();
}

/* MapLine::x2
 * Returns the x coordinate of the second vertex
 *******************************************************************/
double MapLine::x2()
{
	return vertex2_->xPos();
}

/* MapLine::y2
 * Returns the y coordinate of the second vertex
 *******************************************************************/
double MapLine::y2()
{
	return vertex2_->yPos();
}

/* MapLine::v1Index
 * Returns the index of the first vertex, or -1 if none
 *******************************************************************/
int MapLine::v1Index()
{
	if (vertex1_)
		return vertex1_->index();
	else
		return -1;
}

/* MapLine::v2Index
 * Returns the index of the second vertex, or -1 if none
 *******************************************************************/
int MapLine::v2Index()
{
	if (vertex2_)
		return vertex2_->index();
	else
		return -1;
}

/* MapLine::s1Index
 * Returns the index of the front side, or -1 if none
 *******************************************************************/
int MapLine::s1Index()
{
	if (side1_)
		return side1_->index();
	else
		return -1;
}

/* MapLine::s2Index
 * Returns the index of the back side, or -1 if none
 *******************************************************************/
int MapLine::s2Index()
{
	if (side2_)
		return side2_->index();
	else
		return -1;
}

/* MapLine::boolProperty
 * Returns the value of the boolean property matching [key]. Can be
 * prefixed with 'side1.' or 'side2.' to get bool properties from the
 * front and back sides respectively
 *******************************************************************/
bool MapLine::boolProperty(const string& key)
{
	if (key.StartsWith("side1.") && side1_)
		return side1_->boolProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2_)
		return side2_->boolProperty(key.Mid(6));
	else
		return MapObject::boolProperty(key);
}

/* MapLine::intProperty
 * Returns the value of the integer property matching [key]. Can be
 * prefixed with 'side1.' or 'side2.' to get int properties from the
 * front and back sides respectively
 *******************************************************************/
int MapLine::intProperty(const string& key)
{
	if (key.StartsWith("side1.") && side1_)
		return side1_->intProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2_)
		return side2_->intProperty(key.Mid(6));
	else if (key == "v1")
		return v1Index();
	else if (key == "v2")
		return v2Index();
	else if (key == "sidefront")
		return s1Index();
	else if (key == "sideback")
		return s2Index();
	else if (key == "special")
		return special_;
	else if (key == "id")
		return id_;
	else
		return MapObject::intProperty(key);
}

/* MapLine::floatProperty
 * Returns the value of the float property matching [key]. Can be
 * prefixed with 'side1.' or 'side2.' to get float properties from
 * the front and back sides respectively
 *******************************************************************/
double MapLine::floatProperty(const string& key)
{
	if (key.StartsWith("side1.") && side1_)
		return side1_->floatProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2_)
		return side2_->floatProperty(key.Mid(6));
	else
		return MapObject::floatProperty(key);
}

/* MapLine::stringProperty
 * Returns the value of the string property matching [key]. Can be
 * prefixed with 'side1.' or 'side2.' to get string properties from
 * the front and back sides respectively
 *******************************************************************/
string MapLine::stringProperty(const string& key)
{
	if (key.StartsWith("side1.") && side1_)
		return side1_->stringProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2_)
		return side2_->stringProperty(key.Mid(6));
	else
		return MapObject::stringProperty(key);
}

/* MapLine::setBoolProperty
 * Sets the boolean value of the property [key] to [value]. Can be
 * prefixed with 'side1.' or 'side2.' to set bool properties on the
 * front and back sides respectively.
 *******************************************************************/
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

/* MapLine::setIntProperty
 * Sets the integer value of the property [key] to [value]. Can be
 * prefixed with 'side1.' or 'side2.' to set int properties on the
 * front and back sides respectively.
 *******************************************************************/
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
	if (key == "v1")
	{
		MapVertex* vertex;
		if ((vertex = parent_map_->getVertex(value)))
		{
			vertex1_->disconnectLine(this);
			vertex1_ = vertex;
			vertex1_->connectLine(this);
			resetInternals();
		}
	}
	else if (key == "v2")
	{
		MapVertex* vertex;
		if ((vertex = parent_map_->getVertex(value)))
		{
			vertex2_->disconnectLine(this);
			vertex2_ = vertex;
			vertex2_->connectLine(this);
			resetInternals();
		}
	}

	// Sides
	else if (key == "sidefront")
	{
		MapSide* side = parent_map_->getSide(value);
		if (side)
			parent_map_->setLineSide(this, side, true);
	}
	else if (key == "sideback")
	{
		MapSide* side = parent_map_->getSide(value);
		if (side)
			parent_map_->setLineSide(this, side, false);
	}

	// Special
	else if (key == "special")
		special_ = value;

	// Id
	else if (key == "id")
		id_ = value;

	// Line property
	else
		MapObject::setIntProperty(key, value);
}

/* MapLine::setFloatProperty
 * Sets the float value of the property [key] to [value]. Can be
 * prefixed with 'side1.' or 'side2.' to set float properties on the
 * front and back sides respectively.
 *******************************************************************/
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

/* MapLine::setStringProperty
 * Sets the string value of the property [key] to [value]. Can be
 * prefixed with 'side1.' or 'side2.' to set string properties on the
 * front and back sides respectively.
 *******************************************************************/
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

/* MapLine::scriptCanModifyProp
 * Returns true if the property [key] can be modified via script
 *******************************************************************/
bool MapLine::scriptCanModifyProp(const string& key)
{
	if (key == "v1" ||
		key == "v2" ||
		key == "sidefront" ||
		key == "sideback")
		return false;

	return true;
}

/* MapLine::setS1
 * Sets the front side of the line to [side]
 *******************************************************************/
void MapLine::setS1(MapSide* side)
{
	if (!side1_ && parent_map_)
		parent_map_->setLineSide(this, side, true);
}

/* MapLine::setS1
 * Sets the back side of the line to [side]
 *******************************************************************/
void MapLine::setS2(MapSide* side)
{
	if (!side2_ && parent_map_)
		parent_map_->setLineSide(this, side, false);
}

/* MapLine::getPoint
 * Returns the object point [point]. Currently for lines this is
 * always the mid point
 *******************************************************************/
fpoint2_t MapLine::point(Point point)
{
	//if (point == MOBJ_POINT_MID || point == MOBJ_POINT_WITHIN)
	return point1() + (point2() - point1()) * 0.5;
}

/* MapLine::point1
 * Returns the point at the first vertex.
 *******************************************************************/
fpoint2_t MapLine::point1()
{
	return vertex1_->pos();
}

/* MapLine::point2
 * Returns the point at the second vertex.
 *******************************************************************/
fpoint2_t MapLine::point2()
{
	return vertex2_->pos();
}

/* MapLine::seg
 * Returns this line as a segment.
 *******************************************************************/
fseg2_t MapLine::seg()
{
	return fseg2_t(vertex1_->pos(), vertex2_->pos());
}

/* MapLine::getLength
 * Returns the length of the line
 *******************************************************************/
double MapLine::length()
{
	if (!vertex1_ || !vertex2_)
		return -1;

	if (length_ < 0)
	{
		length_ = this->seg().length();
		ca_ = (vertex2_->xPos() - vertex1_->xPos()) / length_;
		sa_ = (vertex2_->yPos() - vertex1_->yPos()) / length_;
	}

	return length_;
}

/* MapLine::getAngle
 * Returns the angle (in degrees) of line, east is zero, front side counter-clock wise.
 *******************************************************************/
double MapLine::angle()
{
	if (!vertex1_ || !vertex2_)
		return -1;
	double angle = (180.0 / M_PI) *  atan2(vertex1_->yPos() - vertex2_->yPos(), vertex1_->xPos() - vertex2_->xPos());
	if (angle < 0) angle += 360.0;
	return angle;
}

/* MapLine::doubleSector
 * Returns true if the line has the same sector on both sides
 *******************************************************************/
bool MapLine::doubleSector()
{
	// Check both sides exist
	if (!side1_ || !side2_)
		return false;

	return (side1_->sector() == side2_->sector());
}

/* MapLine::frontVector
 * Returns the vector perpendicular to the front side of the line
 *******************************************************************/
fpoint2_t MapLine::frontVector()
{
	// Check if vector needs to be recalculated
	if (front_vec_.x == 0 && front_vec_.y == 0)
	{
		front_vec_.set(-(vertex2_->yPos() - vertex1_->yPos()), vertex2_->xPos() - vertex1_->xPos());
		front_vec_.normalize();
	}

	return front_vec_;
}

/* MapLine::dirTabPoint
 * Calculates and returns the end point of the 'direction tab' for
 * the line (used as a front side indicator for 2d map display)
 *******************************************************************/
fpoint2_t MapLine::dirTabPoint(double tablen)
{
	// Calculate midpoint
	fpoint2_t mid(x1() + ((x2() - x1()) * 0.5), y1() + ((y2() - y1()) * 0.5));

	// Calculate tab length
	if (tablen == 0)
	{
		tablen = length() * 0.1;
		if (tablen > 16) tablen = 16;
		if (tablen < 2) tablen = 2;
	}

	// Calculate tab endpoint
	if (front_vec_.x == 0 && front_vec_.y == 0) frontVector();
	return fpoint2_t(mid.x - front_vec_.x*tablen, mid.y - front_vec_.y*tablen);
}

/* MapLine::distanceTo
 * Returns the minimum distance from the point to the line
 *******************************************************************/
double MapLine::distanceTo(fpoint2_t point)
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
	mx = (-vertex1_->xPos()+point.x)*ca_ + (-vertex1_->yPos()+point.y)*sa_;
	if (mx <= 0)		mx = 0.00001;				// Clip intersection to line (but not exactly on endpoints)
	else if (mx >= length_)	mx = length_ - 0.00001;	// ^^
	ix = vertex1_->xPos() + mx*ca_;
	iy = vertex1_->yPos() + mx*sa_;

	// Calculate distance to line
	return sqrt((ix-point.x)*(ix-point.x) + (iy-point.y)*(iy-point.y));
}

// Minimum gap between planes for a texture to be considered missing
static const float EPSILON = 0.001f;

/* MapLine::needsTexture
 * Returns a flag set of any parts of the line that require a texture
 *******************************************************************/
int MapLine::needsTexture()
{
	// Check line is valid
	if (!frontSector())
		return 0;

	// If line is 1-sided, it only needs front middle
	if (!backSector())
		return FrontMiddle;

	// Get sector planes
	plane_t floor_front = frontSector()->planeFloor();
	plane_t ceiling_front = frontSector()->planeCeiling();
	plane_t floor_back = backSector()->planeFloor();
	plane_t ceiling_back = backSector()->planeCeiling();

	int tex = 0;

	// Check the floor
	double front_height = floor_front.height_at(x1(), y1());
	double back_height = floor_back.height_at(x1(), y1());

	if (front_height - back_height > EPSILON)
		tex |= BackLower;
	if (back_height - front_height > EPSILON)
		tex |= FrontLower;

	front_height = floor_front.height_at(x2(), y2());
	back_height = floor_back.height_at(x2(), y2());

	if (front_height - back_height > EPSILON)
		tex |= BackLower;
	if (back_height - front_height > EPSILON)
		tex |= FrontLower;

	// Check the ceiling
	front_height = ceiling_front.height_at(x1(), y1());
	back_height = ceiling_back.height_at(x1(), y1());

	if (back_height - front_height > EPSILON)
		tex |= BackUpper;
	if (front_height - back_height > EPSILON)
		tex |= FrontUpper;

	front_height = ceiling_front.height_at(x2(), y2());
	back_height = ceiling_back.height_at(x2(), y2());

	if (back_height - front_height > EPSILON)
		tex |= BackUpper;
	if (front_height - back_height > EPSILON)
		tex |= FrontUpper;

	return tex;
}

void MapLine::disconnectFromVertices()
{
	if (vertex1_)
		vertex1_->disconnectLine(this);
	if (vertex2_)
		vertex2_->disconnectLine(this);
}

/* MapLine::clearUnneededTextures
 * Clears any textures not needed on the line (eg. a front upper
 * texture that would be invisible)
 *******************************************************************/
void MapLine::clearUnneededTextures()
{
	// Check needed textures
	int tex = needsTexture();

	// Clear any unneeded textures
	if (side1_)
	{
		if ((tex & FrontMiddle) == 0)
			setStringProperty("side1.texturemiddle", "-");
		if ((tex & FrontUpper) == 0)
			setStringProperty("side1.texturetop", "-");
		if ((tex & FrontLower) == 0)
			setStringProperty("side1.texturebottom", "-");
	}
	if (side2_)
	{
		if ((tex & BackMiddle) == 0)
			setStringProperty("side2.texturemiddle", "-");
		if ((tex & BackUpper) == 0)
			setStringProperty("side2.texturetop", "-");
		if ((tex & BackLower) == 0)
			setStringProperty("side2.texturebottom", "-");
	}
}

/* MapLine::resetInternals
 * Resets all calculated internal values for the line and sectors
 *******************************************************************/
void MapLine::resetInternals()
{
	// Reset line internals
	length_ = -1;
	front_vec_.set(0, 0);

	// Reset front sector internals
	MapSector* s1 = frontSector();
	if (s1)
	{
		s1->resetPolygon();
		s1->resetBBox();
	}

	// Reset back sector internals
	MapSector* s2 = backSector();
	if (s2)
	{
		s2->resetPolygon();
		s2->resetBBox();
	}
}

/* MapLine::flip
 * Flips the line. If [sides] is true front and back sides are also
 * swapped
 *******************************************************************/
void MapLine::flip(bool sides)
{
	setModified();

	// Flip vertices
	MapVertex* v = vertex1_;
	vertex1_ = vertex2_;
	vertex2_ = v;

	// Flip sides if needed
	if (sides)
	{
		MapSide* s = side1_;
		side1_ = side2_;
		side2_ = s;
	}

	resetInternals();
	if (parent_map_)
		parent_map_->setGeometryUpdated();
}

/* MapLine::writeBackup
 * Write all line info to a mobj_backup_t struct
 *******************************************************************/
void MapLine::writeBackup(Backup* backup)
{
	// Vertices
	backup->props_internal["v1"] = vertex1_->objId();
	backup->props_internal["v2"] = vertex2_->objId();

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
	backup->props_internal["special"] = special_;
	backup->props_internal["id"] = id_;
}

/* MapLine::readBackup
 * Reads all line info from a mobj_backup_t struct
 *******************************************************************/
void MapLine::readBackup(Backup* backup)
{
	// Vertices
	MapObject* v1 = parent_map_->getObjectById(backup->props_internal["v1"]);
	MapObject* v2 = parent_map_->getObjectById(backup->props_internal["v2"]);
	if (v1)
	{
		vertex1_->disconnectLine(this);
		vertex1_ = (MapVertex*)v1;
		vertex1_->connectLine(this);
		resetInternals();
	}
	if (v2)
	{
		vertex2_->disconnectLine(this);
		vertex2_ = (MapVertex*)v2;
		vertex2_->connectLine(this);
		resetInternals();
	}

	// Sides
	MapObject* s1 = parent_map_->getObjectById(backup->props_internal["s1"]);
	MapObject* s2 = parent_map_->getObjectById(backup->props_internal["s2"]);
	side1_ = (MapSide*)s1;
	side2_ = (MapSide*)s2;
	if (side1_) side1_->parent_line_ = this;
	if (side2_) side2_->parent_line_ = this;

	// Special
	special_ = backup->props_internal["special"];
	id_ = backup->props_internal["id"];
}

/* MapLine::copy
 * Copies another map object [c]
 *******************************************************************/
void MapLine::copy(MapObject* c)
{
	if(objType() != c->objType())
		return;

	MapObject::copy(c);

	MapLine* l = static_cast<MapLine*>(c);

	if(side1_ && l->side1_)
		side1_->copy(l->side1_);

	if(side2_ && l->side2_)
		side2_->copy(l->side2_);

	//setIntProperty("special", l->intProperty("special"));
	special_ = l->special_;
	id_ = l->id_;
}
