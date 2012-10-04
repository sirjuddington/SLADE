
#include "Main.h"
#include "MapLine.h"
#include "MapVertex.h"
#include "MapSide.h"
#include "MathStuff.h"
#include "SLADEMap.h"
#include "MainApp.h"

MapLine::MapLine(SLADEMap* parent) : MapObject(MOBJ_LINE, parent) {
	// Init variables
	vertex1 = NULL;
	vertex2 = NULL;
	side1 = NULL;
	side2 = NULL;
	length = -1;
}

MapLine::MapLine(MapVertex* v1, MapVertex* v2, MapSide* s1, MapSide* s2, SLADEMap* parent) : MapObject(MOBJ_LINE, parent) {
	// Init variables
	vertex1 = v1;
	vertex2 = v2;
	side1 = s1;
	side2 = s2;
	length = -1;

	// Connect to vertices
	if (v1) v1->connectLine(this);
	if (v2) v2->connectLine(this);

	// Connect to sides
	if (s1) s1->parent = this;
	if (s2) s2->parent = this;
}

MapLine::~MapLine() {
	if (vertex1) vertex1->disconnectLine(this);
	if (vertex2) vertex2->disconnectLine(this);
}

MapSector* MapLine::frontSector() {
	if (side1)
		return side1->sector;
	else
		return NULL;
}

MapSector* MapLine::backSector() {
	if (side2)
		return side2->sector;
	else
		return NULL;
}

double MapLine::x1() {
	return vertex1->xPos();
}

double MapLine::y1() {
	return vertex1->yPos();
}

double MapLine::x2() {
	return vertex2->xPos();
}

double MapLine::y2() {
	return vertex2->yPos();
}

int MapLine::v1Index() {
	if (vertex1)
		return vertex1->getIndex();
	else
		return -1;
}

int MapLine::v2Index() {
	if (vertex2)
		return vertex2->getIndex();
	else
		return -1;
}

int MapLine::s1Index() {
	if (side1)
		return side1->getIndex();
	else
		return -1;
}

int MapLine::s2Index() {
	if (side2)
		return side2->getIndex();
	else
		return -1;
}

bool MapLine::boolProperty(string key) {
	if (key.StartsWith("side1.") && side1)
		return side1->boolProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2)
		return side2->boolProperty(key.Mid(6));
	else
		return MapObject::boolProperty(key);
}

int MapLine::intProperty(string key) {
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
	else
		return MapObject::intProperty(key);
}

double MapLine::floatProperty(string key) {
	if (key.StartsWith("side1.") && side1)
		return side1->floatProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2)
		return side2->floatProperty(key.Mid(6));
	else
		return MapObject::floatProperty(key);
}

string MapLine::stringProperty(string key) {
	if (key.StartsWith("side1.") && side1)
		return side1->stringProperty(key.Mid(6));
	else if (key.StartsWith("side2.") && side2)
		return side2->stringProperty(key.Mid(6));
	else
		return MapObject::stringProperty(key);
}

void MapLine::setBoolProperty(string key, bool value) {
	// Front side property
	if (key.StartsWith("side1.")) {
		if (side1)
			return side1->setBoolProperty(key.Mid(6), value);
	}

	// Back side property
	else if (key.StartsWith("side2.")) {
		if (side2)
			return side2->setBoolProperty(key.Mid(6), value);
	}

	// Line property
	else
		MapObject::setBoolProperty(key, value);
}

void MapLine::setIntProperty(string key, int value) {
	// Front side property
	if (key.StartsWith("side1.")) {
		if (side1)
			return side1->setIntProperty(key.Mid(6), value);
	}

	// Back side property
	else if (key.StartsWith("side2.")) {
		if (side2)
			return side2->setIntProperty(key.Mid(6), value);
	}

	// Line property
	else
		MapObject::setIntProperty(key, value);
}

void MapLine::setFloatProperty(string key, double value) {
	// Front side property
	if (key.StartsWith("side1.")) {
		if (side1)
			return side1->setFloatProperty(key.Mid(6), value);
	}

	// Back side property
	else if (key.StartsWith("side2.")) {
		if (side2)
			return side2->setFloatProperty(key.Mid(6), value);
	}

	// Line property
	else
		MapObject::setFloatProperty(key, value);
}

void MapLine::setStringProperty(string key, string value) {
	// Front side property
	if (key.StartsWith("side1.")) {
		if (side1)
			return side1->setStringProperty(key.Mid(6), value);
	}

	// Back side property
	else if (key.StartsWith("side2.")) {
		if (side2)
			return side2->setStringProperty(key.Mid(6), value);
	}

	// Line property
	else
		MapObject::setStringProperty(key, value);
}

void MapLine::setS1(MapSide* side) {
	if (!side1) {
		side1 = side;
		side->parent = this;
	}
}

void MapLine::setS2(MapSide* side) {
	if (!side2) {
		side2 = side;
		side->parent = this;
	}
}

fpoint2_t MapLine::midPoint() {
	return fpoint2_t(x1() + ((x2() - x1()) * 0.5), y1() + ((y2() - y1()) * 0.5));
}

double MapLine::getLength() {
	if (!vertex1 || !vertex2)
		return -1;

	if (length < 0) {
		length = MathStuff::distance(vertex1->xPos(), vertex1->yPos(), vertex2->xPos(), vertex2->yPos());
		ca = (vertex2->xPos() - vertex1->xPos()) / length;
		sa = (vertex2->yPos() - vertex1->yPos()) / length;
	}

	return length;
}

bool MapLine::doubleSector() {
	// Check both sides exist
	if (!side1 || !side2)
		return false;

	return (side1->getSector() == side2->getSector());
}

fpoint2_t MapLine::frontVector() {
	// Check if vector needs to be recalculated
	if (front_vec.x == 0 && front_vec.y == 0) {
		front_vec.set(-(vertex2->yPos() - vertex1->yPos()), vertex2->xPos() - vertex1->xPos());
		front_vec.normalize();
	}

	return front_vec;
}

fpoint2_t MapLine::dirTabPoint() {
	// Calculate midpoint
	fpoint2_t mid = midPoint();

	// Calculate tab length
	double tablen = getLength() * 0.1;
	if (tablen > 16) tablen = 16;
	if (tablen < 2) tablen = 2;

	// Calculate tab endpoint
	if (front_vec.x == 0 && front_vec.y == 0) frontVector();
	return fpoint2_t(mid.x - front_vec.x*tablen, mid.y - front_vec.y*tablen);
}

double MapLine::distanceTo(double x, double y) {
	// Update length data if needed
	if (length < 0) {
		length = MathStuff::distance(vertex1->xPos(), vertex1->yPos(), vertex2->xPos(), vertex2->yPos());
		ca = (vertex2->xPos() - vertex1->xPos()) / length;
		sa = (vertex2->yPos() - vertex1->yPos()) / length;
	}

	// Calculate intersection point
	double mx, ix, iy;
	mx = (-vertex1->xPos()+x)*ca + (-vertex1->yPos()+y)*sa;
	if (mx <= 0)		mx = 0.00001;				// Clip intersection to line (but not exactly on endpoints)
	else if (mx >= length)	mx = length - 0.00001;	// ^^
	ix = vertex1->xPos() + mx*ca;
	iy = vertex1->yPos() + mx*sa;

	// Calculate distance to line
	return sqrt((ix-x)*(ix-x) + (iy-y)*(iy-y));
}

int MapLine::needsTexture() {
	// Check line is valid
	if (!side1)
		return 0;

	// If line is 1-sided, it only needs front middle
	if (!side2)
		return TEX_FRONT_MIDDLE;

	// Get sector heights
	int floor_front = frontSector()->intProperty("heightfloor");
	int ceiling_front = frontSector()->intProperty("heightceiling");
	int floor_back = backSector()->intProperty("heightfloor");
	int ceiling_back = backSector()->intProperty("heightceiling");

	// Front lower
	int tex = 0;
	if (floor_front < floor_back)
		tex |= TEX_FRONT_LOWER;

	// Back lower
	else if (floor_front > floor_back)
		tex |= TEX_BACK_LOWER;

	// Front upper
	if (ceiling_front > ceiling_back)
		tex |= TEX_FRONT_UPPER;

	// Back upper
	else if (ceiling_front < ceiling_back)
		tex |= TEX_BACK_UPPER;

	return tex;
}

void MapLine::clearUnneededTextures() {
	// Check needed textures
	int tex = needsTexture();

	// Clear any unneeded textures
	if (side1) {
		if ((tex & TEX_FRONT_MIDDLE) == 0)
			setStringProperty("side1.texturemiddle", "-");
		if ((tex & TEX_FRONT_UPPER) == 0)
			setStringProperty("side1.texturetop", "-");
		if ((tex & TEX_FRONT_LOWER) == 0)
			setStringProperty("side1.texturebottom", "-");
	}
	if (side2) {
		if ((tex & TEX_BACK_MIDDLE) == 0)
			setStringProperty("side2.texturemiddle", "-");
		if ((tex & TEX_BACK_UPPER) == 0)
			setStringProperty("side2.texturetop", "-");
		if ((tex & TEX_BACK_LOWER) == 0)
			setStringProperty("side2.texturebottom", "-");
	}
}

void MapLine::resetInternals() {
	// Reset line internals
	length = -1;
	front_vec.set(0, 0);

	// Reset front sector internals
	MapSector* s1 = frontSector();
	if (s1) {
		s1->resetPolygon();
		s1->resetBBox();
	}

	// Reset back sector internals
	MapSector* s2 = backSector();
	if (s2) {
		s2->resetPolygon();
		s2->resetBBox();
	}

	modified_time = theApp->runTimer();
}

void MapLine::flip(bool sides) {
	// Flip vertices
	MapVertex* v = vertex1;
	vertex1 = vertex2;
	vertex2 = v;

	// Flip sides if needed
	if (sides) {
		MapSide* s = side1;
		side1 = side2;
		side2 = s;
	}

	modified_time = theApp->runTimer();
	resetInternals();
}
