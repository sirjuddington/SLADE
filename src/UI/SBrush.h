#pragma once
#ifndef __SBRUSH_H__
#define __SBRUSH_H__

#include "common.h"

class SAction;
class SImage;
class SBrush
{
private:
	// The cursor graphic
	SImage * image;
	string name;
	string icon;
	int cx, cy;
public:
	// Constructor
	SBrush(string name);
	// Destructor
	~SBrush();

	// Returns an action ready to be inserted in a menu or toolbar
	SAction getAction();

	// Returns the brush's name ("pgfx_brush_xyz")
	string getName() { return name; }

	// Returns the brush's icon name ("brush_xyz")
	string getIcon() { return icon; }

	// Returns intensity of how much this pixel is affected by the brush; [0, 0] is the brush's center
	uint8_t getPixel(int x, int y);
};

class SBrushManager
{
private:
	// The collection of SBrushes
	vector<SBrush *> brushes;

	// Single-instance implementation
	static SBrushManager* instance;
public:
	// Constructor
	SBrushManager();
	// Destructor
	~SBrushManager();

	// Get a brush from its name
	SBrush* get(string name);

	// Add a brush
	void add(SBrush* brush);

	// Single-instance implementation
	static SBrushManager* getInstance()
	{
		if (!instance)
			instance = new SBrushManager();
		return instance;
	}

	// Init brushes
	static bool initBrushes();
};
#define theBrushManager SBrushManager::getInstance()

#endif // __SBRUSH_H__
