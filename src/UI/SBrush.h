#pragma once

class SAction;
class SImage;

class SBrush
{
public:
	SBrush(string name);
	~SBrush();

	// SAction getAction(); // Returns an action ready to be inserted in a menu or toolbar (NYI)

	string  name() { return name_; } // Returns the brush's name ("pgfx_brush_xyz")
	string  icon() { return icon_; } // Returns the brush's icon name ("brush_xyz")
	uint8_t pixel(int x, int y);

private:
	SImage*  image_; // The cursor graphic
	string   name_;
	string   icon_;
	Vec2i center_;
};

class SBrushManager
{
public:
	SBrushManager();
	~SBrushManager();

	SBrush* get(string name);
	void    add(SBrush* brush);

	// Single-instance implementation
	static SBrushManager* getInstance()
	{
		if (!instance_)
			instance_ = new SBrushManager();
		return instance_;
	}

	static bool initBrushes();

private:
	vector<SBrush*> brushes_; // The collection of SBrushes

	static SBrushManager* instance_; // Single-instance implementation
};

#define theBrushManager SBrushManager::getInstance()
