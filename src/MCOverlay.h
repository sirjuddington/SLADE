
#ifndef __MC_OVERLAY_H__
#define __MC_OVERLAY_H__

class MCOverlay {
private:

protected:
	bool	active;
	bool	allow_3d_mlook;

public:
	MCOverlay() { active = true; allow_3d_mlook = false; }
	~MCOverlay() {}

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
