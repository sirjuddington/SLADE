
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
MapLine::MapLine(SLADEMap* parent) : MapObject(MOBJ_LINE, parent)
{
	// Init variables
	vertex1 = NULL;
	vertex2 = NULL;
	side1 = NULL;
	side2 = NULL;
	length = -1;
	special = 0;
}

/* MapLine::MapLine
 * MapLine class constructor
 *******************************************************************/
MapLine::MapLine(MapVertex* v1, MapVertex* v2, MapSide* s1, MapSide* s2, SLADEMap* parent) : MapObject(MOBJ_LINE, parent)
{
	// Init variables
	vertex1 = v1;
	vertex2 = v2;
	side1 = s1;
	side2 = s2;
	length = -1;
	special = 0;

	// Connect to vertices
	if (v1) v1->connectLine(this);
	if (v2) v2->connectLine(this);

	// Connect to sides
	if (s1) s1->parent = this;
	if (s2) s2->parent = this;
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
	if (side1)
		return side1->sector;
	else
		return NULL;
}

/* MapLine::backSector
 * Returns the sector on the back side of the line (if any)
 *******************************************************************/
MapSector* MapLine::backSector()
{
	if (side2)
		return side2->sector;
	else
		return NULL;
}

/* MapLine::x1
 * Returns the x coordinate of the first vertex
 *******************************************************************/
double MapLine::x1()
{
	return vertex1->xPos();
}

/* MapLine::y1
 * Returns the y coordinate of the first vertex
 *******************************************************************/
double MapLine::y1()
{
	return vertex1->yPos();
}

/* MapLine::x2
 * Returns the x coordinate of the second vertex
 *******************************************************************/
double MapLine::x2()
{
	return vertex2->xPos();
}

/* MapLine::y2
 * Returns the y coordinate of the second vertex
 *******************************************************************/
double MapLine::y2()
{
	return vertex2->yPos();
}

/* MapLine::v1Index
 * Returns the index of the first vertex, or -1 if none
 *******************************************************************/
int MapLine::v1Index()
{
	if (vertex1)
		return vertex1->getIndex();
	else
		return -1;
}

/* MapLine::v2Index
 * Returns the index of the second vertex, or -1 if none
 *******************************************************************/
int MapLine::v2Index()
{
	if (vertex2)
		return vertex2->getIndex();
	else
		return -1;
}

/* MapLine::s1Index
 * Returns the index of the front side, or -1 if none
 *******************************************************************/
int MapLine::s1Index()
{
	if (side1)
		return side1->getIndex();
	else
		return -1;
}

/* MapLine::s2Index
 * Returns the index of the back side, or -1 if none
 *******************************************************************/
int MapLine::s2Index()
{
	if (side2)
		return side2->getIndex();
	else
		return -1;
}

/* MapLine::boolProperty
 * Returns the value of the boolean property matching [key]. Can be
 * prefixed with 'side1.' or 'side2.' to get bool properties from the
 * front and back sides respectively
 *******************************************************************/
bool MapLine::boolProperty(string key)
{
	if (key.StartsWith("side1.") && side1)
		return side1->boolProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2)
		return side2->boolProperty(key.Mid(6));
	else
		return MapObject::boolProperty(key);
}

/* MapLine::intProperty
 * Returns the value of the integer property matching [key]. Can be
 * prefixed with 'side1.' or 'side2.' to get int properties from the
 * front and back sides respectively
 *******************************************************************/
int MapLine::intProperty(string key)
{
	if (key.StartsWith("side1.") && side1)
		return side1->intProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2)
		return side2->intProperty(key.Mid(6));
	else if (key == "v1")
		return v1Index();
	else if (key == "v2")
		return v2Index();
	else if (key == "sidefront")
		return s1Index();
	else if (key == "sideback")
		return s2Index();
	else if (key == "special")
		return special;
	else
		return MapObject::intProperty(key);
}

/* MapLine::floatProperty
 * Returns the value of the float property matching [key]. Can be
 * prefixed with 'side1.' or 'side2.' to get float properties from
 * the front and back sides respectively
 *******************************************************************/
double MapLine::floatProperty(string key)
{
	if (key.StartsWith("side1.") && side1)
		return side1->floatProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2)
		return side2->floatProperty(key.Mid(6));
	else
		return MapObject::floatProperty(key);
}

/* MapLine::stringProperty
 * Returns the value of the string property matching [key]. Can be
 * prefixed with 'side1.' or 'side2.' to get string properties from
 * the front and back sides respectively
 *******************************************************************/
string MapLine::stringProperty(string key)
{
	if (key.StartsWith("side1.") && side1)
		return side1->stringProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2)
		return side2->stringProperty(key.Mid(6));
	else
		return MapObject::stringProperty(key);
}

/* MapLine::setBoolProperty
 * Sets the boolean value of the property [key] to [value]. Can be
 * prefixed with 'side1.' or 'side2.' to set bool properties on the
 * front and back sides respectively.
 *******************************************************************/
void MapLine::setBoolProperty(string key, bool value)
{
	// Front side property
	if (key.StartsWith("side1."))
	{
		if (side1)
			return side1->setBoolProperty(key.Mid(6), value);
	}

	// Back side property
	else if (key.StartsWith("side2."))
	{
		if (side2)
			return side2->setBoolProperty(key.Mid(6), value);
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
void MapLine::setIntProperty(string key, int value)
{
	// Front side property
	if (key.StartsWith("side1."))
	{
		if (side1)
			side1->setIntProperty(key.Mid(6), value);
		return;
	}

	// Back side property
	else if (key.StartsWith("side2."))
	{
		if (side2)
			side2->setIntProperty(key.Mid(6), value);
		return;
	}

	// Mark as modified only if a line prop, not a side prop, is changing
	setModified();

	// Vertices
	if (key == "v1")
	{
		MapVertex* vertex;
		if ((vertex = parent_map->getVertex(value)))
		{
			vertex1->disconnectLine(this);
			vertex1 = vertex;
			vertex1->connectLine(this);
			resetInternals();
		}
	}
	else if (key == "v2")
	{
		MapVertex* vertex;
		if ((vertex = parent_map->getVertex(value)))
		{
			vertex2->disconnectLine(this);
			vertex2 = vertex;
			vertex2->connectLine(this);
			resetInternals();
		}
	}

	// Sides
	else if (key == "sidefront")
	{
		MapSide* side = parent_map->getSide(value);
		if (side)
			parent_map->setLineSide(this, side, true);
	}
	else if (key == "sideback")
	{
		MapSide* side = parent_map->getSide(value);
		if (side)
			parent_map->setLineSide(this, side, false);
	}

	// Special
	else if (key == "special")
		special = value;

	// Line property
	else
		MapObject::setIntProperty(key, value);
}

/* MapLine::setFloatProperty
 * Sets the float value of the property [key] to [value]. Can be
 * prefixed with 'side1.' or 'side2.' to set float properties on the
 * front and back sides respectively.
 *******************************************************************/
void MapLine::setFloatProperty(string key, double value)
{
	// Front side property
	if (key.StartsWith("side1."))
	{
		if (side1)
			return side1->setFloatProperty(key.Mid(6), value);
	}

	// Back side property
	else if (key.StartsWith("side2."))
	{
		if (side2)
			return side2->setFloatProperty(key.Mid(6), value);
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
void MapLine::setStringProperty(string key, string value)
{
	// Front side property
	if (key.StartsWith("side1."))
	{
		if (side1)
			return side1->setStringProperty(key.Mid(6), value);
	}

	// Back side property
	else if (key.StartsWith("side2."))
	{
		if (side2)
			return side2->setStringProperty(key.Mid(6), value);
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
	if (!side1 && parent_map)
		parent_map->setLineSide(this, side, true);
}

/* MapLine::setS1
 * Sets the back side of the line to [side]
 *******************************************************************/
void MapLine::setS2(MapSide* side)
{
	if (!side2 && parent_map)
		parent_map->setLineSide(this, side, false);
}

/* MapLine::getPoint
 * Returns the object point [point]. Currently for lines this is
 * always the mid point
 *******************************************************************/
fpoint2_t MapLine::getPoint(uint8_t point)
{
	//if (point == MOBJ_POINT_MID || point == MOBJ_POINT_WITHIN)
	return point1() + (point2() - point1()) * 0.5;
}

/* MapLine::point1
 * Returns the point at the first vertex.
 *******************************************************************/
fpoint2_t MapLine::point1()
{
	return vertex1->point();
}

/* MapLine::point2
 * Returns the point at the second vertex.
 *******************************************************************/
fpoint2_t MapLine::point2()
{
	return vertex2->point();
}

/* MapLine::seg
 * Returns this line as a segment.
 *******************************************************************/
fseg2_t MapLine::seg()
{
	return fseg2_t(vertex1->point(), vertex2->point());
}

/* MapLine::getLength
 * Returns the length of the line
 *******************************************************************/
double MapLine::getLength()
{
	if (!vertex1 || !vertex2)
		return -1;

	if (length < 0)
	{
		length = this->seg().length();
		ca = (vertex2->xPos() - vertex1->xPos()) / length;
		sa = (vertex2->yPos() - vertex1->yPos()) / length;
	}

	return length;
}

/* MapLine::doubleSector
 * Returns true if the line has the same sector on both sides
 *******************************************************************/
bool MapLine::doubleSector()
{
	// Check both sides exist
	if (!side1 || !side2)
		return false;

	return (side1->getSector() == side2->getSector());
}

/* MapLine::frontVector
 * Returns the vector perpendicular to the front side of the line
 *******************************************************************/
fpoint2_t MapLine::frontVector()
{
	// Check if vector needs to be recalculated
	if (front_vec.x == 0 && front_vec.y == 0)
	{
		front_vec.set(-(vertex2->yPos() - vertex1->yPos()), vertex2->xPos() - vertex1->xPos());
		front_vec.normalize();
	}

	return front_vec;
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
		tablen = getLength() * 0.1;
		if (tablen > 16) tablen = 16;
		if (tablen < 2) tablen = 2;
	}

	// Calculate tab endpoint
	if (front_vec.x == 0 && front_vec.y == 0) frontVector();
	return fpoint2_t(mid.x - front_vec.x*tablen, mid.y - front_vec.y*tablen);
}

/* MapLine::distanceTo
 * Returns the minimum distance from the point to the line
 *******************************************************************/
double MapLine::distanceTo(fpoint2_t point)
{
	// Update length data if needed
	if (length < 0)
	{
		length = this->seg().length();
		if (length != 0)
		{
			ca = (vertex2->xPos() - vertex1->xPos()) / length;
			sa = (vertex2->yPos() - vertex1->yPos()) / length;
		}
	}

	// Calculate intersection point
	double mx, ix, iy;
	mx = (-vertex1->xPos()+point.x)*ca + (-vertex1->yPos()+point.y)*sa;
	if (mx <= 0)		mx = 0.00001;				// Clip intersection to line (but not exactly on endpoints)
	else if (mx >= length)	mx = length - 0.00001;	// ^^
	ix = vertex1->xPos() + mx*ca;
	iy = vertex1->yPos() + mx*sa;

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
		return TEX_FRONT_MIDDLE;

	// Get sector planes
	plane_t floor_front = frontSector()->getFloorPlane();
	plane_t ceiling_front = frontSector()->getCeilingPlane();
	plane_t floor_back = backSector()->getFloorPlane();
	plane_t ceiling_back = backSector()->getCeilingPlane();

	double front_height, back_height;

	int tex = 0;

	// Check the floor
	front_height = floor_front.height_at(x1(), y1());
	back_height = floor_back.height_at(x1(), y1());

	if (front_height - back_height > EPSILON)
		tex |= TEX_BACK_LOWER;
	if (back_height - front_height > EPSILON)
		tex |= TEX_FRONT_LOWER;

	front_height = floor_front.height_at(x2(), y2());
	back_height = floor_back.height_at(x2(), y2());

	if (front_height - back_height > EPSILON)
		tex |= TEX_BACK_LOWER;
	if (back_height - front_height > EPSILON)
		tex |= TEX_FRONT_LOWER;

	// Check the ceiling
	front_height = ceiling_front.height_at(x1(), y1());
	back_height = ceiling_back.height_at(x1(), y1());

	if (back_height - front_height > EPSILON)
		tex |= TEX_BACK_UPPER;
	if (front_height - back_height > EPSILON)
		tex |= TEX_FRONT_UPPER;

	front_height = ceiling_front.height_at(x2(), y2());
	back_height = ceiling_back.height_at(x2(), y2());

	if (back_height - front_height > EPSILON)
		tex |= TEX_BACK_UPPER;
	if (front_height - back_height > EPSILON)
		tex |= TEX_FRONT_UPPER;

	return tex;
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
	if (side1)
	{
		if ((tex & TEX_FRONT_MIDDLE) == 0)
			setStringProperty("side1.texturemiddle", "-");
		if ((tex & TEX_FRONT_UPPER) == 0)
			setStringProperty("side1.texturetop", "-");
		if ((tex & TEX_FRONT_LOWER) == 0)
			setStringProperty("side1.texturebottom", "-");
	}
	if (side2)
	{
		if ((tex & TEX_BACK_MIDDLE) == 0)
			setStringProperty("side2.texturemiddle", "-");
		if ((tex & TEX_BACK_UPPER) == 0)
			setStringProperty("side2.texturetop", "-");
		if ((tex & TEX_BACK_LOWER) == 0)
			setStringProperty("side2.texturebottom", "-");
	}
}

/* MapLine::resetInternals
 * Resets all calculated internal values for the line and sectors
 *******************************************************************/
void MapLine::resetInternals()
{
	// Reset line internals
	length = -1;
	front_vec.set(0, 0);

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
	MapVertex* v = vertex1;
	vertex1 = vertex2;
	vertex2 = v;

	// Flip sides if needed
	if (sides)
	{
		MapSide* s = side1;
		side1 = side2;
		side2 = s;
	}

	resetInternals();
	if (parent_map)
		parent_map->setGeometryUpdated();
}

/* MapLine::writeBackup
 * Write all line info to a mobj_backup_t struct
 *******************************************************************/
void MapLine::writeBackup(mobj_backup_t* backup)
{
	// Vertices
	backup->props_internal["v1"] = vertex1->getId();
	backup->props_internal["v2"] = vertex2->getId();

	// Sides
	if (side1)
		backup->props_internal["s1"] = side1->getId();
	else
		backup->props_internal["s1"] = 0;
	if (side2)
		backup->props_internal["s2"] = side2->getId();
	else
		backup->props_internal["s2"] = 0;

	// Special
	backup->props_internal["special"] = special;
}

/* MapLine::readBackup
 * Reads all line info from a mobj_backup_t struct
 *******************************************************************/
void MapLine::readBackup(mobj_backup_t* backup)
{
	// Vertices
	MapObject* v1 = parent_map->getObjectById(backup->props_internal["v1"]);
	MapObject* v2 = parent_map->getObjectById(backup->props_internal["v2"]);
	if (v1)
	{
		vertex1->disconnectLine(this);
		vertex1 = (MapVertex*)v1;
		vertex1->connectLine(this);
		resetInternals();
	}
	if (v2)
	{
		vertex2->disconnectLine(this);
		vertex2 = (MapVertex*)v2;
		vertex2->connectLine(this);
		resetInternals();
	}

	// Sides
	MapObject* s1 = parent_map->getObjectById(backup->props_internal["s1"]);
	MapObject* s2 = parent_map->getObjectById(backup->props_internal["s2"]);
	side1 = (MapSide*)s1;
	side2 = (MapSide*)s2;
	if (side1) side1->parent = this;
	if (side2) side2->parent = this;

	// Special
	special = backup->props_internal["special"];
}

/* MapLine::copy
 * Copies another map object [c]
 *******************************************************************/
void MapLine::copy(MapObject* c)
{
	if(getObjType() != c->getObjType())
		return;

	MapObject::copy(c);

	MapLine* l = static_cast<MapLine*>(c);

	if(side1 && l->side1)
		side1->copy(l->side1);

	if(side2 && l->side2)
		side2->copy(l->side2);

	setIntProperty("special", l->intProperty("special"));
}
