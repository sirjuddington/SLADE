
#include "Main.h"
#include "MapThing.h"
#include "MainApp.h"

MapThing::MapThing(SLADEMap* parent) : MapObject(MOBJ_THING, parent) {
	// Init variables
	this->x = 0;
	this->y = 0;
	this->type = 1;
}

MapThing::MapThing(double x, double y, short type, SLADEMap* parent) : MapObject(MOBJ_THING, parent) {
	// Init variables
	this->x = x;
	this->y = y;
	this->type = type;
}

MapThing::~MapThing() {
}

int MapThing::intProperty(string key) {
	if (key == "type")
		return type;
	else if (key == "x")
		return (int)x;
	else if (key == "y")
		return (int)y;
	else
		return MapObject::intProperty(key);
}

double MapThing::floatProperty(string key) {
	if (key == "x")
		return x;
	else if (key == "y")
		return y;
	else
		return MapObject::floatProperty(key);
}

void MapThing::setIntProperty(string key, int value) {
	if (key == "type")
		type = value;
	else if (key == "x")
		x = value;
	else if (key == "y")
		y = value;
	else
		MapObject::setIntProperty(key, value);

	// Update modified time
	modified_time = theApp->runTimer();
}

void MapThing::setFloatProperty(string key, double value) {
	if (key == "x")
		x = value;
	else if (key == "y")
		y = value;
	else
		MapObject::setFloatProperty(key, value);

	// Update modified time
	modified_time = theApp->runTimer();
}

void MapThing::copy(MapObject* c) {
	// Don't copy a non-thing
	if (c->getObjType() != MOBJ_THING)
		return;

	// Basic variables
	MapThing* thing = (MapThing*)c;
	this->x = thing->x;
	this->y = thing->y;
	this->type = thing->type;

	// Other properties
	MapObject::copy(c);
}

void MapThing::setAnglePoint(fpoint2_t point) {
	// Calculate direction vector
	fpoint2_t vec(point.x - x, point.y - y);
	double mag = sqrt((vec.x * vec.x) + (vec.y * vec.y));
	double x = vec.x / mag;
	double y = vec.y / mag;

	// Determine angle
	int angle = 0;
	if (x > 0.89)				// east
		angle = 0;
	else if (x < -0.89)			// west
		angle = 180;
	else if (y > 0.89)			// north
		angle = 90;
	else if (y < -0.89)			// south
		angle = 270;
	else if (x > 0 && y > 0)	// northeast
		angle = 45;
	else if (x < 0 && y > 0)	// northwest
		angle = 135;
	else if (x < 0 && y < 0)	// southwest
		angle = 225;
	else if (x > 0 && y < 0)	// southeast
		angle = 315;

	// Set thing angle
	setIntProperty("angle", angle);
}
