#pragma once

#include "Geometry/RectFwd.h"
#include "Item.h"

// Forward declarations
namespace slade
{
class MapCanvas;
}
namespace slade::mapeditor
{
enum class Mode;
class MapEditContext;
} // namespace slade::mapeditor

namespace slade::mapeditor
{
class ItemSelection
{
public:
	typedef std::map<Item, bool>         ChangeSet;
	typedef vector<Item>::const_iterator const_iterator;
	typedef vector<Item>::iterator       iterator;
	typedef vector<Item>::value_type     value_type;

	ItemSelection(MapEditContext* context = nullptr) : context_{ context } {}

	// Accessors
	Item                hilight() const { return hilight_; }
	bool                hilightLocked() const { return hilight_lock_; }
	const ChangeSet&    lastChange() const { return last_change_; }
	const vector<Item>& selectedItems() const { return selection_; }
	vector<Item>&       selectedItems() { return selection_; }

	// Access to selection
	const_iterator begin() const { return selection_.begin(); }
	const_iterator end() const { return selection_.end(); }
	iterator       begin() { return selection_.begin(); }
	iterator       end() { return selection_.end(); }
	Item&          operator[](unsigned index) { return selection_[index]; }
	const Item&    operator[](unsigned index) const { return selection_[index]; }

	vector<Item> selectionOrHilight();
	Item         firstSelectedOrHilight() const;

	// Setters
	void lockHilight(bool lock = true) { hilight_lock_ = lock; }
	bool setHilight(const Item& item);
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
	bool isSelected(const Item& item) const { return VECTOR_EXISTS(selection_, item); }
	bool isHilighted(const Item& item) const { return item == hilight_; }

	bool updateHilight(const Vec2d& mouse_pos, double dist_scale);
	void clear();

	void select(const Item& item, bool select = true, bool new_change = true);
	void select(const vector<Item>& items, bool select = true, bool new_change = true);
	void deSelect(const Item& item, bool new_change = true) { select(item, false, new_change); }
	void selectAll();
	bool toggleCurrent(bool clear_none = true);
	void selectVerticesWithin(const SLADEMap& map, const Rectd& rect);
	void selectLinesWithin(const SLADEMap& map, const Rectd& rect);
	void selectSectorsWithin(const SLADEMap& map, const Rectd& rect);
	void selectThingsWithin(const SLADEMap& map, const Rectd& rect);
	bool selectWithin(const Rectd& rect, bool add);

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

	void migrate(Mode from_edit_mode, Mode to_edit_mode);

private:
	Item            hilight_ = { -1, ItemType::Any };
	vector<Item>    selection_;
	bool            hilight_lock_ = false;
	ChangeSet       last_change_;
	MapEditContext* context_ = nullptr;

	void selectItem(const Item& item, bool select = true);
};
} // namespace slade::mapeditor
