
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapSector.cpp
// Description: MapSector class, represents a sector object in a map
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
#include "MapSector.h"
#include "App.h"
#include "Game/Configuration.h"
#include "Geometry/Geometry.h"
#include "MapLine.h"
#include "MapSide.h"
#include "MapVertex.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/Debuggable.h"
#include "Utility/Parser.h"
#include "Utility/Polygon.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// MapSector Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapSector class constructor
// -----------------------------------------------------------------------------
MapSector::MapSector(
	int         f_height,
	string_view f_tex,
	int         c_height,
	string_view c_tex,
	short       light,
	short       special,
	short       id) :
	MapObject(Type::Sector),
	floor_{ f_tex, f_height, Plane::flat(f_height) },
	ceiling_{ c_tex, c_height, Plane::flat(c_height) },
	light_{ light },
	special_{ special },
	id_{ id },
	geometry_updated_{ app::runTimer() }
{
}

// -----------------------------------------------------------------------------
// MapSector class constructor from UDMF definition
// -----------------------------------------------------------------------------
MapSector::MapSector(string_view f_tex, string_view c_tex, const ParseTreeNode* udmf_def) :
	MapObject(Type::Sector),
	floor_{ f_tex },
	ceiling_{ c_tex }
{
	// Set UDMF defaults
	light_ = 160;

	// Set properties from UDMF definition
	ParseTreeNode* prop;
	for (unsigned a = 0; a < udmf_def->nChildren(); a++)
	{
		prop = udmf_def->childPTN(a);

		// Skip required properties
		if (prop->nameIsCI(PROP_TEXFLOOR) || prop->nameIsCI(PROP_TEXCEILING))
			continue;

		if (prop->nameIsCI(PROP_HEIGHTFLOOR))
			setFloorHeight(prop->intValue());
		else if (prop->nameIsCI(PROP_HEIGHTCEILING))
			setCeilingHeight(prop->intValue());
		else if (prop->nameIsCI(PROP_LIGHTLEVEL))
			light_ = prop->intValue();
		else if (prop->nameIsCI(PROP_SPECIAL))
			special_ = prop->intValue();
		else if (prop->nameIsCI(PROP_ID))
			id_ = prop->intValue();
		else
			properties_[prop->name()] = prop->value();
	}
}

// -----------------------------------------------------------------------------
// MapSector class destructor
// -----------------------------------------------------------------------------
MapSector::~MapSector() = default;

// -----------------------------------------------------------------------------
// Copies another map object [s]
// -----------------------------------------------------------------------------
void MapSector::copy(MapObject* obj)
{
	// Don't copy a non-sector
	if (obj->objType() != Type::Sector)
		return;

	// Update modified time
	setModified();

	// Update texture counts (decrement previous)
	if (parent_map_)
	{
		parent_map_->sectors().updateTexUsage(floor_.texture, -1);
		parent_map_->sectors().updateTexUsage(ceiling_.texture, -1);
	}

	// Basic variables
	auto sector      = dynamic_cast<MapSector*>(obj);
	floor_.texture   = sector->floor_.texture;
	ceiling_.texture = sector->ceiling_.texture;
	floor_.height    = sector->floor_.height;
	ceiling_.height  = sector->ceiling_.height;
	light_           = sector->light_;
	special_         = sector->special_;
	id_              = sector->id_;
	floor_.plane.set(0, 0, 1, sector->floor_.height);
	ceiling_.plane.set(0, 0, 1, sector->ceiling_.height);

	// Update texture counts (increment new)
	if (parent_map_)
	{
		parent_map_->sectors().updateTexUsage(floor_.texture, 1);
		parent_map_->sectors().updateTexUsage(ceiling_.texture, 1);
	}

	// Other properties
	MapObject::copy(obj);
}

// -----------------------------------------------------------------------------
// Update the last time the sector geometry changed
// -----------------------------------------------------------------------------
void MapSector::setGeometryUpdated() const
{
	geometry_updated_ = app::runTimer();
}

// -----------------------------------------------------------------------------
// Returns the value of the string property matching [key]
// -----------------------------------------------------------------------------
string MapSector::stringProperty(string_view key) const
{
	if (key == PROP_TEXFLOOR)
		return floor_.texture;
	else if (key == PROP_TEXCEILING)
		return ceiling_.texture;
	else
		return MapObject::stringProperty(key);
}

// -----------------------------------------------------------------------------
// Returns the value of the integer property matching [key]
// -----------------------------------------------------------------------------
int MapSector::intProperty(string_view key) const
{
	if (key == PROP_HEIGHTFLOOR)
		return floor_.height;
	else if (key == PROP_HEIGHTCEILING)
		return ceiling_.height;
	else if (key == PROP_LIGHTLEVEL)
		return light_;
	else if (key == PROP_SPECIAL)
		return special_;
	else if (key == PROP_ID)
		return id_;
	else
		return MapObject::intProperty(key);
}

// -----------------------------------------------------------------------------
// Sets the string value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapSector::setStringProperty(string_view key, string_view value)
{
	if (key == PROP_TEXFLOOR)
		setFloorTexture(value);
	else if (key == PROP_TEXCEILING)
		setCeilingTexture(value);
	else
		return MapObject::setStringProperty(key, value);
}

// -----------------------------------------------------------------------------
// Sets the float value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapSector::setFloatProperty(string_view key, double value)
{
	using game::UDMFFeature;

	//// Check if flat offset/scale/rotation is changing (if UDMF)
	// if (parent_map_->currentFormat() == MapFormat::UDMF)
	//{
	//	if ((game::configuration().featureSupported(UDMFFeature::FlatPanning)
	//		 && (key == "xpanningfloor" || key == "ypanningfloor"))
	//		|| (game::configuration().featureSupported(UDMFFeature::FlatScaling)
	//			&& (key == "xscalefloor" || key == "yscalefloor" || key == "xscaleceiling" || key == "yscaleceiling"))
	//		|| (game::configuration().featureSupported(UDMFFeature::FlatRotation)
	//			&& (key == "rotationfloor" || key == "rotationceiling")))
	//		polygon_.setTexture(0); // Clear texture to force update
	// }

	MapObject::setFloatProperty(key, value);
}

// -----------------------------------------------------------------------------
// Sets the integer value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapSector::setIntProperty(string_view key, int value)
{
	// Update modified time
	setModified();

	if (key == PROP_HEIGHTFLOOR)
		setFloorHeight(value);
	else if (key == PROP_HEIGHTCEILING)
		setCeilingHeight(value);
	else if (key == PROP_LIGHTLEVEL)
		light_ = value;
	else if (key == PROP_SPECIAL)
		special_ = value;
	else if (key == PROP_ID)
		id_ = value;
	else
		MapObject::setIntProperty(key, value);
}

// -----------------------------------------------------------------------------
// Sets the floor texture to [tex]
// -----------------------------------------------------------------------------
void MapSector::setFloorTexture(string_view tex)
{
	setModified();
	if (parent_map_)
		parent_map_->sectors().updateTexUsage(floor_.texture, -1);
	floor_.texture = tex;
	if (parent_map_)
		parent_map_->sectors().updateTexUsage(floor_.texture, 1);
}

// -----------------------------------------------------------------------------
// Sets the ceiling texture to [tex]
// -----------------------------------------------------------------------------
void MapSector::setCeilingTexture(string_view tex)
{
	setModified();
	if (parent_map_)
		parent_map_->sectors().updateTexUsage(ceiling_.texture, -1);
	ceiling_.texture = tex;
	if (parent_map_)
		parent_map_->sectors().updateTexUsage(ceiling_.texture, 1);
}

// -----------------------------------------------------------------------------
// Sets the floor height to [height]
// -----------------------------------------------------------------------------
void MapSector::setFloorHeight(short height)
{
	setModified();
	floor_.height = height;
	setFloorPlane(Plane::flat(height));
}

// -----------------------------------------------------------------------------
// Sets the ceiling height to [height]
// -----------------------------------------------------------------------------
void MapSector::setCeilingHeight(short height)
{
	setModified();
	ceiling_.height = height;
	setCeilingPlane(Plane::flat(height));
}

// -----------------------------------------------------------------------------
// Sets the floor plane to [p]
// -----------------------------------------------------------------------------
void MapSector::setFloorPlane(const Plane& p)
{
	if (floor_.plane != p)
		setGeometryUpdated();
	floor_.plane = p;
}

// -----------------------------------------------------------------------------
// Sets the ceiling plane to [p]
// -----------------------------------------------------------------------------
void MapSector::setCeilingPlane(const Plane& p)
{
	if (ceiling_.plane != p)
		setGeometryUpdated();
	ceiling_.plane = p;
}

// -----------------------------------------------------------------------------
// Sets the [light] level
// -----------------------------------------------------------------------------
void MapSector::setLightLevel(int light)
{
	setModified();
	light_ = light;
}

// -----------------------------------------------------------------------------
// Sets the [special]
// -----------------------------------------------------------------------------
void MapSector::setSpecial(int special)
{
	setModified();
	special_ = special;
}

// -----------------------------------------------------------------------------
// Sets the [tag]
// -----------------------------------------------------------------------------
void MapSector::setTag(int tag)
{
	setModified();
	id_ = tag;
}

// -----------------------------------------------------------------------------
// Returns the object point [point]:
// Mid = the absolute mid point of the sector,
// Within/Text = a calculated point that is within the actual sector
// -----------------------------------------------------------------------------
Vec2d MapSector::getPoint(Point point)
{
	if (point == Point::Mid)
	{
		auto bbox = boundingBox();
		return { bbox.min.x + ((bbox.max.x - bbox.min.x) * 0.5), bbox.min.y + ((bbox.max.y - bbox.min.y) * 0.5) };
	}
	else
	{
		if (text_point_.x == 0 && text_point_.y == 0 && parent_map_)
			findTextPoint();
		return text_point_;
	}
}

// -----------------------------------------------------------------------------
// Calculates the sector's bounding box
// -----------------------------------------------------------------------------
void MapSector::updateBBox()
{
	// Reset bounding box
	bbox_.reset();

	for (auto& connected_side : connected_sides_)
	{
		auto line = connected_side->parentLine();
		if (!line)
			continue;
		bbox_.extend(line->v1()->xPos(), line->v1()->yPos());
		bbox_.extend(line->v2()->xPos(), line->v2()->yPos());
	}

	text_point_ = { 0, 0 };
	setGeometryUpdated();
}

// -----------------------------------------------------------------------------
// Returns the sector bounding box
// -----------------------------------------------------------------------------
BBox MapSector::boundingBox()
{
	// Update bbox if needed
	if (!bbox_.isValid())
		updateBBox();

	return bbox_;
}

// -----------------------------------------------------------------------------
// Returns the sector polygon vertices (as triangles), updating if necessary
// -----------------------------------------------------------------------------
const vector<glm::vec2>& MapSector::polygonVertices() const
{
	if (poly_needsupdate_)
	{
		polygon_triangles_ = polygon::generateSectorTriangles(*this);
		poly_needsupdate_  = false;
	}

	return polygon_triangles_;
}

// -----------------------------------------------------------------------------
// Returns true if the given [point] is inside the sector
// -----------------------------------------------------------------------------
bool MapSector::containsPoint(const Vec2d& point)
{
	// Check with bbox first
	if (!boundingBox().contains(point))
		return false;

	// Find nearest line in the sector
	double   dist;
	double   min_dist = 999999;
	MapLine* nline    = nullptr;
	for (auto& connected_side : connected_sides_)
	{
		// Calculate distance to line
		dist = connected_side->parentLine()->distanceTo(point);

		// Check distance
		if (dist < min_dist)
		{
			nline    = connected_side->parentLine();
			min_dist = dist;
		}
	}

	// No nearest (shouldn't happen)
	if (!nline)
		return false;

	// Check the side of the nearest line
	double side = geometry::lineSide(point, nline->seg());
	if (side >= 0 && nline->frontSector() == this)
		return true;
	else if (side < 0 && nline->backSector() == this)
		return true;
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns the minimum distance from the point to the closest line in the sector
// -----------------------------------------------------------------------------
double MapSector::distanceTo(const Vec2d& point, double maxdist)
{
	// Init
	if (maxdist < 0)
		maxdist = 9999999;

	// Check bounding box first
	if (!bbox_.isValid())
		updateBBox();
	double min_dist = 9999999;
	double dist     = geometry::distanceToLine(point, bbox_.leftSide());
	if (dist < min_dist)
		min_dist = dist;
	dist = geometry::distanceToLine(point, bbox_.topSide());
	if (dist < min_dist)
		min_dist = dist;
	dist = geometry::distanceToLine(point, bbox_.rightSide());
	if (dist < min_dist)
		min_dist = dist;
	dist = geometry::distanceToLine(point, bbox_.bottomSide());
	if (dist < min_dist)
		min_dist = dist;

	if (min_dist > maxdist && !bbox_.contains(point))
		return -1;

	// Go through connected sides
	for (auto& connected_side : connected_sides_)
	{
		// Get side parent line
		auto line = connected_side->parentLine();
		if (!line)
			continue;

		// Check distance
		dist = line->distanceTo(point);
		if (dist < min_dist)
			min_dist = dist;
	}

	return min_dist;
}

// -----------------------------------------------------------------------------
// Adds all lines that are part of the sector to [list]
// -----------------------------------------------------------------------------
bool MapSector::putLines(vector<MapLine*>& list) const
{
	// Go through connected sides
	for (auto& connected_side : connected_sides_)
	{
		// Add the side's parent line to the list if it doesn't already exist
		if (std::find(list.begin(), list.end(), connected_side->parentLine()) == list.end())
			list.push_back(connected_side->parentLine());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Adds all vertices that are part of the sector to [list]
// -----------------------------------------------------------------------------
bool MapSector::putVertices(vector<MapVertex*>& list) const
{
	for (auto& connected_side : connected_sides_)
	{
		auto line = connected_side->parentLine();

		// Add the side's parent line's vertices to the list if they doesn't already exist
		if (line->v1() && std::find(list.begin(), list.end(), line->v1()) == list.end())
			list.push_back(line->v1());
		if (line->v2() && std::find(list.begin(), list.end(), line->v2()) == list.end())
			list.push_back(line->v2());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Adds all vertices that are part of the sector to [list]
// -----------------------------------------------------------------------------
bool MapSector::putVertices(vector<MapObject*>& list) const
{
	for (auto& connected_side : connected_sides_)
	{
		auto line = connected_side->parentLine();

		// Add the side's parent line's vertices to the list if they doesn't already exist
		if (line->v1() && std::find(list.begin(), list.end(), line->v1()) == list.end())
			list.push_back(line->v1());
		if (line->v2() && std::find(list.begin(), list.end(), line->v2()) == list.end())
			list.push_back(line->v2());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns the light level of the sector at [where] - 1 = floor, 2 = ceiling
// -----------------------------------------------------------------------------
uint8_t MapSector::lightAt(int where, int extra_floor_index)
{
	// Check for UDMF + flat lighting
	if (parent_map_->currentFormat() == MapFormat::UDMF
		&& game::configuration().featureSupported(game::UDMFFeature::FlatLighting))
	{
		// 3D floors cast their light downwards to the next floor down, so we
		// need to know which floor this plane is below.
		// The floor plane is on the bottom of the 3D floor, so no change is
		// necessary -- unless we're being asked for the light of the sector
		// itself, which is below the bottommost floor.
		// Ceilings are below the next 3D floor up, so subtract 1.
		int floor_gap = extra_floor_index;
		if (where == 2)
			floor_gap--;
		else if (where == 1 && floor_gap < 0)
			floor_gap = extra_floors_.size() - 1;
		auto* control_sector = this;
		if (floor_gap >= 0 && floor_gap < extra_floors_.size() && !extra_floors_[floor_gap].disableLighting()
			&& !extra_floors_[floor_gap].lightingInsideOnly())
		{
			control_sector = parent_map_->sector(extra_floors_[floor_gap].control_sector_index);
		}

		// Get general light level
		int l = control_sector->lightLevel();

		// Get specific light level
		// TODO unclear how 3D floors work here -- what wins? what sector does it come from?
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
		int l = light_;
		if (l > 255)
			l = 255;
		if (l < 0)
			l = 0;

		return l;
	}
}

// -----------------------------------------------------------------------------
// Changes the sector light level by [amount]
// -----------------------------------------------------------------------------
void MapSector::changeLight(int amount, int where)
{
	// Get current light level
	int ll = lightAt(where);

	// Clamp amount
	if (ll + amount > 255)
		amount -= ((ll + amount) - 255);
	else if (ll + amount < 0)
		amount = -ll;

	// Check for UDMF + flat lighting independent from the sector
	bool separate = parent_map_->currentFormat() == MapFormat::UDMF
					&& game::configuration().featureSupported(game::UDMFFeature::FlatLighting);

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
		light_ = ll + amount;
	}
}

// -----------------------------------------------------------------------------
// Returns the colour of the sector at [where] - 1 = floor, 2 = ceiling.
// If [fullbright] is true, light level is ignored
// -----------------------------------------------------------------------------
ColRGBA MapSector::colourAt(int where, bool fullbright) const
{
	using game::UDMFFeature;

	// Check for sector colour set in open script
	// TODO: Test if this is correct behaviour
	if (parent_map_->mapSpecials()->tagColoursSet())
	{
		ColRGBA col;
		if (parent_map_->mapSpecials()->tagColour(id_, &col))
		{
			if (fullbright)
				return col;

			// Get sector light level
			int ll = light_;

			// Clamp light level
			if (ll > 255)
				ll = 255;
			if (ll < 0)
				ll = 0;

			// Calculate and return the colour
			float lightmult = static_cast<float>(ll) / 255.0f;
			return col.ampf(lightmult, lightmult, lightmult, 1.0f);
		}
	}

	// Check for UDMF
	if (parent_map_->currentFormat() == MapFormat::UDMF
		&& (game::configuration().featureSupported(UDMFFeature::SectorColor)
			|| game::configuration().featureSupported(UDMFFeature::FlatLighting)))
	{
		// Get sector light colour
		wxColour wxcol;
		if (game::configuration().featureSupported(UDMFFeature::SectorColor))
		{
			int intcol = MapObject::intProperty("lightcolor");
			wxcol      = wxColour(intcol);
		}
		else
			wxcol = wxColour(255, 255, 255, 255);


		// Ignore light level if fullbright
		if (fullbright)
			return { wxcol.Blue(), wxcol.Green(), wxcol.Red(), 255 };

		// Get sector light level
		int ll = light_;

		if (game::configuration().featureSupported(UDMFFeature::FlatLighting))
		{
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
		}

		// Clamp light level
		if (ll > 255)
			ll = 255;
		if (ll < 0)
			ll = 0;

		// Calculate and return the colour
		float lightmult = static_cast<float>(ll) / 255.0f;
		return { static_cast<uint8_t>(wxcol.Blue() * lightmult),
				 static_cast<uint8_t>(wxcol.Green() * lightmult),
				 static_cast<uint8_t>(wxcol.Red() * lightmult),
				 255 };
	}

	// Other format, simply return the light level
	if (fullbright)
		return { 255, 255, 255, 255 };
	else
	{
		int l = light_;

		// Clamp light level
		if (l > 255)
			l = 255;
		if (l < 0)
			l = 0;

		auto l8 = static_cast<uint8_t>(l);

		return { l8, l8, l8, 255 };
	}
}

// -----------------------------------------------------------------------------
// Returns the fog colour of the sector
// -----------------------------------------------------------------------------
ColRGBA MapSector::fogColour() const
{
	ColRGBA color(0, 0, 0, 0);

	// Map specials/scripts
	if (parent_map_->mapSpecials()->tagFadeColoursSet())
	{
		if (parent_map_->mapSpecials()->tagFadeColour(id_, &color))
			return color;
	}

	// UDMF
	if (parent_map_->currentFormat() == MapFormat::UDMF
		&& game::configuration().featureSupported(game::UDMFFeature::SectorFog))
	{
		int intcol = MapObject::intProperty("fadecolor");

		wxColour wxcol(intcol);
		color = ColRGBA(wxcol.Blue(), wxcol.Green(), wxcol.Red(), 0);
	}
	return color;
}

// -----------------------------------------------------------------------------
// Finds the 'text point' for the sector. This is a point within the sector that
// is reasonably close to the middle of the sector bbox while still being within
// the sector itself
// -----------------------------------------------------------------------------
void MapSector::findTextPoint()
{
	// Check if actual sector midpoint can be used
	text_point_ = getPoint(Point::Mid);
	if (containsPoint(text_point_))
		return;

	if (connected_sides_.empty())
		return;

	// Find nearest line to sector midpoint (that is also part of the sector)
	double   min_dist        = 9999999999.0;
	auto     mid_side        = connected_sides_[0];
	MapLine* mid_side_parent = mid_side->parentLine();
	for (auto& connected_side : connected_sides_)
	{
		auto   l    = connected_side->parentLine();
		double dist = geometry::distanceToLineFast(text_point_, l->seg());

		if (dist < min_dist)
		{
			min_dist        = dist;
			mid_side        = connected_side;
			mid_side_parent = l;
		}
	}

	// Calculate ray
	auto r_o = mid_side_parent->getPoint(Point::Mid);
	auto r_d = mid_side_parent->frontVector();
	if (mid_side == mid_side_parent->s1())
		r_d = { -r_d.x, -r_d.y };

	// Find nearest intersecting line
	min_dist = 9999999999.0;
	for (auto& connected_side : connected_sides_)
	{
		if (connected_side == mid_side)
			continue;

		auto   line = connected_side->parentLine();
		double dist = geometry::distanceRayLine(r_o, r_o + r_d, line->start(), line->end());

		if (dist > 0 && dist < min_dist)
			min_dist = dist;
	}

	// Set text point to halfway between the two lines
	text_point_ = { r_o.x + (r_d.x * min_dist * 0.5), r_o.y + (r_d.y * min_dist * 0.5) };
}

// -----------------------------------------------------------------------------
// Adds [side] to the list of 'connected sides'
// (sides that are part of this sector)
// -----------------------------------------------------------------------------
void MapSector::connectSide(MapSide* side)
{
	setModified();
	connected_sides_.push_back(side);
	poly_needsupdate_ = true;
	bbox_.reset();
	setGeometryUpdated();
}

// -----------------------------------------------------------------------------
// Removes [side] from the list of connected sides
// -----------------------------------------------------------------------------
void MapSector::disconnectSide(const MapSide* side)
{
	setModified();
	for (unsigned a = 0; a < connected_sides_.size(); a++)
	{
		if (connected_sides_[a] == side)
		{
			connected_sides_.erase(connected_sides_.begin() + a);
			break;
		}
	}

	poly_needsupdate_ = true;
	bbox_.reset();
	setGeometryUpdated();
}

void MapSector::addExtraFloor(const ExtraFloor& extra_floor, const MapSector& control_sector)
{
	extra_floors_.emplace_back(extra_floor);

	// Sort extra floors from top down
	std::sort(
		extra_floors_.begin(),
		extra_floors_.end(),
		[](const ExtraFloor& a, const ExtraFloor& b) { return b.effective_height < a.effective_height; });

	// Mark the sector as updated if the control sector has been; this is a sort of very rudimentary dependency graph
	if (control_sector.geometry_updated_ > geometry_updated_)
		setGeometryUpdated();
	if (control_sector.modifiedTime() > modifiedTime())
		setModified();
}

// -----------------------------------------------------------------------------
// Write all sector info to a Backup struct
// -----------------------------------------------------------------------------
void MapSector::writeBackup(Backup* backup)
{
	backup->props_internal[PROP_TEXFLOOR]      = floor_.texture;
	backup->props_internal[PROP_TEXCEILING]    = ceiling_.texture;
	backup->props_internal[PROP_HEIGHTFLOOR]   = floor_.height;
	backup->props_internal[PROP_HEIGHTCEILING] = ceiling_.height;
	backup->props_internal[PROP_LIGHTLEVEL]    = light_;
	backup->props_internal[PROP_SPECIAL]       = special_;
	backup->props_internal[PROP_ID]            = id_;
}

// -----------------------------------------------------------------------------
// Reads all sector info from a Backup struct
// -----------------------------------------------------------------------------
void MapSector::readBackup(Backup* backup)
{
	// Update texture counts (decrement previous)
	parent_map_->sectors().updateTexUsage(floor_.texture, -1);
	parent_map_->sectors().updateTexUsage(ceiling_.texture, -1);

	floor_.texture   = backup->props_internal.get<string>(PROP_TEXFLOOR);
	ceiling_.texture = backup->props_internal.get<string>(PROP_TEXCEILING);
	floor_.height    = backup->props_internal.get<int>(PROP_HEIGHTFLOOR);
	ceiling_.height  = backup->props_internal.get<int>(PROP_HEIGHTCEILING);
	floor_.plane.set(0, 0, 1, floor_.height);
	ceiling_.plane.set(0, 0, 1, ceiling_.height);
	light_   = backup->props_internal.get<int>(PROP_LIGHTLEVEL);
	special_ = backup->props_internal.get<int>(PROP_SPECIAL);
	id_      = backup->props_internal.get<int>(PROP_ID);

	// Update texture counts (increment new)
	parent_map_->sectors().updateTexUsage(floor_.texture, 1);
	parent_map_->sectors().updateTexUsage(ceiling_.texture, 1);

	// Update geometry info
	poly_needsupdate_ = true;
	bbox_.reset();
	setGeometryUpdated();
}

// -----------------------------------------------------------------------------
// Writes the sector as a UDMF text definition to [def]
// -----------------------------------------------------------------------------
void MapSector::writeUDMF(string& def)
{
	def = fmt::format("sector//#{}\n{{\n", index_);

	// Basic properties
	def += fmt::format("texturefloor=\"{}\";\ntextureceiling=\"{}\";\n", floor_.texture, ceiling_.texture);
	if (floor_.height != 0)
		def += fmt::format("heightfloor={};\n", floor_.height);
	if (ceiling_.height != 0)
		def += fmt::format("heightceiling={};\n", ceiling_.height);
	if (light_ != 160)
		def += fmt::format("lightlevel={};\n", light_);
	if (special_ != 0)
		def += fmt::format("special={};\n", special_);
	if (id_ != 0)
		def += fmt::format("id={};\n", id_);

	// For UDMF sector planes, ALL values must be added, or else GZDoom
	// will consider them invalid.
	// Check for UDMF floor planes, and if they are present, copy the
	// values from the existing floor planes, and remove the floor plane
	// properties so that they can be put in order.
	double floor_a = 0, floor_b = 0, floor_c = 0, floor_d = 0;
	bool   hasFloorPlane =
		(hasProp("floorplane_a") || hasProp("floorplane_b") || hasProp("floorplane_c") || hasProp("floorplane_d"));
	if (hasFloorPlane)
	{
		// The ABC values are internally negated to compensate for the
		// differences in the point height calculations between SLADE and
		// GZDoom
		floor_a = -floor_.plane.a;
		floor_b = -floor_.plane.b;
		floor_c = -floor_.plane.c;
		floor_d = floor_.plane.d;
		// Write the floor/ceiling plane properties in order later
		properties_.remove("floorplane_a");
		properties_.remove("floorplane_b");
		properties_.remove("floorplane_c");
		properties_.remove("floorplane_d");
	}
	// Do the same for the ceiling plane
	double ceiling_a = 0, ceiling_b = 0, ceiling_c = 0, ceiling_d = 0;
	bool   hasCeilingPlane =
		(hasProp("ceilingplane_a") || hasProp("ceilingplane_b") || hasProp("ceilingplane_c")
		 || hasProp("ceilingplane_d"));
	if (hasCeilingPlane)
	{
		ceiling_a = -ceiling_.plane.a;
		ceiling_b = -ceiling_.plane.b;
		ceiling_c = -ceiling_.plane.c;
		ceiling_d = ceiling_.plane.d;
		properties_.remove("ceilingplane_a");
		properties_.remove("ceilingplane_b");
		properties_.remove("ceilingplane_c");
		properties_.remove("ceilingplane_d");
	}

	// Other properties (that are not related to floor/ceiling planes
	if (!properties_.empty())
		def += properties_.toString(true, 3);

	// Write the floor and ceiling plane values in order
	if (hasFloorPlane)
	{
		def += fmt::format("floorplane_a = {};", floor_a);
		def += fmt::format("floorplane_b = {};", floor_b);
		def += fmt::format("floorplane_c = {};", floor_c);
		def += fmt::format("floorplane_d = {};", floor_d);
		// Persist between multiple saves
		properties_["floorplane_a"] = floor_a;
		properties_["floorplane_b"] = floor_b;
		properties_["floorplane_c"] = floor_c;
		properties_["floorplane_d"] = floor_d;
	}
	if (hasCeilingPlane)
	{
		def += fmt::format("ceilingplane_a = {};", ceiling_a);
		def += fmt::format("ceilingplane_b = {};", ceiling_b);
		def += fmt::format("ceilingplane_c = {};", ceiling_c);
		def += fmt::format("ceilingplane_d = {};", ceiling_d);
		// Persist between multiple saves
		properties_["ceilingplane_a"] = ceiling_a;
		properties_["ceilingplane_b"] = ceiling_b;
		properties_["ceilingplane_c"] = ceiling_c;
		properties_["ceilingplane_d"] = ceiling_d;
	}

	def += "}\n\n";
}

#ifndef NDEBUG
// -----------------------------------------------------------------------------
// Debuggable operator
// -----------------------------------------------------------------------------
MapSector::operator Debuggable() const
{
	if (!this)
		return { "<sector NULL>" };

	return { fmt::format("<sector {}>", index_) };
}
#endif
