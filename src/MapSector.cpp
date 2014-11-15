
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapSector.cpp
 * Description: MapSector class, represents a sector object in a map
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
#include "MapSector.h"
#include "MapLine.h"
#include "MapSide.h"
#include "MapVertex.h"
#include "MainApp.h"
#include "SLADEMap.h"
#include "MathStuff.h"
#include "GameConfiguration.h"
#include <wx/colour.h>


/*******************************************************************
 * MAPSECTOR CLASS FUNCTIONS
 *******************************************************************/

/* MapSector::MapSector
 * MapSector class constructor
 *******************************************************************/
MapSector::MapSector(SLADEMap* parent) : MapObject(MOBJ_SECTOR, parent)
{
	// Init variables
	this->special = 0;
	this->tag = 0;
	poly_needsupdate = true;
	geometry_updated = theApp->runTimer();
}

/* MapSector::MapSector
 * MapSector class constructor
 *******************************************************************/
MapSector::MapSector(string f_tex, string c_tex, SLADEMap* parent) : MapObject(MOBJ_SECTOR, parent)
{
	// Init variables
	this->f_tex = f_tex;
	this->c_tex = c_tex;
	this->special = 0;
	this->tag = 0;
	poly_needsupdate = true;
	geometry_updated = theApp->runTimer();
}

/* MapSector::~MapSector
 * MapSector class destructor
 *******************************************************************/
MapSector::~MapSector()
{
}

/* MapSector::copy
 * Copies another map object [s]
 *******************************************************************/
void MapSector::copy(MapObject* s)
{
	// Don't copy a non-sector
	if (s->getObjType() != MOBJ_SECTOR)
		return;

	// Update texture counts (decrement previous)
	if (parent_map)
	{
		parent_map->updateFlatUsage(f_tex, -1);
		parent_map->updateFlatUsage(c_tex, -1);
	}

	// Basic variables
	MapSector* sector = (MapSector*)s;
	this->f_tex = sector->f_tex;
	this->c_tex = sector->c_tex;
	this->f_height = sector->f_height;
	this->c_height = sector->c_height;
	this->light = sector->light;
	this->special = sector->special;
	this->tag = sector->tag;

	// Update texture counts (increment new)
	if (parent_map)
	{
		parent_map->updateFlatUsage(f_tex, 1);
		parent_map->updateFlatUsage(c_tex, 1);
	}

	// Other properties
	MapObject::copy(s);
}

/* MapSector::stringProperty
 * Returns the value of the string property matching [key]
 *******************************************************************/
string MapSector::stringProperty(string key)
{
	if (key == "texturefloor")
		return f_tex;
	else if (key == "textureceiling")
		return c_tex;
	else
		return MapObject::stringProperty(key);
}

/* MapSector::intProperty
 * Returns the value of the integer property matching [key]
 *******************************************************************/
int MapSector::intProperty(string key)
{
	if (key == "heightfloor")
		return f_height;
	else if (key == "heightceiling")
		return c_height;
	else if (key == "lightlevel")
		return light;
	else if (key == "special")
		return special;
	else if (key == "id")
		return tag;
	else
		return MapObject::intProperty(key);
}

/* MapSector::setStringProperty
 * Sets the string value of the property [key] to [value]
 *******************************************************************/
void MapSector::setStringProperty(string key, string value)
{
	// Update modified time
	setModified();

	if (key == "texturefloor")
	{
		if (parent_map) parent_map->updateFlatUsage(f_tex, -1);
		f_tex = value;
		if (parent_map) parent_map->updateFlatUsage(f_tex, 1);
	}
	else if (key == "textureceiling")
	{
		if (parent_map) parent_map->updateFlatUsage(c_tex, -1);
		c_tex = value;
		if (parent_map) parent_map->updateFlatUsage(c_tex, 1);
	}
	else
		return MapObject::setStringProperty(key, value);
}

/* MapSector::setFloatProperty
 * Sets the float value of the property [key] to [value]
 *******************************************************************/
void MapSector::setFloatProperty(string key, double value)
{
	// Check if flat offset/scale/rotation is changing (if UDMF + ZDoom)
	if (parent_map->currentFormat() == MAP_UDMF && S_CMPNOCASE(parent_map->udmfNamespace(), "zdoom"))
	{
		if (key == "xpanningfloor" || key == "ypanningfloor" ||
		        key == "xpanningceiling" || key == "ypanningceiling" ||
		        key == "xscalefloor" || key == "yscalefloor" ||
		        key == "xscaleceiling" || key == "yscaleceiling" ||
		        key == "rotationfloor" || key == "rotationceiling")
			polygon.setTexture(NULL);	// Clear texture to force update
	}

	MapObject::setFloatProperty(key, value);
}

/* MapSector::setIntProperty
 * Sets the integer value of the property [key] to [value]
 *******************************************************************/
void MapSector::setIntProperty(string key, int value)
{
	// Update modified time
	setModified();

	if (key == "heightfloor")
		f_height = value;
	else if (key == "heightceiling")
		c_height = value;
	else if (key == "lightlevel")
		light = value;
	else if (key == "special")
		special = value;
	else if (key == "id")
		tag = value;
	else
		MapObject::setIntProperty(key, value);
}

/* MapLine::getPoint
 * Returns the object point [point]: MOBJ_POINT_MID = the absolute
 * mid point of the sector, MOBJ_POINT_WITHIN/MOBJ_POINT_TEXT =
 * a calculated point that is within the actual sector
 *******************************************************************/
fpoint2_t MapSector::getPoint(uint8_t point)
{
	if (point == MOBJ_POINT_MID)
	{
		bbox_t bbox = boundingBox();
		return fpoint2_t(bbox.min.x + ((bbox.max.x-bbox.min.x)*0.5),
			bbox.min.y + ((bbox.max.y-bbox.min.y)*0.5));
	}
	else
	{
		if (text_point.x == 0 && text_point.y == 0 && parent_map)
			parent_map->findSectorTextPoint(this);
		return text_point;
	}
}

/* MapSector::updateBBox
 * Calculates the sector's bounding box
 *******************************************************************/
void MapSector::updateBBox()
{
	// Reset bounding box
	bbox.reset();

	for (unsigned a = 0; a < connected_sides.size(); a++)
	{
		MapLine* line = connected_sides[a]->getParentLine();
		if (!line) continue;
		bbox.extend(line->v1()->xPos(), line->v1()->yPos());
		bbox.extend(line->v2()->xPos(), line->v2()->yPos());
	}

	text_point.set(0, 0);
	geometry_updated = theApp->runTimer();
}

/* MapSector::boundingBox
 * Returns the sector bounding box
 *******************************************************************/
bbox_t MapSector::boundingBox()
{
	// Update bbox if needed
	if (!bbox.is_valid())
		updateBBox();

	return bbox;
}

/* MapSector::getPolygon
 * Returns the sector polygon, updating it if necessary
 *******************************************************************/
Polygon2D* MapSector::getPolygon()
{
	if (poly_needsupdate)
	{
		poly_needsupdate = false;
		polygon.openSector(this);
	}

	return &polygon;
}

/* MapSector::isWithin
 * Returns true if the point [x, y] is inside the sector
 *******************************************************************/
bool MapSector::isWithin(double x, double y)
{
	// Check with bbox first
	if (!boundingBox().point_within(x, y))
		return false;

	// Find nearest line in the sector
	double dist;
	double min_dist = 999999;
	MapLine* nline = NULL;
	for (unsigned a = 0; a < connected_sides.size(); a++)
	{
		// Calculate distance to line
		//if (connected_sides[a] == NULL) {
		//	LOG_MESSAGE(3, "Warning: connected side #%i is a NULL pointer!", a);
		//	continue;
		//} else if (connected_sides[a]->getParentLine() == NULL) {
		//	LOG_MESSAGE(3, "Warning: connected side #%i has a NULL pointer parent line!", connected_sides[a]->getIndex());
		//	continue;
		//}
		dist = connected_sides[a]->getParentLine()->distanceTo(x, y);

		// Check distance
		if (dist < min_dist)
		{
			nline = connected_sides[a]->getParentLine();
			min_dist = dist;
		}
	}

	// No nearest (shouldn't happen)
	if (!nline)
		return false;

	// Check the side of the nearest line
	double side = MathStuff::lineSide(x, y, nline->x1(), nline->y1(), nline->x2(), nline->y2());
	if (side >= 0 && nline->frontSector() == this)
		return true;
	else if (side < 0 && nline->backSector() == this)
		return true;
	else
		return false;
}

/* MapSector::distanceTo
 * Returns the minimum distance from [x, y] to the closest line in
 * the sector
 *******************************************************************/
double MapSector::distanceTo(double x, double y, double maxdist)
{
	// Init
	if (maxdist < 0)
		maxdist = 9999999;

	// Check bounding box first
	if (!bbox.is_valid())
		updateBBox();
	double min_dist = 9999999;
	double dist = MathStuff::distanceToLine(x, y, bbox.min.x, bbox.min.y, bbox.min.x, bbox.max.y);
	if (dist < min_dist) min_dist = dist;
	dist = MathStuff::distanceToLine(x, y, bbox.min.x, bbox.max.y, bbox.max.x, bbox.max.y);
	if (dist < min_dist) min_dist = dist;
	dist = MathStuff::distanceToLine(x, y, bbox.max.x, bbox.max.y, bbox.max.x, bbox.min.y);
	if (dist < min_dist) min_dist = dist;
	dist = MathStuff::distanceToLine(x, y, bbox.max.x, bbox.min.y, bbox.min.x, bbox.min.y);
	if (dist < min_dist) min_dist = dist;

	if (min_dist > maxdist && !bbox.point_within(x, y))
		return -1;

	// Go through connected sides
	for (unsigned a = 0; a < connected_sides.size(); a++)
	{
		// Get side parent line
		MapLine* line = connected_sides[a]->getParentLine();
		if (!line) continue;

		// Check distance
		dist = line->distanceTo(x, y);
		if (dist < min_dist)
			min_dist = dist;
	}

	return min_dist;
}

/* MapSector::getLines
 * Adds all lines that are part of the sector to [list]
 *******************************************************************/
bool MapSector::getLines(vector<MapLine*>& list)
{
	// Go through connected sides
	for (unsigned a = 0; a < connected_sides.size(); a++)
	{
		// Add the side's parent line to the list if it doesn't already exist
		if (std::find(list.begin(), list.end(), connected_sides[a]->getParentLine()) == list.end())
			list.push_back(connected_sides[a]->getParentLine());
	}

	return true;
}

/* MapSector::getVertices
 * Adds all vertices that are part of the sector to [list]
 *******************************************************************/
bool MapSector::getVertices(vector<MapVertex*>& list)
{
	// Go through connected sides
	MapLine* line;
	for (unsigned a = 0; a < connected_sides.size(); a++)
	{
		line = connected_sides[a]->getParentLine();

		// Add the side's parent line's vertices to the list if they doesn't already exist
		if (line->v1() && std::find(list.begin(), list.end(), line->v1()) == list.end())
			list.push_back(line->v1());
		if (line->v2() && std::find(list.begin(), list.end(), line->v2()) == list.end())
			list.push_back(line->v2());
	}

	return true;
}

/* MapSector::getVertices
 * Adds all vertices that are part of the sector to [list]
 *******************************************************************/
bool MapSector::getVertices(vector<MapObject*>& list)
{
	// Go through connected sides
	MapLine* line;
	for (unsigned a = 0; a < connected_sides.size(); a++)
	{
		line = connected_sides[a]->getParentLine();

		// Add the side's parent line's vertices to the list if they doesn't already exist
		if (line->v1() && std::find(list.begin(), list.end(), line->v1()) == list.end())
			list.push_back(line->v1());
		if (line->v2() && std::find(list.begin(), list.end(), line->v2()) == list.end())
			list.push_back(line->v2());
	}

	return true;
}

template<> short MapSector::getPlaneHeight<FLOOR_PLANE>() { return getFloorHeight(); }
template<> short MapSector::getPlaneHeight<CEILING_PLANE>() { return getCeilingHeight(); }

/* MapSector::getPlane
 * Returns the slope of the floor or ceiling
 *******************************************************************/
template<PlaneType p> static const char* _plane_align_arg();
template<> const char* _plane_align_arg<FLOOR_PLANE>() { return "arg0"; }
template<> const char* _plane_align_arg<CEILING_PLANE>() { return "arg1"; }

template plane_t MapSector::getPlane<FLOOR_PLANE>();
template plane_t MapSector::getPlane<CEILING_PLANE>();

template<PlaneType p>
plane_t MapSector::getPlane()
{
	// Deal with slopes, in the same order as ZDoom.
	// Plane_Align
	MapSide* side;
	MapLine* line;
	// Go through connected sides
	// TODO does this need to go through lines in id order?
	for (unsigned a = 0; a < connected_sides.size(); a++)
	{
		side = connected_sides[a];
		line = side->getParentLine();
		// TODO faster to find Plane_Align's number first
		if (line->getSpecial())
		{
			ActionSpecial* as = theGameConfiguration->actionSpecial(line->getSpecial());
			if (as->getName() == "Plane_Align")
			{
				// TODO honestly this is all terrible.  just terrible.

				int prop;
				MapSector* adjacent_sector;
				// Floor
				prop = line->intProperty(_plane_align_arg<p>());
				short adjacent_height;
				if (prop == 1 && side == line->s1())
				{
					adjacent_sector = line->backSector();
					if (! adjacent_sector)
						continue;
					adjacent_height = adjacent_sector->getPlaneHeight<p>();
				}
				else if (prop == 2 && side == line->s2())
				{
					adjacent_sector = line->frontSector();
					if (! adjacent_sector)
						continue;
					adjacent_height = adjacent_sector->getPlaneHeight<p>();
				}
				else
				{
					continue;
				}

				MapSide* sideb;
				MapVertex* vtmp;
				double furthest = 0.0;
				double dist;
				MapVertex* v = NULL;
				for (unsigned b = 0; b < connected_sides.size(); b++)
				{
					vtmp = connected_sides[b]->getParentLine()->v1();
					dist = line->distanceTo(vtmp->xPos(), vtmp->yPos());
					if (dist > furthest) {
						furthest = dist;
						v = vtmp;
					}
					vtmp = connected_sides[b]->getParentLine()->v2();
					dist = line->distanceTo(vtmp->xPos(), vtmp->yPos());
					if (dist > furthest) {
						furthest = dist;
						v = vtmp;
					}
				}

				// TODO found point must not be on this same line!
				// TODO does zdoom look for furthest point, or furthest /perpendicular/ point?  which does distanceTo do?
				// TODO this line must not have length zero!
				// TODO all this code is bad
				if (furthest > 0.01) {
					// Slope the floor such that point v remains at the
					// actual floor height, but this line is at the floor
					// height of the adjacent sector.
					// Make two vectors, then make a plane from them:
					fpoint3_t pt1 = line->v1()->getPoint(0);
					pt1.z = adjacent_height;
					fpoint3_t pt2 = line->v2()->getPoint(0);
					pt2.z = adjacent_height;
					fpoint3_t pt3 = v->getPoint(0);
					pt3.z = getPlaneHeight<p>();

					fpoint3_t vec1 = pt1 - pt2;
					fpoint3_t vec2 = pt1 - pt3;
					// Procedure to get a plane from two vectors
					fpoint3_t norm = vec1.cross(vec2);
					double dot = norm.dot(pt1);
					return plane_t(norm.x, norm.y, norm.z, dot);
				}
			}
		}
	}

	// Default to a flat plane
	return plane_t::flat(getPlaneHeight<p>());
}

/* MapSector::getLight
 * Returns the light level of the sector at [where] - 1 = floor,
 * 2 = ceiling
 *******************************************************************/
uint8_t MapSector::getLight(int where)
{
	// Check for UDMF+ZDoom namespace
	if (parent_map->currentFormat() == MAP_UDMF && S_CMPNOCASE(parent_map->udmfNamespace(), "zdoom"))
	{
		// Get general light level
		int l = light;

		// Get specific light level
		if (where == 1)
		{
			// Floor
			int fl = intProperty("lightfloor");
			if (boolProperty("lightfloorabsolute"))
				l = fl;
			else
				l += fl;
		}
		else if (where == 2)
		{
			// Ceiling
			int cl = intProperty("lightceiling");
			if (boolProperty("lightceilingabsolute"))
				l = cl;
			else
				l += cl;
		}

		// Clamp light level
		if (l > 255)
			l = 255;
		if (l < 0)
			l = 0;

		return l;
	}
	else
	{
		// Clamp light level
		int l = light;
		if (l > 255)
			l = 255;
		if (l < 0)
			l = 0;

		return l;
	}
}

/* MapSector::changeLight
 * Changes the sector light level by [amount]
 *******************************************************************/
void MapSector::changeLight(int amount, int where)
{
	// Get current light level
	int ll = getLight(where);

	// Clamp amount
	if (ll + amount > 255)
		amount -= ((ll+amount) - 255);
	else if (ll + amount < 0)
		amount = -ll;

	// Check for UDMF+ZDoom namespace
	bool separate = false;
	if (parent_map->currentFormat() == MAP_UDMF && S_CMPNOCASE(parent_map->udmfNamespace(), "zdoom"))
		separate = true;

	// Change light level by amount
	if (where == 1 && separate)
	{
		int cur = intProperty("lightfloor");
		setIntProperty("lightfloor", cur + amount);
	}
	else if (where == 2 && separate)
	{
		int cur = intProperty("lightceiling");
		setIntProperty("lightceiling", cur + amount);
	}
	else
	{
		setModified();
		light = ll + amount;
	}
}

/* MapSector::getLight
 * Returns the colour of the sector at [where] - 1 = floor,
 * 2 = ceiling. If [fullbright] is true, light level is ignored
 *******************************************************************/
rgba_t MapSector::getColour(int where, bool fullbright)
{
	// Check for UDMF+ZDoom namespace
	if (parent_map->currentFormat() == MAP_UDMF && S_CMPNOCASE(parent_map->udmfNamespace(), "zdoom"))
	{
		// Get sector light colour
		int intcol = MapObject::intProperty("lightcolor");
		wxColour wxcol(intcol);

		// Ignore light level if fullbright
		if (fullbright)
			return rgba_t(wxcol.Blue(), wxcol.Green(), wxcol.Red(), 255);

		// Get sector light level
		int ll = light;

		// Get specific light level
		if (where == 1)
		{
			// Floor
			int fl = MapObject::intProperty("lightfloor");
			if (boolProperty("lightfloorabsolute"))
				ll = fl;
			else
				ll += fl;
		}
		else if (where == 2)
		{
			// Ceiling
			int cl = MapObject::intProperty("lightceiling");
			if (boolProperty("lightceilingabsolute"))
				ll = cl;
			else
				ll += cl;
		}

		// Clamp light level
		if (ll > 255)
			ll = 255;
		if (ll < 0)
			ll = 0;

		// Calculate and return the colour
		float lightmult = (float)ll / 255.0f;
		return rgba_t(wxcol.Blue() * lightmult, wxcol.Green() * lightmult, wxcol.Red() * lightmult, 255);
	}
	else
	{
		// Other format, simply return the light level
		if (fullbright)
			return rgba_t(255, 255, 255, 255);
		else
		{
			int l = light;

			// Clamp light level
			if (l > 255)
				l = 255;
			if (l < 0)
				l = 0;

			return rgba_t(l, l, l, 255);
		}
	}
}

/* MapSector::connectSide
 * Adds [side] to the list of 'connected sides' (sides that are part
 * of this sector)
 *******************************************************************/
void MapSector::connectSide(MapSide* side)
{
	connected_sides.push_back(side);
	poly_needsupdate = true;
	bbox.reset();
	setModified();
	geometry_updated = theApp->runTimer();
}

/* MapSector::disconnectSide
 * Removes [side] from the list of connected sides
 *******************************************************************/
void MapSector::disconnectSide(MapSide* side)
{
	for (unsigned a = 0; a < connected_sides.size(); a++)
	{
		if (connected_sides[a] == side)
		{
			connected_sides.erase(connected_sides.begin() + a);
			break;
		}
	}

	setModified();
	poly_needsupdate = true;
	bbox.reset();
	geometry_updated = theApp->runTimer();
}

/* MapSector::writeBackup
 * Write all sector info to a mobj_backup_t struct
 *******************************************************************/
void MapSector::writeBackup(mobj_backup_t* backup)
{
	backup->props_internal["texturefloor"] = f_tex;
	backup->props_internal["textureceiling"] = c_tex;
	backup->props_internal["heightfloor"] = f_height;
	backup->props_internal["heightceiling"] = c_height;
	backup->props_internal["lightlevel"] = light;
	backup->props_internal["special"] = special;
	backup->props_internal["id"] = tag;
}

/* MapSector::readBackup
 * Reads all sector info from a mobj_backup_t struct
 *******************************************************************/
void MapSector::readBackup(mobj_backup_t* backup)
{
	// Update texture counts (decrement previous)
	parent_map->updateFlatUsage(f_tex, -1);
	parent_map->updateFlatUsage(c_tex, -1);

	f_tex = backup->props_internal["texturefloor"].getStringValue();
	c_tex = backup->props_internal["textureceiling"].getStringValue();
	f_height = backup->props_internal["heightfloor"].getIntValue();
	c_height = backup->props_internal["heightceiling"].getIntValue();
	light = backup->props_internal["lightlevel"].getIntValue();
	special = backup->props_internal["special"].getIntValue();
	tag = backup->props_internal["id"].getIntValue();

	// Update texture counts (increment new)
	parent_map->updateFlatUsage(f_tex, 1);
	parent_map->updateFlatUsage(c_tex, 1);

	// Update geometry info
	poly_needsupdate = true;
	bbox.reset();
	geometry_updated = theApp->runTimer();
}
