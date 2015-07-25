
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
#include <cmath>


// Number of radians in the unit circle
const double TAU = M_PI * 2;


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
	plane_floor.set(0, 0, 1, 0);
	plane_ceiling.set(0, 0, 1, 0);
	poly_needsupdate = true;
	specials_needupdate = true;
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
	plane_floor.set(0, 0, 1, 0);
	plane_ceiling.set(0, 0, 1, 0);
	poly_needsupdate = true;
	specials_needupdate = true;
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
	plane_floor.set(0, 0, 1, sector->f_height);
	plane_ceiling.set(0, 0, 1, sector->c_height);

	// Update texture counts (increment new)
	if (parent_map)
	{
		parent_map->updateFlatUsage(f_tex, 1);
		parent_map->updateFlatUsage(c_tex, 1);
	}

	// Other properties
	MapObject::copy(s);
}

double MapSector::floorHeightAt(double x, double y)
{
	return plane_floor.height_at(x, y);
}

double MapSector::ceilingHeightAt(double x, double y)
{
	return plane_ceiling.height_at(x, y);
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
	{
		f_height = value;
		expireNeighborSpecials();
	}
	else if (key == "heightceiling")
	{
		c_height = value;
		expireNeighborSpecials();
	}
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
		polygon.openSector(this);
		poly_needsupdate = false;
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

/* MapSector::getColour
 * Returns the colour of the sector at [where] - 1 = floor,
 * 2 = ceiling. If [fullbright] is true, light level is ignored
 *******************************************************************/
rgba_t MapSector::getColour(int where, bool fullbright)
{
	// Check for sector colour set in open script
	// TODO: Test if this is correct behaviour
	if (parent_map->mapSpecials()->tagColoursSet())
	{
		rgba_t col;
		if (parent_map->mapSpecials()->getTagColour(tag, &col))
		{
			if (fullbright)
				return col;

			// Get sector light level
			int ll = light;

			// Clamp light level
			if (ll > 255)
				ll = 255;
			if (ll < 0)
				ll = 0;

			// Calculate and return the colour
			float lightmult = (float)ll / 255.0f;
			return col.ampf(lightmult, lightmult, lightmult, 1.0f);
		}
	}

	// Check for UDMF+ZDoom namespace
	if ((parent_map->currentFormat() == MAP_UDMF && S_CMPNOCASE(parent_map->udmfNamespace(), "zdoom")))
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

/* MapSector::expireNeighborSpecials
 * Expire special sector properties on this sector and all its neighbors -- the
 * height of this sector can change the slope of neighbors due to Plane_Align.
 *******************************************************************/
void MapSector::expireNeighborSpecials()
{
	for (unsigned a = 0; a < connected_sides.size(); a++)
		connected_sides[a]->getParentLine()->expireSectorSpecials();
}

/* MapSector::updatePlanes
 * Recompute the floor and ceiling planes, if any part of this sector has
 * changed
 *******************************************************************/
void MapSector::updatePlanes()
{
	if (!specials_needupdate)
		return;
	specials_needupdate = false;
	setModified();

	// Only ZDoom sloped sectors are currently supported
	// TODO this is kinda fugly, maybe split this function up
	if (theGameConfiguration->currentPort() == "zdoom")
	{
		plane_floor = computeZDoomPlane<FLOOR_PLANE>();
		plane_ceiling = computeZDoomPlane<CEILING_PLANE>();
	}
	else
	{
		plane_floor.set(0, 0, 1, f_height);
		plane_ceiling.set(0, 0, 1, c_height);
	}

}

/* MapSector::computeZDoomPlane
 * Computes the plane for this sector, based on ZDoom rules
 *******************************************************************/
template<PlaneType p>
plane_t MapSector::computeZDoomPlane()
{
	// ZDoom has a variety of slope mechanisms.  Replicating its behavior is
	// slightly complicated, because it applies several map-wide passes when
	// the map loads, but we want to support live adjustments.
	// Here is what ZDoom does, in order:
	//  - applies Plane_Align in line order
	//  - applies line slope + sector tilt + vavoom in thing order
	//  - applies slope copy things in thing order
	//  - overwrites vertex heights with vertex height things
	//  - applies vertex triangle slopes in sector order
	//  - applies Plane_Copy in line order
	// If the same sector is given a slope in several different ways, the last
	// one clobbers all the others.  So to replicate ZDoom's behavior, we apply
	// all these operations in /reverse/, including iterating over map objects
	// in reverse order.  Also, copies are only applied if the model sector's
	// id is less than this sector's id, which also avoids any possible
	// infinite loops of copying.
	// TODO line slope things have the same problem here
	// NOTE: One edge case isn't handled correctly.  Consider a sector X whose
	// slope is determined by vertex heights.  If this sector contains a slope
	// copy thing that points to sector X, we should NOT copy the vertex slope
	// -- ZDoom applies slope copy things before vertex heights, so the vertex
	// slope wouldn't exist yet.  But we're not this clever, alas.
	// TODO how on earth do you expire slopes when a slope thing moves, or a
	// line id changes, or the height of a sector that a line slope thing is in
	// changes...

	vector<MapLine*> lines;
	getLines(lines);
	sort(lines.begin(), lines.end());
	reverse(lines.begin(), lines.end());
	// TODO add some logging here?  is there any point?  maybe in case of errors?

	// Plane_Copy
	for (unsigned a = 0; a < lines.size(); a++)
	{
		MapLine* line = lines[a];
		if (line->getSpecial() != 118)
			continue;

		// The fifth "share" argument copies from one side of the line to the
		// other, and takes priority
		if (line->s1() && line->s2())
		{
			int share = line->intProperty("arg4");
			MapSector* model;
			int floor_copy_flag, ceiling_copy_flag;
			if (this == line->frontSector())
			{
				model = line->backSector();
				floor_copy_flag = 2;
				ceiling_copy_flag = 8;
			}
			else
			{
				model = line->frontSector();
				floor_copy_flag = 1;
				ceiling_copy_flag = 4;
			}

			// TODO fix for ceiling as well
			if (model->id < this->id && (share & 3) == floor_copy_flag)
			{
				return model->getPlane<p>();
			}
		}

		// TODO other args...
	}

	vector<MapVertex*> vertices;
	getVertices(vertices);

	// Vertex heights -- only applies if the sector has exactly three vertices.
	// Heights may be set by UDMF properties, or by a vertex height thing
	// placed exactly on the vertex (which takes priority over the prop).
	// TODO changing vertex height doesn't expire the slope, oops
	if (vertices.size() == 3)
	{
		string prop = (p == FLOOR_PLANE ? "zfloor" : "zceiling");
		if (theGameConfiguration->getUDMFProperty(prop, MOBJ_VERTEX))
		{
			double z1 = vertices[0]->floatProperty(prop);
			double z2 = vertices[1]->floatProperty(prop);
			double z3 = vertices[2]->floatProperty(prop);
			// NOTE: there's currently no way to distinguish a height of 0 from
			// an unset height, so assume the author intended to have a slope
			// if at least one vertex has a height
			if (z1 || z2 || z3)
			{
				fpoint3_t p1(vertices[0]->xPos(), vertices[0]->yPos(), z1);
				fpoint3_t p2(vertices[1]->xPos(), vertices[1]->yPos(), z2);
				fpoint3_t p3(vertices[2]->xPos(), vertices[2]->yPos(), z3);
				return MathStuff::planeFromTriangle(p1, p2, p3);
			}
		}
	}

	// Slope copy things (9510/9511)
	// TODO needs to go in reverse order
	for (unsigned a = 0; a < parent_map->nThings(); a++)
	{
		MapThing* thing = parent_map->getThing(a);

		if (thing->getType() == (p == FLOOR_PLANE ? 9510 : 9511)
			&& bbox.point_within(thing->xPos(), thing->yPos())
			&& isWithin(thing->xPos(), thing->yPos())
		)
		{
			// First argument is the tag of a sector whose slope should be copied
			int tag = thing->intProperty("arg0");
			if (!tag)
			{
				LOG_MESSAGE(1, "Ignoring slope copy thing in sector %d with no argument", index);
				continue;
			}

			vector<MapSector*> tagged_sectors;
			parent_map->getSectorsByTag(tag, tagged_sectors);
			if (tagged_sectors.empty())
			{
				LOG_MESSAGE(1, "Ignoring slope copy thing in sector %d; no sectors have target tag %d", index, tag);
				continue;
			}

			if (tagged_sectors[0]->getId() < this->id)
			{
				return tagged_sectors[0]->getPlane<p>();
			}
		}
	}

	// Line slope things (9500/9501), sector tilt things (9502/9503), and
	// vavoom things (1500/1501), all in the same pass
	// TODO needs to go in reverse order
	for (unsigned a = 0; a < parent_map->nThings(); a++)
	{
		MapThing* thing = parent_map->getThing(a);

		// Line slope things, which do NOT have to be within the sector
		if (thing->getType() == (p == FLOOR_PLANE ? 9500 : 9501))
		{
			int lineid = thing->intProperty("arg0");
			if (lineid)
			{
				// TODO oops actually i think this should go in /forward/ order by lines.  confirm that though
				for (unsigned b = 0; b < lines.size(); b++)
				{
					MapLine* line = lines[b];
					if (line->intProperty("id") != lineid)
						continue;

					// The thing only affects the sector on the side of the
					// line that faces the thing
					double side = MathStuff::lineSide(
						thing->xPos(), thing->yPos(),
						line->x1(), line->y1(), line->x2(), line->y2());
					if ((side > 0 && this == line->frontSector()) ||
						(side < 0 && this == line->backSector()))
					{
						int containing_sector_idx = parent_map->sectorAt(
							thing->xPos(), thing->yPos());
						if (containing_sector_idx < 0)
							break;

						// The height of the thing is based on the SLOPED
						// height of the sector it's in, which makes 
						// little hokey
						// TODO whoopsadaisy, this can rely on
						// previously-defined slopes for the same sector
						MapSector* containing_sector = parent_map->getSector(
							containing_sector_idx);
						double thingz = thing->floatProperty("height");
						if (containing_sector_idx < index)
							thingz += containing_sector->getPlane<p>().height_at(thing->xPos(), thing->yPos());
						else
							thingz += containing_sector->getPlaneHeight<p>();

						// Three points: endpoints of the line, and the thing itself
						double thisz = getPlaneHeight<p>();
						fpoint3_t p1(lines[b]->x1(), lines[b]->y1(), thisz);
						fpoint3_t p2(lines[b]->x2(), lines[b]->y2(), thisz);
						fpoint3_t p3(thing->xPos(), thing->yPos(), thingz);
						return MathStuff::planeFromTriangle(p1, p2, p3);
					}
				}
			}
		}

		// Sector tilt things
		if (thing->getType() == (p == FLOOR_PLANE ? 9502 : 9503)
			&& bbox.point_within(thing->xPos(), thing->yPos())
			&& isWithin(thing->xPos(), thing->yPos())
		)
		{
			// Sector tilt things.  First argument is the tilt angle, but
			// starting with 0 as straight down; subtracting 90 fixes that.
			// TODO skip if the tilt is vertical!
			double angle = thing->getAngle() / 360.0 * TAU;
			double tilt = (thing->intProperty("arg0") - 90) / 360.0 * TAU;
			// Resulting plane goes through the position of the thing
			double z = getPlaneHeight<p>() + thing->floatProperty("height");
			fpoint3_t point(thing->xPos(), thing->yPos(), z);

			double cos_angle = cos(angle);
			double sin_angle = sin(angle);
			double cos_tilt = cos(tilt);
			double sin_tilt = sin(tilt);
			// Need to convert these angles into vectors on the plane, so we
			// can take a normal.
			// For the first: we know that the line perpendicular to the
			// direction the thing faces lies "flat", because this is the axis
			// the tilt thing rotates around.  "Rotate" the angle a quarter
			// turn to get this vector -- switch x and y, and negate one.
			fpoint3_t vec1(-sin_angle, cos_angle, 0.0);

			// For the second: the tilt angle makes a triangle between the
			// floor plane and the z axis.  sin gives us the distance along the
			// z-axis, but cos only gives us the distance away /from/ the
			// z-axis.  Break that into x and y by multiplying by cos and sin
			// of the thing's facing angle.
			fpoint3_t vec2(cos_tilt * cos_angle, cos_tilt * sin_angle, sin_tilt);

			return MathStuff::planeFromTriangle(point, point + vec1, point + vec2);
		}

		// Vavoom things
		// TODO
	}

	// Plane_Align (181)
	for (unsigned a = 0; a < lines.size(); a++)
	{
		MapLine* line = lines[a];
		if (line->getSpecial() != 181)
			continue;

		int side = (this == line->frontSector()) ? 1 : 2;
		if (side != line->intProperty(p == FLOOR_PLANE ? "arg0" : "arg1"))
			continue;

		// Get sectors
		MapSector* model_sector;
		if (this == line->frontSector())
			model_sector = line->backSector();
		else
			model_sector = line->frontSector();
		if (!model_sector)
		{
			LOG_MESSAGE(1, "Ignoring Plane_Align on one-sided line %d", line->getIndex());
			continue;
		}
		if (this == model_sector)
		{
			LOG_MESSAGE(1, "Ignoring Plane_Align on line %d, which has the same sector on both sides", line->getIndex());
			continue;
		}

		// The slope is between the line with Plane_Align, and the point in the
		// sector furthest away from it, which can only be at a vertex
		double this_dist;
		MapVertex* this_vertex;
		double furthest_dist = 0.0;
		MapVertex* furthest_vertex = NULL;
		for (unsigned a = 0; a < vertices.size(); a++)
		{
			this_vertex = vertices[a];
			this_dist = line->distanceTo(this_vertex->xPos(), this_vertex->yPos());
			if (this_dist > furthest_dist)
			{
				furthest_dist = this_dist;
				furthest_vertex = this_vertex;
			}
		}

		if (!furthest_vertex || furthest_dist < 0.01)
		{
			LOG_MESSAGE(1, "Ignoring Plane_Align on line %d; sector %d has no appropriate reference vertex", line->getIndex(), this->getIndex());
			continue;
		}

		// Calculate slope plane from our three points: this line's endpoints
		// (at the model sector's height) and the found vertex (at this
		// sector's height).
		double modelz = model_sector->getPlaneHeight<p>();
		double thisz = this->getPlaneHeight<p>();
		fpoint3_t p1(line->x1(), line->y1(), modelz);
		fpoint3_t p2(line->x2(), line->y2(), modelz);
		fpoint3_t p3(furthest_vertex->xPos(), furthest_vertex->yPos(), thisz);
		return MathStuff::planeFromTriangle(p1, p2, p3);
	}

	return plane_t(0, 0, 1, getPlaneHeight<p>());
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
