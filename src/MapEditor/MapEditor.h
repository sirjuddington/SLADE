
#ifndef __MAP_EDITOR_H__
#define __MAP_EDITOR_H__

#include "SLADEMap/SLADEMap.h"
#include "ObjectEdit.h"
#include "GameConfiguration/GameConfiguration.h"

struct selection_3d_t
{
	int		index;
	uint8_t	type;

	selection_3d_t(int index = -1, uint8_t type = 0)
	{
		this->index = index;
		this->type = type;
	}

	bool operator<(const selection_3d_t& other) const {
		if (this->type == other.type)
			return this->index < other.index;
		else
			return this->type < other.type;
	}
};

class MapCanvas;
class UndoManager;
class MapObjectCreateDeleteUS;
class MapEditor
{
private:
	SLADEMap			map;
	MapCanvas*			canvas;

	// Undo/Redo stuff
	UndoManager*				undo_manager;
	UndoManager*				undo_manager_3d;
	MapObjectCreateDeleteUS*	us_create_delete;

	// Editor state
	uint8_t		edit_mode;
	int			hilight_item;
	bool		hilight_locked;
	vector<int>	selection;
	int			gridsize;
	int			sector_mode;
	bool		grid_snap;
	bool		link_3d_light;
	bool		link_3d_offset;
	int			current_tag;

	// Undo/Redo
	bool	undo_modified;
	bool	undo_created;
	bool	undo_deleted;
	string	last_undo_level;

	// Tagged items
	vector<MapSector*>	tagged_sectors;
	vector<MapLine*>	tagged_lines;
	vector<MapThing*>	tagged_things;

	// Tagging items
	vector<MapLine*>	tagging_lines;
	vector<MapThing*>	tagging_things;

	// Pathed things
	vector<MapThing*>	pathed_things;

	// Moving
	fpoint2_t	move_origin;
	fpoint2_t	move_vec;
	vector<int>	move_items;
	int			move_item_closest;

	// Object edit
	ObjectEditGroup	edit_object_group;

	// Object properties and copy/paste
	MapThing*	copy_thing;
	MapSector*	copy_sector;
	MapLine*	copy_line;
	string		copy_texture;
	double		copy_offsets[2];

	// Drawing
	vector<fpoint2_t>	draw_points;
	fpoint2_t			draw_origin;

	// Editor messages
	struct editor_msg_t
	{
		string	message;
		long	act_time;
	};
	vector<editor_msg_t>	editor_messages;

	void migrateSelection(int old_edit_mode, vector<int>& old_selection, vector<selection_3d_t>& old_selection_3d);

	// 3d mode
	selection_3d_t			hilight_3d;
	vector<selection_3d_t>	selection_3d;

	// Player start swap
	fpoint2_t	player_start_pos;
	int			player_start_dir;

	// Helper for selectAdjacent3d
	bool wallMatches(MapSide* side, uint8_t part, string tex);
	void getAdjacentWalls3d(selection_3d_t item, vector<selection_3d_t>& list);
	void getAdjacentFlats3d(selection_3d_t item, vector<selection_3d_t>& list);
	void getAdjacent3d(selection_3d_t item, vector<selection_3d_t>& list);

	// Helper for autoAlignX3d
	void doAlignX3d(MapSide* side, int offset, string tex, vector<selection_3d_t>& walls_done, int tex_width);

	void mergeLines(long, vector<fpoint2_t>&);

public:
	enum
	{
		// Editor modes
		MODE_VERTICES = 0,
		MODE_LINES,
		MODE_SECTORS,
		MODE_THINGS,
		MODE_3D,

		// Sector edit modes (for shortcut keys, mostly)
		SECTOR_BOTH = 0,
		SECTOR_FLOOR,
		SECTOR_CEILING,

		// Selection
		DESELECT = 0,
		SELECT,
		TOGGLE,

		// 3d mode selection type
		SEL_SIDE_TOP = 0,
		SEL_SIDE_MIDDLE,
		SEL_SIDE_BOTTOM,
		SEL_FLOOR,
		SEL_CEILING,
		SEL_THING,

		// Copy/paste types
		COPY_TEXTYPE = 0,
		COPY_OFFSETS,
		COPY_SCALE,
	};

	MapEditor();
	~MapEditor();

	SLADEMap&			getMap() { return map; }
	uint8_t				editMode() { return edit_mode; }
	int					sectorEditMode() { return sector_mode; }
	double				gridSize();
	unsigned			selectionSize() { return selection.size(); }
	vector<int>&		getSelection() { return selection; }
	int					hilightItem() { return hilight_item; }
	vector<MapSector*>&	taggedSectors() { return tagged_sectors; }
	vector<MapLine*>&	taggedLines() { return tagged_lines; }
	vector<MapThing*>&	taggedThings() { return tagged_things; }
	vector<MapLine*>&	taggingLines() { return tagging_lines; }
	vector<MapThing*>&	taggingThings() { return tagging_things; }
	vector<MapThing*>&	pathedThings() { return pathed_things; }
	bool				hilightLocked() { return hilight_locked; }
	void				lockHilight(bool lock = true) { hilight_locked = lock; }
	bool				gridSnap() { return grid_snap; }
	UndoManager*		undoManager() { return undo_manager; }

	vector<selection_3d_t>&	get3dSelection() { return selection_3d; }
	bool					set3dHilight(selection_3d_t hl);
	selection_3d_t			hilightItem3d() { return hilight_3d; }
	void					get3dSelectionOrHilight(vector<selection_3d_t>& list);

	void	setEditMode(int mode);
	void	setSectorEditMode(int mode);
	void	setCanvas(MapCanvas* canvas) { this->canvas = canvas; }

	// Map loading
	bool	openMap(Archive::mapdesc_t map);
	void	clearMap();

	// Selection/hilight
	void		clearHilight() { if (!hilight_locked) hilight_item = -1; }
	bool		updateHilight(fpoint2_t mouse_pos, double dist_scale);
	void		updateTagged();
	void		selectionUpdated();
	void		clearSelection(bool animate = true);
	void		selectAll();
	bool		selectCurrent(bool clear_none = true);
	bool		selectWithin(double xmin, double ymin, double xmax, double ymax, bool add = false);
	MapVertex*	getHilightedVertex();
	MapLine*	getHilightedLine();
	MapSector*	getHilightedSector();
	MapThing*	getHilightedThing();
	MapObject*	getHilightedObject();
	void		getSelectedVertices(vector<MapVertex*>& list);
	void		getSelectedLines(vector<MapLine*>& list);
	void		getSelectedSectors(vector<MapSector*>& list);
	void		getSelectedThings(vector<MapThing*>& list);
	void		getSelectedObjects(vector<MapObject*>& list);
	void		showItem(int index);
	bool		isHilightOrSelection() { return !selection.empty() || hilight_item != -1; }
	void		selectItem3d(selection_3d_t item, int sel = TOGGLE);

	// Grid
	void	incrementGrid();
	void	decrementGrid();
	double	snapToGrid(double position, bool force = true);
	fpoint2_t relativeSnapToGrid(fpoint2_t origin, fpoint2_t mouse_pos);

	// Item moving
	vector<int>&	movingItems() { return move_items; }
	fpoint2_t		moveVector() { return move_vec; }
	bool			beginMove(fpoint2_t mouse_pos);
	void			doMove(fpoint2_t mouse_pos);
	void			endMove(bool accept = true);

	// Editing
	void	copyProperties(MapObject* object = NULL);
	void	pasteProperties();
	void	splitLine(double x, double y, double min_dist = 64);
	void	flipLines(bool sides = true);
	void	correctLineSectors();
	void	changeSectorHeight(int amount, bool floor = true, bool ceiling = true);
	void	changeSectorLight(bool up, bool fine);
	void	joinSectors(bool remove_lines);
	void	changeThingType(int newtype);
	void	thingQuickAngle(fpoint2_t mouse_pos);
	void	mirror(bool x_axis);

	// Tag edit
	int		beginTagEdit();
	void	tagSectorAt(double x, double y);
	void	endTagEdit(bool accept = true);

	// Object creation/deletion
	void	createObject(double x, double y);
	void	createVertex(double x, double y);
	void	createThing(double x, double y);
	void	createSector(double x, double y);
	void	deleteObject();

	// Line drawing
	unsigned	nLineDrawPoints() { return draw_points.size(); }
	fpoint2_t	lineDrawPoint(unsigned index);
	bool		addLineDrawPoint(fpoint2_t point, bool nearest = false);
	void		removeLineDrawPoint();
	void		setShapeDrawOrigin(fpoint2_t point, bool nearest = false);
	void		updateShapeDraw(fpoint2_t point);
	void		endLineDraw(bool apply = true);

	// Object edit
	ObjectEditGroup*	getObjectEditGroup() { return &edit_object_group; }
	bool				beginObjectEdit();
	void				endObjectEdit(bool accept);

	// Copy/paste
	void	copy();
	void	paste(fpoint2_t mouse_pos);

	// 3d mode
	void	selectAdjacent3d(selection_3d_t item);
	void	changeSectorLight3d(int amount);
	void	changeOffset3d(int amount, bool x);
	void	changeSectorHeight3d(int amount);
	void	autoAlignX3d(selection_3d_t start);
	void	resetOffsets3d();
	void	toggleUnpegged3d(bool lower);
	void	copy3d(int type);
	void	paste3d(int type);
	void	floodFill3d(int type);
	void	changeThingZ3d(int amount);
	void	deleteThing3d();
	void	changeScale3d(double amount, bool x);
	void	changeHeight3d(int amount);

	// Editor messages
	unsigned	numEditorMessages();
	string		getEditorMessage(int index);
	long		getEditorMessageTime(int index);
	void		addEditorMessage(string message);

	// Undo/Redo
	void	beginUndoRecord(string name, bool mod = true, bool create = true, bool del = true);
	void	beginUndoRecordLocked(string name, bool mod = true, bool create = true, bool del = true);
	void	endUndoRecord(bool success = true);
	void	recordPropertyChangeUndoStep(MapObject* object);
	void	doUndo();
	void	doRedo();

	// Player start swapping
	void	swapPlayerStart3d();
	void	swapPlayerStart2d(fpoint2_t pos);
	void	resetPlayerStart();

	// Misc
	string	getModeString();
	bool	handleKeyBind(string key, fpoint2_t position);
	void	updateDisplay();
	void	updateStatusText();
	void	updateThingLists();
};

#endif//__MAP_EDITOR_H__
