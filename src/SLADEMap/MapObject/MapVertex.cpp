
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapVertex.cpp
// Description: MapVertex class, represents a vertex object in a map
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
#include "MapVertex.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/Parser.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// MapVertex Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapVertex class constructor
// -----------------------------------------------------------------------------
MapVertex::MapVertex(const Vec2d& pos) : MapObject(Type::Vertex), position_{ pos } {}

// -----------------------------------------------------------------------------
// MapVertex class constructor from UDMF definition
// -----------------------------------------------------------------------------
MapVertex::MapVertex(const Vec2d& pos, ParseTreeNode* udmf_def) : MapObject(Type::Vertex), position_{ pos }
{
	// Set properties from UDMF definition
	ParseTreeNode* prop;
	for (unsigned a = 0; a < udmf_def->nChildren(); a++)
	{
		prop = udmf_def->childPTN(a);

		// Skip required properties
		if (prop->name() == PROP_X || prop->name() == PROP_Y)
			continue;

		properties_[prop->name()] = prop->value();
	}
}

// -----------------------------------------------------------------------------
// Returns the object point [point].
// Currently for vertices this is always the vertex position
// -----------------------------------------------------------------------------
Vec2d MapVertex::getPoint(Point point)
{
	return position_;
}

// -----------------------------------------------------------------------------
// Moves the vertex to new a position [nx,ny]
// -----------------------------------------------------------------------------
void MapVertex::move(double nx, double ny)
{
	// Move the vertex
	setModified();
	position_.x = nx;
	position_.y = ny;

	// Reset all attached lines' geometry info
	for (auto& connected_line : connected_lines_)
		connected_line->resetInternals();

	parent_map_->setGeometryUpdated();
}

// -----------------------------------------------------------------------------
// Returns the value of the integer property matching [key]
// -----------------------------------------------------------------------------
int MapVertex::intProperty(string_view key)
{
	if (key == PROP_X)
		return (int)position_.x;
	else if (key == PROP_Y)
		return (int)position_.y;
	else
		return MapObject::intProperty(key);
}

// -----------------------------------------------------------------------------
// Returns the value of the float property matching [key]
// -----------------------------------------------------------------------------
double MapVertex::floatProperty(string_view key)
{
	if (key == PROP_X)
		return position_.x;
	else if (key == PROP_Y)
		return position_.y;
	else
		return MapObject::floatProperty(key);
}

// -----------------------------------------------------------------------------
// Sets the integer value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapVertex::setIntProperty(string_view key, int value)
{
	// Update modified time
	setModified();

	if (key == PROP_X)
	{
		position_.x = value;
		for (auto& connected_line : connected_lines_)
			connected_line->resetInternals();
	}
	else if (key == PROP_Y)
	{
		position_.y = value;
		for (auto& connected_line : connected_lines_)
			connected_line->resetInternals();
	}
	else
		return MapObject::setIntProperty(key, value);
}

// -----------------------------------------------------------------------------
// Sets the float value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapVertex::setFloatProperty(string_view key, double value)
{
	// Update modified time
	setModified();

	if (key == PROP_X)
		position_.x = value;
	else if (key == PROP_Y)
		position_.y = value;
	else
		return MapObject::setFloatProperty(key, value);
}

// -----------------------------------------------------------------------------
// Returns true if the property [key] can be modified via script
// -----------------------------------------------------------------------------
bool MapVertex::scriptCanModifyProp(string_view key)
{
	if (key == PROP_X || key == PROP_Y)
		return false;

	return true;
}

// -----------------------------------------------------------------------------
// Adds [line] to the list of lines connected to this vertex
// -----------------------------------------------------------------------------
void MapVertex::connectLine(MapLine* line)
{
	VECTOR_ADD_UNIQUE(connected_lines_, line);
}

// -----------------------------------------------------------------------------
// Removes [line] from the list of lines connected to this vertex
// -----------------------------------------------------------------------------
void MapVertex::disconnectLine(MapLine* line)
{
	for (unsigned a = 0; a < connected_lines_.size(); a++)
	{
		if (connected_lines_[a] == line)
		{
			connected_lines_.erase(connected_lines_.begin() + a);
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Returns the connected line at [index]
// -----------------------------------------------------------------------------
MapLine* MapVertex::connectedLine(unsigned index)
{
	if (index >= connected_lines_.size())
		return nullptr;

	return connected_lines_[index];
}

// -----------------------------------------------------------------------------
// Write all vertex info to a Backup struct
// -----------------------------------------------------------------------------
void MapVertex::writeBackup(Backup* backup)
{
	// Position
	backup->props_internal[PROP_X] = position_.x;
	backup->props_internal[PROP_Y] = position_.y;
}

// -----------------------------------------------------------------------------
// Read all vertex info from a Backup struct
// -----------------------------------------------------------------------------
void MapVertex::readBackup(Backup* backup)
{
	// Position
	position_.x = backup->props_internal.get<double>(PROP_X);
	position_.y = backup->props_internal.get<double>(PROP_Y);
}

// -----------------------------------------------------------------------------
// Writes the vertex as a UDMF text definition to [def]
// -----------------------------------------------------------------------------
void MapVertex::writeUDMF(string& def)
{
	def = fmt::format("vertex//#{}\n{{\n", index_);

	// Basic properties
	def += fmt::format("x={:1.3f};\ny={:1.3f};\n", position_.x, position_.y);

	// Other properties
	if (!properties_.empty())
		def += properties_.toString(true, 3);

	def += "}\n\n";
}
