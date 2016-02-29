
#ifndef __MC_OVERLAY_H__
#define __MC_OVERLAY_H__

// MCOverlay is a base class for full-screen map editor overlays
// that take input and mouse clicks when active

class MCOverlay
{
private:

protected:
	bool	active;
	bool	allow_3d_mlook;

public:
	MCOverlay() { active = true; allow_3d_mlook = false; }
	virtual ~MCOverlay() {}

	bool	isActive() { return active; }
	bool	allow3dMlook() { return allow_3d_mlook; }

	virtual void	update(long frametime) {}
	virtual void	draw(int width, int height, float fade = 1.0f) {}
	virtual void	close(bool cancel = false) {}
	virtual void	mouseMotion(int x, int y) {}
	virtual void	mouseLeftClick() {}
	virtual void	mouseRightClick() {}
	virtual void	keyDown(string key) {}
};

#endif//__MC_OVERLAY_H__
