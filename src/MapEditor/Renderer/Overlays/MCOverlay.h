#pragma once

// MCOverlay is a base class for full-screen map editor overlays
// that take input and mouse clicks when active

class MCOverlay
{
public:
	MCOverlay()
	{
		active_         = true;
		allow_3d_mlook_ = false;
	}
	virtual ~MCOverlay() {}

	bool isActive() { return active_; }
	bool allow3dMlook() { return allow_3d_mlook_; }

	virtual void update(long frametime) {}
	virtual void draw(int width, int height, float fade = 1.0f) {}
	virtual void close(bool cancel = false) {}
	virtual void mouseMotion(int x, int y) {}
	virtual void mouseLeftClick() {}
	virtual void mouseRightClick() {}
	virtual void keyDown(string key) {}

protected:
	bool active_;
	bool allow_3d_mlook_;
};
