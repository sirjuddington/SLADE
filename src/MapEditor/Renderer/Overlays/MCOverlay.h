#pragma once

namespace slade
{
// MCOverlay is a base class for full-screen map editor overlays
// that take input and mouse clicks when active
class MCOverlay
{
public:
	MCOverlay(bool allow_3d_mlook = false) : allow_3d_mlook_{ allow_3d_mlook } {}
	virtual ~MCOverlay() = default;

	bool isActive() const { return active_; }
	bool allow3dMlook() const { return allow_3d_mlook_; }

	virtual void update(long frametime) {}
	virtual void draw(int width, int height, float fade = 1.0f) {}
	virtual void close(bool cancel = false) {}
	virtual void mouseMotion(int x, int y) {}
	virtual void mouseLeftClick() {}
	virtual void mouseRightClick() {}
	virtual void keyDown(string_view key) {}

protected:
	bool active_         = true;
	bool allow_3d_mlook_ = false;
};
} // namespace slade
