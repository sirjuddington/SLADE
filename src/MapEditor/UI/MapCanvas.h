
#ifndef __MAPCANVAS_H__
#define __MAPCANVAS_H__

#include "common.h"
#include "General/KeyBind.h"
#include "General/SAction.h"
#include "MapEditor/Renderer/MapRenderer3D.h"
#include "MapEditor/Renderer/Overlays/InfoOverlay3d.h"
#include "MapEditor/Renderer/Overlays/LineInfoOverlay.h"
#include "MapEditor/Renderer/Overlays/MCOverlay.h"
#include "MapEditor/Renderer/Overlays/SectorInfoOverlay.h"
#include "MapEditor/Renderer/Overlays/ThingInfoOverlay.h"
#include "MapEditor/Renderer/Overlays/VertexInfoOverlay.h"
#include "UI/Canvas/OGLCanvas.h"

class MapEditor;
class MCAnimation;
class MapSide;
class ThingType;
class GLTexture;
class MapRenderer2D;
class MapThing;
class MapCanvas : public OGLCanvas, public KeyBindHandler, public SActionHandler
{
private:
	MapEditor*				editor;
	vector<MCAnimation*>	animations;
	int						frametime_last;
	vector<int>				fps_avg;
	int						fr_idle;
	sf::Clock				sfclock;
	MapRenderer2D*			renderer_2d;
	MapRenderer3D*			renderer_3d;
	int						modifiers_current;
	int						mode_last;
	double					mwheel_rotation;

	// Mouse stuff
	enum
	{
		MSTATE_NORMAL = 0,
		MSTATE_SELECTION,
		//MSTATE_PANNING,
		MSTATE_MOVE,
		MSTATE_THING_ANGLE,
		MSTATE_LINE_DRAW,
		MSTATE_EDIT,
		MSTATE_PASTE,
		MSTATE_TAG_SECTORS,
		MSTATE_TAG_THINGS,

		DSTATE_LINE = 0,
		DSTATE_SHAPE_ORIGIN,
		DSTATE_SHAPE_EDGE,

		ESTATE_NONE = 0,
		ESTATE_MOVE,
		ESTATE_SIZE_L,
		ESTATE_SIZE_R,
		ESTATE_SIZE_T,
		ESTATE_SIZE_B,
		ESTATE_SIZE_TL,
		ESTATE_SIZE_BL,
		ESTATE_SIZE_TR,
		ESTATE_SIZE_BR,
	};
	point2_t	mouse_pos;			// 'Raw' mouse position
	point2_t	mouse_downpos;
	fpoint2_t	mouse_pos_m;		// 'Map' mouse position (translated)
	fpoint2_t	mouse_downpos_m;
	uint8_t		mouse_state;
	bool		zooming_cursor;
	bool		mouse_selbegin;
	bool		mouse_movebegin;
	int			draw_state;
	bool		mouse_locked;
	bool		mouse_warp;
	int			edit_state;
	bool		edit_rotate;
	bool		panning;

	// Info overlays
	int					last_hilight;
	int					last_hilight_type;
	VertexInfoOverlay	info_vertex;
	LineInfoOverlay		info_line;
	SectorInfoOverlay	info_sector;
	ThingInfoOverlay	info_thing;
	InfoOverlay3D		info_3d;
	MCOverlay*			overlay_current;

	// View properties
	double		view_xoff;
	double		view_yoff;
	double		view_scale;
	fpoint2_t	view_tl;
	fpoint2_t	view_br;

	// Animation
	float	anim_flash_level;
	bool	anim_flash_inc;
	float	anim_info_fade;
	bool	anim_info_show;
	float	anim_overlay_fade;
	double	anim_view_speed;
	double	view_xoff_inter;
	double	view_yoff_inter;
	double	view_scale_inter;
	float	anim_move_fade;
	float	fade_vertices;
	float	fade_things;
	float	fade_flats;
	float	fade_lines;
	float	anim_help_fade;

	// Feature help text
	vector<string>	feature_help_lines;

public:
	MapCanvas(wxWindow* parent, int id, MapEditor* editor);
	~MapCanvas();

	bool	overlayActive();
	bool	helpActive();

	double	translateX(double x, bool inter = false);
	double	translateY(double y, bool inter = false);
	int		screenX(double x);
	int		screenY(double y);
	void	setTopY(double y);
	void	setOverlayCoords(bool set = true);

	void	setView(double x, double y);
	void	pan(double x, double y);
	void	zoom(double amount, bool toward_cursor = false);
	void	viewFitToMap(bool snap = false);
	void	viewShowObject();
	void	viewMatchSpot(double mx, double my, double sx, double sy);

	// 3d mode
	void		set3dCameraThing(MapThing* thing);
	fpoint2_t	get3dCameraPos();
	fpoint2_t	get3dCameraDir();

	// Drawing
	void	drawGrid();
	void	drawEditorMessages();
	void	drawFeatureHelpText();
	void	drawSelectionNumbers();
	void	drawThingQuickAngleLines();
	void	drawLineLength(fpoint2_t p1, fpoint2_t p2, rgba_t col);
	void	drawLineDrawLines();
	void	drawPasteLines();
	void	drawObjectEdit();
	void	drawMap2d();
	void	drawMap3d();
	void	draw();

	// Frame updates
	bool	update2d(double mult);
	bool	update3d(double mult);
	void	update(long frametime);

	// Mouse
	void		mouseToCenter();
	void		lockMouse(bool lock);
	void		determineObjectEditState();
	fpoint2_t	mouseDownPosM() { return mouse_downpos_m; }
	void		mouseLook3d();

	void	itemSelected(int index, bool selected = true);
	void	itemsSelected(vector<int>& items, bool selected = true);
	void	itemSelected3d(selection_3d_t item, bool selected = true);
	void	itemsSelected3d(vector<selection_3d_t>& items, bool selected = true);
	void	updateInfoOverlay();
	void	forceRefreshRenderer();
	void	changeEditMode(int mode);
	void	changeFlatType(int type);
	void	editObjectProperties(vector<MapObject*>& objects);
	void	beginLineDraw();
	void	beginShapeDraw();
	void	beginObjectEdit();

	// Keybind/action handlers (that use UI features - anything else goes to MapEditor)
	void	changeThingType();
	void	changeSectorTexture();
	void	changeThingType3d(selection_3d_t first);
	void	changeTexture3d(selection_3d_t first);

	// Keybind handling
	void	onKeyBindPress(string name);
	void	keyBinds2dView(string name);
	void	keyBinds2d(string name);
	void	keyBinds3d(string name);
	void	onKeyBindRelease(string name);

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
};

#endif //__MAPCANVAS_H__
