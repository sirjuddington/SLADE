
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapVertex.cpp
 * Description: MapVertex class, represents a vertex object in a map
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
#include "MapVertex.h"
#include "MapLine.h"
#include "App.h"


/*******************************************************************
 * MAPVERTEX CLASS FUNCTIONS
 *******************************************************************/

/* MapVertex::MapVertex
 * MapVertex class constructor
 *******************************************************************/
MapVertex::MapVertex(SLADEMap* parent) : MapObject(MOBJ_VERTEX, parent)
{
	// Init variables
	this->x = 0;
	this->y = 0;
}

/* MapVertex::MapVertex
 * MapVertex class constructor
 *******************************************************************/
MapVertex::MapVertex(double x, double y, SLADEMap* parent) : MapObject(MOBJ_VERTEX, parent)
{
	// Init variables
	this->x = x;
	this->y = y;
}

/* MapVertex::~MapVertex
 * MapVertex class constructor
 *******************************************************************/
MapVertex::~MapVertex()
{
}

/* MapVertex::getPoint
 * Returns the object point [point]. Currently for vertices this is
 * always the vertex position
 *******************************************************************/
fpoint2_t MapVertex::getPoint(uint8_t point)
{
	return this->point();
}

/* MapVertex::point
 * Returns the vertex position, more explicitly than the overridden
 * getPoint
 *******************************************************************/
fpoint2_t MapVertex::point()
{
	return fpoint2_t(x, y);
}

/* MapVertex::intProperty
 * Returns the value of the integer property matching [key]
 *******************************************************************/
int MapVertex::intProperty(const string& key)
{
	if (key == "x")
		return (int)x;
	else if (key == "y")
		return (int)y;
	else
		return MapObject::intProperty(key);
}

/* MapVertex::floatProperty
 * Returns the value of the float property matching [key]
 *******************************************************************/
double MapVertex::floatProperty(const string& key)
{
	if (key == "x")
		return x;
	else if (key == "y")
		return y;
	else
		return MapObject::floatProperty(key);
}

/* MapVertex::setIntProperty
 * Sets the integer value of the property [key] to [value]
 *******************************************************************/
void MapVertex::setIntProperty(const string& key, int value)
{
	// Update modified time
	setModified();

	if (key == "x")
	{
		x = value;
		for (unsigned a = 0; a < connected_lines.size(); a++)
			connected_lines[a]->resetInternals();
	}
	else if (key == "y")
	{
		y = value;
		for (unsigned a = 0; a < connected_lines.size(); a++)
			connected_lines[a]->resetInternals();
	}
	else
		return MapObject::setIntProperty(key, value);
}

/* MapVertex::setFloatProperty
 * Sets the float value of the property [key] to [value]
 *******************************************************************/
void MapVertex::setFloatProperty(const string& key, double value)
{
	// Update modified time
	setModified();

	if (key == "x")
		x = value;
	else if (key == "y")
		y = value;
	else
		return MapObject::setFloatProperty(key, value);
}

/* MapVertex::scriptCanModifyProp
 * Returns true if the property [key] can be modified via script
 *******************************************************************/
bool MapVertex::scriptCanModifyProp(const string& key)
{
	if (key == "x" || key == "y")
		return false;

	return true;
}

/* MapVertex::connectLine
 * Adds [line] to the list of lines connected to this vertex
 *******************************************************************/
void MapVertex::connectLine(MapLine* line)
{
	VECTOR_ADD_UNIQUE(connected_lines, line);
}

/* MapVertex::disconnectLine
 * Removes [line] from the list of lines connected to this vertex
 *******************************************************************/
void MapVertex::disconnectLine(MapLine* line)
{
	for (unsigned a = 0; a < connected_lines.size(); a++)
	{
		if (connected_lines[a] == line)
		{
			connected_lines.erase(connected_lines.begin() + a);
			return;
		}
	}
}

/* MapVertex::connectedLine
 * Returns the connected line at [index]
 *******************************************************************/
MapLine* MapVertex::connectedLine(unsigned index)
{
	if (index >= connected_lines.size())
		return nullptr;

	return connected_lines[index];
}

/* MapVertex::writeBackup
 * Write all vertex info to a mobj_backup_t struct
 *******************************************************************/
void MapVertex::writeBackup(mobj_backup_t* backup)
{
	// Position
	backup->props_internal["x"] = x;
	backup->props_internal["y"] = y;
}

/* MapVertex::readBackup
 * Read all vertex info from a mobj_backup_t struct
 *******************************************************************/
void MapVertex::readBackup(mobj_backup_t* backup)
{
	// Position
	x = backup->props_internal["x"].getFloatValue();
	y = backup->props_internal["y"].getFloatValue();
}
