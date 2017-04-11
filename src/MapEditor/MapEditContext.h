
#ifndef __MAP_EDITOR_H__
#define __MAP_EDITOR_H__

#include "Archive/Archive.h"
#include "SLADEMap/SLADEMap.h"
#include "ObjectEdit.h"
#include "MapEditor.h"
#include "Edit/LineDraw.h"
#include "Edit/Edit3D.h"

class MapCanvas;
class UndoManager;
class MapObjectCreateDeleteUS;
class MapEditContext
{
private:
	SLADEMap			map;
	MapCanvas*			canvas;
	Archive::mapdesc_t	map_desc;

	// Undo/Redo stuff
	UndoManager*				undo_manager;
	MapObjectCreateDeleteUS*	us_create_delete;

	// Editor state
	uint8_t		edit_mode;
	int			hilight_item;
	bool		hilight_locked;
	vector<int>	selection;
	int			gridsize;
	int			sector_mode;
	bool		grid_snap;
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

	// Editing
	LineDraw	line_draw;
	Edit3D		edit_3d;

	// Object edit
	ObjectEditGroup	edit_object_group;

	// Object properties and copy/paste
	MapThing*	copy_thing;
	MapSector*	copy_sector;
	MapLine*	copy_line;
	string		copy_texture;
	double		copy_offsets[2];

	// Editor messages
	struct editor_msg_t
	{
		string	message;
		long	act_time;
	};
	vector<editor_msg_t>	editor_messages;

	void migrateSelection(int old_edit_mode, vector<int>& old_selection, vector<Edit3D::Selection>& old_selection_3d);

	// 3d mode
	Edit3D::Selection			hilight_3d;
	vector<Edit3D::Selection>	selection_3d;

	// Player start swap
	fpoint2_t	player_start_pos;
	int			player_start_dir;

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

		// Copy/paste types
		COPY_TEXTYPE = 0,
		COPY_OFFSETS,
		COPY_SCALE,
	};

	MapEditContext();
	~MapEditContext();

	SLADEMap&				getMap() { return map; }
	uint8_t					editMode() { return edit_mode; }
	int						sectorEditMode() { return sector_mode; }
	double					gridSize();
	unsigned				selectionSize() { return selection.size(); }
	vector<int>&			getSelection() { return selection; }
	int						hilightItem() { return hilight_item; }
	vector<MapSector*>&		taggedSectors() { return tagged_sectors; }
	vector<MapLine*>&		taggedLines() { return tagged_lines; }
	vector<MapThing*>&		taggedThings() { return tagged_things; }
	vector<MapLine*>&		taggingLines() { return tagging_lines; }
	vector<MapThing*>&		taggingThings() { return tagging_things; }
	vector<MapThing*>&		pathedThings() { return pathed_things; }
	bool					hilightLocked() { return hilight_locked; }
	void					lockHilight(bool lock = true) { hilight_locked = lock; }
	bool					gridSnap() { return grid_snap; }
	UndoManager*			undoManager() { return undo_manager; }
	Archive::mapdesc_t&		mapDesc() { return map_desc; }

	vector<Edit3D::Selection>&	get3dSelection() { return selection_3d; }
	bool						set3dHilight(Edit3D::Selection hl);
	Edit3D::Selection			hilightItem3d() { return hilight_3d; }
	void						get3dSelectionOrHilight(vector<Edit3D::Selection>& list);

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
	void		selectItem3d(Edit3D::Selection item, int sel = TOGGLE);

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
	LineDraw&	lineDraw() { return line_draw; }

	// Object edit
	ObjectEditGroup*	getObjectEditGroup() { return &edit_object_group; }
	bool				beginObjectEdit();
	void				endObjectEdit(bool accept);

	// Copy/paste
	void	copy();
	void	paste(fpoint2_t mouse_pos);
	void	copy3d(int type); // TODO: Move to Edit3D
	void	paste3d(int type); // TODO: Move to Edit3D

	// 3d mode
	void	floodFill3d(int type); // TODO: Move to Edit3D
	Edit3D&	edit3d() { return edit_3d; }

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
