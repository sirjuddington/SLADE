
#ifndef __MAP_EDITOR_H__
#define __MAP_EDITOR_H__

#include "Archive/Archive.h"
#include "Edit/Edit3D.h"
#include "Edit/LineDraw.h"
#include "ItemSelection.h"
#include "MapEditor.h"
#include "ObjectEdit.h"
#include "SLADEMap/SLADEMap.h"

class MapCanvas;
class UndoManager;
class UndoStep;

class MapEditContext
{
private:
	SLADEMap			map;
	MapCanvas*			canvas;
	Archive::mapdesc_t	map_desc;

	// Undo/Redo stuff
	UndoManager*	undo_manager;
	UndoStep*		us_create_delete;

	// Editor state
	MapEditor::Mode			edit_mode;
	ItemSelection			item_selection;
	int						gridsize;
	MapEditor::SectorMode	sector_mode;
	bool					grid_snap;
	int						current_tag;

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

	// Editor messages
	struct editor_msg_t
	{
		string	message;
		long	act_time;
	};
	vector<editor_msg_t>	editor_messages;

	// Player start swap
	fpoint2_t	player_start_pos;
	int			player_start_dir;

	void mergeLines(long, vector<fpoint2_t>&);

public:
	enum
	{
		// Selection
		DESELECT = 0,
		SELECT,
		TOGGLE,
	};

	MapEditContext();
	~MapEditContext();

	SLADEMap&				getMap() { return map; }
	MapEditor::Mode			editMode() const { return edit_mode; }
	MapEditor::SectorMode	sectorEditMode() { return sector_mode; }
	double					gridSize();
	ItemSelection&			selection() { return item_selection; }
	MapEditor::Item			hilightItem() { return item_selection.hilight(); }
	vector<MapSector*>&		taggedSectors() { return tagged_sectors; }
	vector<MapLine*>&		taggedLines() { return tagged_lines; }
	vector<MapThing*>&		taggedThings() { return tagged_things; }
	vector<MapLine*>&		taggingLines() { return tagging_lines; }
	vector<MapThing*>&		taggingThings() { return tagging_things; }
	vector<MapThing*>&		pathedThings() { return pathed_things; }
	bool					gridSnap() { return grid_snap; }
	UndoManager*			undoManager() { return undo_manager; }
	Archive::mapdesc_t&		mapDesc() { return map_desc; }
	MapCanvas*				getCanvas() const { return canvas; }

	void	setEditMode(MapEditor::Mode mode);
	void	setSectorEditMode(MapEditor::SectorMode mode);
	void 	cycleSectorEditMode();
	void	setCanvas(MapCanvas* canvas) { this->canvas = canvas; }

	// Map loading
	bool	openMap(Archive::mapdesc_t map);
	void	clearMap();

	// Selection/hilight
	void	showItem(int index);
	void	updateTagged();
	void	selectionUpdated();

	// Grid
	void		incrementGrid();
	void		decrementGrid();
	double		snapToGrid(double position, bool force = true);
	fpoint2_t	relativeSnapToGrid(fpoint2_t origin, fpoint2_t mouse_pos);

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

	// 3d mode
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
	void	resetLastUndoLevel() { last_undo_level = ""; }

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
