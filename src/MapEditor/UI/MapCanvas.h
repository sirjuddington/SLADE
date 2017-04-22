
#ifndef __MAPCANVAS_H__
#define __MAPCANVAS_H__

#include "common.h"
#include "General/KeyBind.h"
#include "General/SAction.h"
#include "MapEditor/Renderer/Overlays/InfoOverlay3d.h"
#include "MapEditor/Renderer/Overlays/LineInfoOverlay.h"
#include "MapEditor/Renderer/Overlays/SectorInfoOverlay.h"
#include "MapEditor/Renderer/Overlays/ThingInfoOverlay.h"
#include "MapEditor/Renderer/Overlays/VertexInfoOverlay.h"
#include "UI/Canvas/OGLCanvas.h"
#include "MapEditor/MapEditContext.h"

class MCAnimation;
class MapSide;
class ThingType;
class GLTexture;
class MapThing;
class MapCanvas : public OGLCanvas, public KeyBindHandler, public SActionHandler
{
private:
	MapEditContext*			editor;
	vector<MCAnimation*>	animations;
	int						frametime_last;
	vector<int>				fps_avg;
	int						fr_idle;
	sf::Clock				sfclock;
	double					mwheel_rotation;

	// Mouse stuff
	bool		mouse_selbegin;
	bool		mouse_movebegin;
	bool		mouse_locked;
	bool		mouse_warp;

	// Info overlays
	int					last_hilight;
	int					last_hilight_type;
	VertexInfoOverlay	info_vertex;
	LineInfoOverlay		info_line;
	SectorInfoOverlay	info_sector;
	ThingInfoOverlay	info_thing;
	InfoOverlay3D		info_3d;

	// View properties
	fpoint2_t	view_tl;
	fpoint2_t	view_br;

	// Animation
	float	anim_flash_level;
	bool	anim_flash_inc;
	float	anim_info_fade;
	bool	anim_info_show;
	float	anim_overlay_fade;
	double	anim_view_speed;
	float	anim_move_fade;
	float	fade_vertices;
	float	fade_things;
	float	fade_flats;
	float	fade_lines;
	float	anim_help_fade;

public:
	MapCanvas(wxWindow* parent, int id, MapEditContext* editor);
	~MapCanvas();

	bool	helpActive();

	// Drawing
	void	drawMap2d();
	void	drawMap3d();
	void	draw();

	// Frame updates
	bool	update2d(double mult);
	bool	update3d(double mult);
	void	update(long frametime);

	// Mouse
	void	mouseToCenter();
	void	lockMouse(bool lock);
	void	mouseLook3d();

	void	animateSelectionChange(const MapEditor::Item& item, bool selected = true);
	void 	animateSelectionChange(const ItemSelection& selection);
	void	updateInfoOverlay();
	void	forceRefreshRenderer();

	// Overlays
	//void 	openSectorTextureOverlay(vector<MapSector*>& sectors);

	// Keybind handling
	void	onKeyBindPress(string name);

	// SAction handler
	bool	handleAction(string id);

	// Events
	void	onSize(wxSizeEvent& e);
	void	onKeyDown(wxKeyEvent& e);
	void	onKeyUp(wxKeyEvent& e);
	void	onMouseDown(wxMouseEvent& e);
	void	onMouseUp(wxMouseEvent& e);
	void	onMouseMotion(wxMouseEvent& e);
	void	onMouseWheel(wxMouseEvent& e);
	void	onMouseLeave(wxMouseEvent& e);
	void	onMouseEnter(wxMouseEvent& e);
	void	onIdle(wxIdleEvent& e);
	void	onRTimer(wxTimerEvent& e);
	void	onFocus(wxFocusEvent& e);

	// Temporary until MapRenderer* redux
	float	lineFadeLevel() const { return fade_lines; }
};

#endif //__MAPCANVAS_H__
