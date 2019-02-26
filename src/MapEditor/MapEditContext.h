#pragma once

#include "Archive/Archive.h"
#include "Edit/Edit2D.h"
#include "Edit/Edit3D.h"
#include "Edit/Input.h"
#include "Edit/LineDraw.h"
#include "Edit/MoveObjects.h"
#include "Edit/ObjectEdit.h"
#include "General/Clipboard.h"
#include "General/SAction.h"
#include "General/UI.h"
#include "General/UndoRedo.h"
#include "ItemSelection.h"
#include "MapEditor.h"
#include "Renderer/Overlays/InfoOverlay3d.h"
#include "Renderer/Overlays/LineInfoOverlay.h"
#include "Renderer/Overlays/MCOverlay.h"
#include "Renderer/Overlays/SectorInfoOverlay.h"
#include "Renderer/Overlays/ThingInfoOverlay.h"
#include "Renderer/Overlays/VertexInfoOverlay.h"
#include "Renderer/Renderer.h"
#include "SLADEMap/SLADEMap.h"

namespace UI
{
enum class MouseCursor;
}

class MapCanvas;

class MapEditContext : public SActionHandler
{
public:
	struct EditorMessage
	{
		std::string message;
		long        act_time;

		EditorMessage(std::string_view message, long act_time) : message{ message }, act_time{ act_time } {}
	};

	MapEditContext();
	~MapEditContext() = default;

	SLADEMap&             map() { return map_; }
	MapEditor::Mode       editMode() const { return edit_mode_; }
	MapEditor::SectorMode sectorEditMode() const { return sector_mode_; }
	double                gridSize() const;
	ItemSelection&        selection() { return selection_; }
	MapEditor::Item       hilightItem() const { return selection_.hilight(); }
	vector<MapSector*>&   taggedSectors() { return tagged_sectors_; }
	vector<MapLine*>&     taggedLines() { return tagged_lines_; }
	vector<MapThing*>&    taggedThings() { return tagged_things_; }
	vector<MapLine*>&     taggingLines() { return tagging_lines_; }
	vector<MapThing*>&    taggingThings() { return tagging_things_; }
	vector<MapThing*>&    pathedThings() { return pathed_things_; }
	bool                  gridSnap() const { return grid_snap_; }
	UndoManager*          undoManager() const { return undo_manager_.get(); }
	Archive::MapDesc&     mapDesc() { return map_desc_; }
	MapCanvas*            canvas() const { return canvas_; }
	MapEditor::Renderer&  renderer() { return renderer_; }
	MapEditor::Input&     input() { return input_; }
	bool                  mouseLocked() const { return mouse_locked_; }

	void setEditMode(MapEditor::Mode mode);
	void setPrevEditMode() { setEditMode(edit_mode_prev_); }
	void setSectorEditMode(MapEditor::SectorMode mode);
	void cycleSectorEditMode();
	void setCanvas(MapCanvas* canvas) { this->canvas_ = canvas; }
	void lockMouse(bool lock);

	// General
	bool update(long frametime);

	// Map loading
	bool openMap(Archive::MapDesc map);
	void clearMap();

	// Selection/hilight
	void showItem(int index);
	void updateTagged();
	void selectionUpdated();

	// Grid
	void   incrementGrid();
	void   decrementGrid();
	double snapToGrid(double position, bool force = true) const;
	Vec2d  relativeSnapToGrid(Vec2d origin, Vec2d mouse_pos) const;

	// Tag edit
	int  beginTagEdit();
	void tagSectorAt(Vec2d pos);
	void endTagEdit(bool accept = true);

	// Editing handlers
	MoveObjects& moveObjects() { return move_objects_; }
	LineDraw&    lineDraw() { return line_draw_; }
	ObjectEdit&  objectEdit() { return object_edit_; }
	Edit3D&      edit3D() { return edit_3d_; }
	Edit2D&      edit2D() { return edit_2d_; }

	// Editor messages
	unsigned           numEditorMessages() const { return editor_messages_.size(); }
	const std::string& editorMessage(int index);
	long               editorMessageTime(int index);
	void               addEditorMessage(std::string_view message);

	// Feature help text
	const vector<std::string>& featureHelpLines() const { return feature_help_lines_; }
	void                       setFeatureHelp(const vector<std::string>& lines);

	// Undo/Redo
	void beginUndoRecord(std::string_view name, bool mod = true, bool create = true, bool del = true);
	void beginUndoRecordLocked(std::string_view name, bool mod = true, bool create = true, bool del = true);
	void endUndoRecord(bool success = true);
	void recordPropertyChangeUndoStep(MapObject* object) const;
	void doUndo();
	void doRedo();
	void resetLastUndoLevel() { last_undo_level_ = ""; }

	// Overlays
	MCOverlay* currentOverlay() const { return overlay_current_.get(); }
	bool       overlayActive() const;
	void       closeCurrentOverlay(bool cancel = false) const;
	void       openSectorTextureOverlay(vector<MapSector*>& sectors);
	void       openQuickTextureOverlay();
	void       openLineTextureOverlay();
	bool       infoOverlayActive() const { return info_showing_; }
	void       updateInfoOverlay();
	void       drawInfoOverlay(const Vec2i& size, float alpha);

	// Player start swapping
	void swapPlayerStart3d();
	void swapPlayerStart2d(Vec2d pos);
	void resetPlayerStart() const;

	// Misc
	std::string modeString(bool plural = true) const;
	bool        handleKeyBind(std::string_view key, Vec2d position);
	void        updateDisplay();
	void        updateStatusText() const;
	void        updateThingLists();
	void        setCursor(UI::MouseCursor cursor) const;
	void        forceRefreshRenderer();


	// SAction handler
	bool handleAction(std::string_view id) override;

private:
	SLADEMap         map_;
	MapCanvas*       canvas_ = nullptr;
	Archive::MapDesc map_desc_;
	long             next_frame_length_ = 0;

	// Undo/Redo stuff
	std::unique_ptr<UndoManager> undo_manager_     = nullptr;
	UndoStep::UPtr               us_create_delete_ = nullptr;

	// Editor state
	MapEditor::Mode       edit_mode_      = MapEditor::Mode::Lines;
	MapEditor::Mode       edit_mode_prev_ = MapEditor::Mode::Lines;
	ItemSelection         selection_      = ItemSelection(this);
	int                   grid_size_      = 9;
	MapEditor::SectorMode sector_mode_    = MapEditor::SectorMode::Both;
	bool                  grid_snap_      = true;
	int                   current_tag_    = 0;
	bool                  mouse_locked_   = false;

	// Undo/Redo
	bool        undo_modified_ = false;
	bool        undo_created_  = false;
	bool        undo_deleted_  = false;
	std::string last_undo_level_;

	// Tagged items
	vector<MapSector*> tagged_sectors_;
	vector<MapLine*>   tagged_lines_;
	vector<MapThing*>  tagged_things_;

	// Tagging items
	vector<MapLine*>  tagging_lines_;
	vector<MapThing*> tagging_things_;

	// Pathed things
	vector<MapThing*> pathed_things_;

	// Editing
	MoveObjects move_objects_{ *this };
	LineDraw    line_draw_{ *this };
	Edit2D      edit_2d_{ *this };
	Edit3D      edit_3d_{ *this };
	ObjectEdit  object_edit_{ *this };

	// Object properties and copy/paste
	std::unique_ptr<MapThing>  copy_thing_      = nullptr;
	std::unique_ptr<MapSector> copy_sector_     = nullptr;
	std::unique_ptr<MapSide>   copy_side_front_ = nullptr;
	std::unique_ptr<MapSide>   copy_side_back_  = nullptr;
	std::unique_ptr<MapLine>   copy_line_       = nullptr;

	// Editor messages
	vector<EditorMessage> editor_messages_;

	// Feature help text
	vector<std::string> feature_help_lines_;

	// Player start swap
	Vec2d player_start_pos_;
	int   player_start_dir_ = 0;

	// Renderer
	MapEditor::Renderer renderer_ = MapEditor::Renderer(*this);

	// Input
	MapEditor::Input input_ = MapEditor::Input(*this);

	// Full-Screen Overlay
	std::unique_ptr<MCOverlay> overlay_current_ = nullptr;

	// Info overlays
	bool              info_showing_ = false;
	VertexInfoOverlay info_vertex_;
	LineInfoOverlay   info_line_;
	SectorInfoOverlay info_sector_;
	ThingInfoOverlay  info_thing_;
	InfoOverlay3D     info_3d_;
};

class MapArchClipboardItem : public ClipboardItem
{
public:
	MapArchClipboardItem() : ClipboardItem(Type::MapArchitecture) {}
	~MapArchClipboardItem() = default;

	void               addLines(const vector<MapLine*>& lines);
	std::string        info() const;
	vector<MapVertex*> pasteToMap(SLADEMap* map, Vec2d position);
	void               putLines(vector<MapLine*>& list);
	Vec2d              midpoint() const { return midpoint_; }

private:
	vector<std::unique_ptr<MapVertex>> vertices_;
	vector<std::unique_ptr<MapSide>>   sides_;
	vector<std::unique_ptr<MapLine>>   lines_;
	vector<std::unique_ptr<MapSector>> sectors_;
	Vec2d                              midpoint_;
};

class MapThingsClipboardItem : public ClipboardItem
{
public:
	MapThingsClipboardItem() : ClipboardItem(Type::MapThings) {}
	~MapThingsClipboardItem() = default;

	void        addThings(vector<MapThing*>& things);
	std::string info() const;
	void        pasteToMap(SLADEMap* map, Vec2d position);
	void        putThings(vector<MapThing*>& list);
	Vec2d       midpoint() const { return midpoint_; }

private:
	vector<std::unique_ptr<MapThing>> things_;
	Vec2d                             midpoint_;
};
