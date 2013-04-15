
#include "Main.h"
#include "MapVertex.h"
#include "MainApp.h"

MapVertex::MapVertex(SLADEMap* parent) : MapObject(MOBJ_VERTEX, parent) {
	// Init variables
	this->x = 0;
	this->y = 0;
}

MapVertex::MapVertex(double x, double y, SLADEMap* parent) : MapObject(MOBJ_VERTEX, parent) {
	// Init variables
	this->x = x;
	this->y = y;
}

MapVertex::~MapVertex() {
}

int MapVertex::intProperty(string key) {
	if (key == "x")
		return (int)x;
	else if (key == "y")
		return (int)y;
	else
		return MapObject::intProperty(key);
}

double MapVertex::floatProperty(string key) {
	if (key == "x")
		return x;
	else if (key == "y")
		return y;
	else
		return MapObject::floatProperty(key);
}

void MapVertex::setIntProperty(string key, int value) {
	// Update modified time
	setModified();

	if (key == "x")
		x = value;
	else if (key == "y")
		y = value;
	else
		return MapObject::setIntProperty(key, value);
}

void MapVertex::setFloatProperty(string key, double value) {
	// Update modified time
	setModified();

	if (key == "x")
		x = value;
	else if (key == "y")
		y = value;
	else
		return MapObject::setFloatProperty(key, value);
}

void MapVertex::connectLine(MapLine* line) {
	for (unsigned a = 0; a < connected_lines.size(); a++) {
		if (connected_lines[a] == line)
			return;
	}

	connected_lines.push_back(line);
}

void MapVertex::disconnectLine(MapLine* line) {
	for (unsigned a = 0; a < connected_lines.size(); a++) {
		if (connected_lines[a] == line) {
			connected_lines.erase(connected_lines.begin() + a);
			return;
		}
	}
}

MapLine* MapVertex::connectedLine(unsigned index) {
	if (index > connected_lines.size())
		return NULL;

	return connected_lines[index];
}

void MapVertex::writeBackup(mobj_backup_t* backup) {
	// Position
	backup->props_internal["x"] = x;
	backup->props_internal["y"] = y;
}

void MapVertex::readBackup(mobj_backup_t* backup) {
	// Position
	x = backup->props_internal["x"].getFloatValue();
	y = backup->props_internal["y"].getFloatValue();
}
