#pragma once

#include "MapEditor.h"

class MapCanvas;
class MapEditContext;
class MapLine;
class MapObject;
class MapSector;
class MapThing;
class MapVertex;
class SLADEMap;

class ItemSelection
{
public:
	typedef std::map<MapEditor::Item, bool>         ChangeSet;
	typedef vector<MapEditor::Item>::const_iterator const_iterator;
	typedef vector<MapEditor::Item>::iterator       iterator;
	typedef vector<MapEditor::Item>::value_type     value_type;

	ItemSelection(MapEditContext* context = nullptr) : context_{ context } {}

	// Accessors
	MapEditor::Item  hilight() const { return hilight_; }
	bool             hilightLocked() const { return hilight_lock_; }
	const ChangeSet& lastChange() const { return last_change_; }

	// Access to selection
	const_iterator         begin() const { return selection_.begin(); }
	const_iterator         end() const { return selection_.end(); }
	iterator               begin() { return selection_.begin(); }
	iterator               end() { return selection_.end(); }
	MapEditor::Item&       operator[](unsigned index) { return selection_[index]; }
	const MapEditor::Item& operator[](unsigned index) const { return selection_[index]; }

	vector<MapEditor::Item> selectionOrHilight();
	MapEditor::Item         firstSelectedOrHilight();

	// Setters
	void lockHilight(bool lock = true) { hilight_lock_ = lock; }
	bool setHilight(const MapEditor::Item& item);
	bool setHilight(int index);

	unsigned size() const { return selection_.size(); }
	bool     empty() const { return selection_.empty(); }
	void     clearHilight()
	{
		if (!hilight_lock_)
			hilight_.index = -1;
	}

	bool hasHilight() const { return hilight_.index >= 0; }
	bool hasHilightOrSelection() const { return !selection_.empty() || hilight_.index >= 0; }
	bool isSelected(const MapEditor::Item& item) const { return VECTOR_EXISTS(selection_, item); }
	bool isHilighted(const MapEditor::Item& item) const { return item == hilight_; }

	bool updateHilight(Vec2f mouse_pos, double dist_scale);
	void clear();

	void select(const MapEditor::Item& item, bool select = true, bool new_change = true);
	void select(const vector<MapEditor::Item>& items, bool select = true, bool new_change = true);
	void deSelect(const MapEditor::Item& item, bool new_change = true) { select(item, false, new_change); }
	void selectAll();
	bool toggleCurrent(bool clear_none = true);
	void selectVerticesWithin(const SLADEMap& map, const Rectf& rect);
	void selectLinesWithin(const SLADEMap& map, const Rectf& rect);
	void selectSectorsWithin(const SLADEMap& map, const Rectf& rect);
	void selectThingsWithin(const SLADEMap& map, const Rectf& rect);
	bool selectWithin(const Rectf& rect, bool add);

	MapVertex* hilightedVertex() const;
	MapLine*   hilightedLine() const;
	MapSector* hilightedSector() const;
	MapThing*  hilightedThing() const;
	MapObject* hilightedObject() const;

	vector<MapVertex*> selectedVertices(bool try_hilight = true) const;
	vector<MapLine*>   selectedLines(bool try_hilight = true) const;
	vector<MapSector*> selectedSectors(bool try_hilight = true) const;
	vector<MapThing*>  selectedThings(bool try_hilight = true) const;
	vector<MapObject*> selectedObjects(bool try_hilight = true) const;

	void migrate(MapEditor::Mode from_edit_mode, MapEditor::Mode to_edit_mode);

	// void	showItem(int index);
	// void	selectItem3d(MapEditor::Item item, int sel);

private:
	MapEditor::Item         hilight_ = { -1, MapEditor::ItemType::Any };
	vector<MapEditor::Item> selection_;
	bool                    hilight_lock_ = false;
	ChangeSet               last_change_;
	MapEditContext*         context_ = nullptr;

	void selectItem(const MapEditor::Item& item, bool select = true);
};
