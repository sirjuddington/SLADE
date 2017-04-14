
#include "Main.h"
#include "ItemSelection.h"
#include "MapEditContext.h"
#include "GameConfiguration/ThingType.h"
#include "GameConfiguration/GameConfiguration.h"
#include "Utility/MathStuff.h"
#include "UI/MapCanvas.h"

using MapEditor::ItemType;

ItemSelection::ItemSelection(MapEditContext* context) :
	hilight_{ -1, ItemType::Any },
	hilight_lock_{ false },
	context_{ context }
{
}

bool ItemSelection::setHilight(const MapEditor::Item& item)
{
	if (item != hilight_)
	{
		if (context_) context_->resetLastUndoLevel();
		hilight_ = item;
		return true;
	}

	hilight_ = item;
	return false;
}

bool ItemSelection::setHilight(int index)
{
	if (index != hilight_.index)
	{
		if (context_) context_->resetLastUndoLevel();
		hilight_.index = index;
		return true;
	}

	hilight_.index = index;
	return false;
}

/* MapEditContext::updateHilight
 * Hilights the map object closest to [mouse_pos], and updates
 * anything needed if the hilight is changed
 *******************************************************************/
bool ItemSelection::updateHilight(fpoint2_t mouse_pos, double dist_scale)
{
	// Do nothing if hilight is locked or we have no context
	if (hilight_lock_ || !context_)
		return false;

	int current = hilight_.index;

	// Update hilighted object depending on mode
	auto& map = context_->getMap();
	if (context_->editMode() == MapEditContext::MODE_VERTICES)
		hilight_ = { map.nearestVertex(mouse_pos, 32 / dist_scale), ItemType::Vertex };
	else if (context_->editMode() == MapEditContext::MODE_LINES)
		hilight_ = { map.nearestLine(mouse_pos, 32 / dist_scale), ItemType::Line };
	else if (context_->editMode() == MapEditContext::MODE_SECTORS)
		hilight_ = { map.sectorAt(mouse_pos), ItemType::Sector };
	else if (context_->editMode() == MapEditContext::MODE_THINGS)
	{
		hilight_ = { -1, ItemType::Thing };

		// Get (possibly multiple) nearest-thing(s)
		auto nearest = map.nearestThingMulti(mouse_pos);
		if (nearest.size() == 1)
		{
			auto t = map.getThing(nearest[0]);
			auto type = theGameConfiguration->thingType(t->getType());
			if (MathStuff::distance(mouse_pos, t->point()) <= type->getRadius() + (32 / dist_scale))
				hilight_.index = nearest[0];
		}
		else
		{
			for (unsigned a = 0; a < nearest.size(); a++)
			{
				auto t = map.getThing(nearest[a]);
				auto type = theGameConfiguration->thingType(t->getType());
				if (MathStuff::distance(mouse_pos, t->point()) <= type->getRadius() + (32 / dist_scale))
					hilight_.index = nearest[a];
			}
		}
	}

	// Update tagged lists if the hilight changed
	if (current != hilight_.index)
		context_->updateTagged();

	// Update map object properties panel if the hilight changed
	if (current != hilight_.index && selection_.empty())
	{
		switch (context_->editMode())
		{
		case MapEditContext::MODE_VERTICES:
			MapEditor::openObjectProperties(map.getVertex(hilight_.index)); break;
		case MapEditContext::MODE_LINES:
			MapEditor::openObjectProperties(map.getLine(hilight_.index)); break;
		case MapEditContext::MODE_SECTORS:
			MapEditor::openObjectProperties(map.getSector(hilight_.index)); break;
		case MapEditContext::MODE_THINGS:
			MapEditor::openObjectProperties(map.getThing(hilight_.index)); break;
		default: break;
		}

		// TODO: This
		//last_undo_level = "";
	}

	return current != hilight_.index;
}

/* MapEditContext::clearSelection
 * Clears the current selection
 *******************************************************************/
void ItemSelection::clearSelection()
{
	// Update change set
	last_change_.clear();
	for (auto& item : selection_)
		last_change_[item] = false;
	//last_change_.deselected_.assign(selection_.begin(), selection_.end());

	// Clear selection
	selection_.clear();
}

//void ItemSelection::clearSelection(MapEditContext& context, bool animate)
//{
//	//auto canvas = context.getCanvas();
//
//	// TODO: Add some kind of 'selection updated' function to MapEditContext
//	//if (context.editMode() == MapEditContext::MODE_3D)
//	//{
//	//	//if (animate && canvas) canvas->itemsSelected3d(selection_, false);
//	//	selection_.clear();
//	//}
//	//else
//	//{
//	//	//if (animate && canvas) canvas->itemsSelected(selection, false);
//	//	selection_.clear();
//	//	MapEditor::openObjectProperties(nullptr);
//	//	context.updateStatusText();
//	//}
//
//	clearSelection();
//	MapEditor::openObjectProperties(nullptr);
//	context.updateStatusText();
//}

void ItemSelection::select(const MapEditor::Item& item, bool select, bool new_change)
{
	// Start new change set if specified
	if (new_change)
		last_change_.clear();

	selectItem(item, select);
}

/* MapEditContext::selectAll
 * Selects all objects depending on edit mode
 *******************************************************************/
void ItemSelection::selectAll()
{
	// Do nothing with no context
	if (!context_)
		return;

	// Clear selection initially
	//selection_.clear();

	// Start new change set
	last_change_.clear();

	// Select all items depending on mode
	auto& map = context_->getMap();
	if (context_->editMode() == MapEditContext::MODE_VERTICES)
	{
		for (unsigned a = 0; a < map.nVertices(); a++)
			selectItem({ (int)a, ItemType::Vertex });
	}
	else if (context_->editMode() == MapEditContext::MODE_LINES)
	{
		for (unsigned a = 0; a < map.nLines(); a++)
			selectItem({ (int)a, ItemType::Line });
	}
	else if (context_->editMode() == MapEditContext::MODE_SECTORS)
	{
		for (unsigned a = 0; a < map.nSectors(); a++)
			selectItem({ (int)a, ItemType::Sector });
	}
	else if (context_->editMode() == MapEditContext::MODE_THINGS)
	{
		for (unsigned a = 0; a < map.nThings(); a++)
			selectItem({ (int)a, ItemType::Thing });
	}

	context_->addEditorMessage(S_FMT("Selected all %lu %s", selection_.size(), context_->getModeString()));

	// TODO: This
	//if (canvas)
	//	canvas->itemsSelected(selection);

	context_->selectionUpdated();
}

/* MapEditContext::selectCurrent
 * Toggles selection on the currently hilighted object
 *******************************************************************/
bool ItemSelection::toggleCurrent(bool clear_none)
{
	// If nothing is hilighted
	if (hilight_.index == -1)
	{
		// Clear selection if specified
		if (clear_none)
		{
			//if (canvas) canvas->itemsSelected3d(selection_3d, false); TODO: This
			clearSelection();
			if (context_)
			{
				context_->selectionUpdated();
				context_->addEditorMessage("Selection cleared");
			}
		}

		return false;
	}

	// Toggle selection on item
	selectItem(hilight_, !isSelected(hilight_));

	if (context_)
		context_->selectionUpdated();

	//// Otherwise, check if item is in selection
	//for (auto& item : selection_)
	//{
	//	if (item == hilight_)
	//	{
	//		selection_.erase(item);
	//		//if (canvas) canvas->itemSelected3d(hilight_3d, false); TODO: This
	//		if (context_)
	//			context_->selectionUpdated();
	//		return true;
	//	}
	//}

	//// Not already selected, add to selection
	//selection_.insert(hilight_);
	////if (canvas) canvas->itemSelected3d(hilight_3d); TODO: This
	//if (context_)
	//	context_->selectionUpdated();

	return true;
}

void ItemSelection::selectVerticesWithin(const SLADEMap& map, const frect_t& rect)
{
	// Start new change set
	last_change_.clear();

	// Select vertices within bounds
	for (unsigned a = 0; a < map.nVertices(); a++)
		if (rect.contains(map.getVertex(a)->point()))
			selectItem({ (int)a, ItemType::Vertex });
}

void ItemSelection::selectLinesWithin(const SLADEMap& map, const frect_t& rect)
{
	// Start new change set
	last_change_.clear();

	// Select lines within bounds
	for (unsigned a = 0; a < map.nLines(); a++)
		if (rect.contains(map.getLine(a)->v1()->point()) &&
			rect.contains(map.getLine(a)->v2()->point()))
			selectItem({ (int)a, ItemType::Line });
}

void ItemSelection::selectSectorsWithin(const SLADEMap& map, const frect_t& rect)
{
	// Start new change set
	last_change_.clear();

	// Select sectors within bounds
	for (unsigned a = 0; a < map.nSectors(); a++)
		if (map.getSector(a)->boundingBox().is_within(rect.tl, rect.br))
			selectItem({ (int)a, ItemType::Sector });
}

void ItemSelection::selectThingsWithin(const SLADEMap& map, const frect_t& rect)
{
	// Start new change set
	last_change_.clear();

	// Select vertices within bounds
	for (unsigned a = 0; a < map.nThings(); a++)
		if (rect.contains(map.getThing(a)->point()))
			selectItem({ (int)a, ItemType::Thing });
}

/* MapEditContext::selectWithin
 * Selects all objects within the box from [xmin,ymin] to [xmax,ymax].
 * If [add] is false, the selection will be cleared first
 *******************************************************************/
bool ItemSelection::selectWithin(const frect_t& rect, bool add)
{
	// Do nothing if no context
	if (!context_)
		return false;

	// Clear current selection if not adding
	if (!add)
		clearSelection();

	// Select depending on edit mode
	switch (context_->editMode())
	{
	case MapEditContext::MODE_VERTICES:	selectVerticesWithin(context_->getMap(), rect); break;
	case MapEditContext::MODE_LINES:	selectLinesWithin(context_->getMap(), rect); break;
	case MapEditContext::MODE_SECTORS:	selectSectorsWithin(context_->getMap(), rect); break;
	case MapEditContext::MODE_THINGS:	selectThingsWithin(context_->getMap(), rect); break;
	default: break;
	}

	// Return true if any new items were selected
	for (auto& change : last_change_)
		if (change.second)
			return true;

	return false;
}

/* MapEditContext::getHilightedVertex
 * Returns the currently hilighted MapVertex
 *******************************************************************/
MapVertex* ItemSelection::hilightedVertex() const
{
	if (context_ && hilight_.index >= 0 && hilight_.type == ItemType::Vertex)
		return context_->getMap().getVertex(hilight_.index);

	return nullptr;
}

/* MapEditContext::getHilightedLine
 * Returns the currently hilighted MapLine
 *******************************************************************/
MapLine* ItemSelection::hilightedLine() const
{
	if (context_ && hilight_.index >= 0 && hilight_.type == ItemType::Line)
		return context_->getMap().getLine(hilight_.index);

	return nullptr;
}

/* MapEditContext::getHilightedSector
 * Returns the currently hilighted MapSector
 *******************************************************************/
MapSector* ItemSelection::hilightedSector() const
{
	if (context_ && hilight_.index >= 0 && hilight_.type == ItemType::Sector)
		return context_->getMap().getSector(hilight_.index);

	return nullptr;
}

/* MapEditContext::getHilightedThing
 * Returns the currently hilighted MapThing
 *******************************************************************/
MapThing* ItemSelection::hilightedThing() const
{
	if (context_ && hilight_.index >= 0 && hilight_.type == ItemType::Thing)
		return context_->getMap().getThing(hilight_.index);

	return nullptr;
}

/* MapEditContext::getHilightedObject
 * Returns the currently hilighted MapObject
 *******************************************************************/
MapObject* ItemSelection::hilightedObject() const
{
	if (!context_)
		return nullptr;

	switch (context_->editMode())
	{
	case MapEditContext::MODE_VERTICES: return hilightedVertex();
	case MapEditContext::MODE_LINES:	return hilightedLine();
	case MapEditContext::MODE_SECTORS:	return hilightedSector();
	case MapEditContext::MODE_THINGS:	return hilightedThing();
	default:							return nullptr;
	}
}

vector<MapVertex*> ItemSelection::selectedVertices(bool try_hilight) const
{
	vector<MapVertex*> list;

	if (!context_)
		return list;

	// Get selected vertices
	for (auto& item : selection_)
		if (item.type == ItemType::Vertex)
			list.push_back(context_->getMap().getVertex(item.index));

	// If no vertices were selected, try the hilight
	if (try_hilight && list.empty() && hilight_.index >= 0 && hilight_.type == ItemType::Vertex)
		list.push_back(context_->getMap().getVertex(hilight_.index));

	return list;
}

vector<MapLine*> ItemSelection::selectedLines(bool try_hilight) const
{
	vector<MapLine*> list;

	if (!context_)
		return list;

	// Get selected lines
	for (auto& item : selection_)
		if (item.type == ItemType::Line)
			list.push_back(context_->getMap().getLine(item.index));

	// If no lines were selected, try the hilight
	if (try_hilight && list.empty() && hilight_.index >= 0 && hilight_.type == ItemType::Line)
		list.push_back(context_->getMap().getLine(hilight_.index));

	return list;
}

vector<MapSector*> ItemSelection::selectedSectors(bool try_hilight) const
{
	vector<MapSector*> list;

	if (!context_)
		return list;

	// Get selected sectors
	for (auto& item : selection_)
		if (item.type == ItemType::Sector)
			list.push_back(context_->getMap().getSector(item.index));

	// If no sectors were selected, try the hilight
	if (try_hilight && list.empty() && hilight_.index >= 0 && hilight_.type == ItemType::Sector)
		list.push_back(context_->getMap().getSector(hilight_.index));

	return list;
}

vector<MapThing*> ItemSelection::selectedThings(bool try_hilight) const
{
	vector<MapThing*> list;

	if (!context_)
		return list;

	// Get selected things
	for (auto& item : selection_)
		if (item.type == ItemType::Thing)
			list.push_back(context_->getMap().getThing(item.index));

	// If no things were selected, try the hilight
	if (try_hilight && list.empty() && hilight_.index >= 0 && hilight_.type == ItemType::Thing)
		list.push_back(context_->getMap().getThing(hilight_.index));

	return list;
}

vector<MapObject*> ItemSelection::selectedObjects(bool try_hilight) const
{
	vector<MapObject*> list;

	if (!context_)
		return list;

	// Get object type depending on edit mode
	uint8_t type;
	switch (context_->editMode())
	{
	case MapEditContext::MODE_VERTICES: type = MOBJ_VERTEX; break;
	case MapEditContext::MODE_LINES: type = MOBJ_LINE; break;
	case MapEditContext::MODE_SECTORS: type = MOBJ_SECTOR; break;
	case MapEditContext::MODE_THINGS: type = MOBJ_THING; break;
	default: return list;
	}

	// Get selected objects
	for (auto& item : selection_)
		list.push_back(context_->getMap().getObject(type, item.index));

	// If no objects were selected, try the hilight
	if (try_hilight && list.empty() && hilight_.index >= 0)
		list.push_back(context_->getMap().getObject(type, hilight_.index));

	return list;
}

/* ItemSelection::migrate
 * Converts the selection from [from_edit_mode] to one appropriate
 * for [to_edit_mode] (and [to_sector_mode] if applicable)
 * For example, selecting a sector and then switching to lines mode
 * will select all its lines
 *******************************************************************/
void ItemSelection::migrate(int from_edit_mode, int to_edit_mode, int to_sector_mode)
{
	std::set<MapEditor::Item> new_selection;

	// 3D to 2D: select anything of the right type
	if (from_edit_mode == MapEditContext::MODE_3D)
	{
		for (auto& item : selection_)
		{
			// To things mode
			if (to_edit_mode == MapEditContext::MODE_THINGS && baseItemType(item.type) == ItemType::Thing)
				new_selection.insert({ item.index, ItemType::Thing });

			// To sectors mode
			else if (to_edit_mode == MapEditContext::MODE_SECTORS && baseItemType(item.type) == ItemType::Sector)
				new_selection.insert({ item.index, ItemType::Sector });

			// To lines mode
			else if (to_edit_mode == MapEditContext::MODE_LINES && baseItemType(item.type) == ItemType::Side)
			{
				auto side = context_->getMap().getSide(item.index);
				if (!side) continue;
				new_selection.insert({ (int)side->getParentLine()->getIndex(), ItemType::Line });
			}
		}
	}

	// 2D to 3D: can be done perfectly
	else if (to_edit_mode == MapEditContext::MODE_3D)
	{
		for (auto& item : selection_)
		{
			// Sector
			if (baseItemType(item.type) == ItemType::Sector)
			{
				// Select floor if in applicable sector mode
				if (to_sector_mode == MapEditContext::SECTOR_BOTH || to_sector_mode == MapEditContext::SECTOR_FLOOR)
					new_selection.insert({ item.index, ItemType::Floor });

				// Select ceiling if in applicable sector mode
				if (to_sector_mode == MapEditContext::SECTOR_BOTH || to_sector_mode == MapEditContext::SECTOR_CEILING)
					new_selection.insert({ item.index, ItemType::Ceiling });
			}

			// Line
			else if (baseItemType(item.type) == ItemType::Line)
			{
				auto line = context_->getMap().getLine(item.index);
				auto front = line->s1();
				auto back = line->s2();

				// Only select the visible areas -- i.e., the ones that need texturing
				int textures = line->needsTexture();
				if (front && textures & TEX_FRONT_UPPER)
					new_selection.insert({ (int)front->getIndex(), ItemType::WallTop });
				if (front && textures & TEX_FRONT_LOWER)
					new_selection.insert({ (int)front->getIndex(), ItemType::WallBottom });
				if (back && textures & TEX_BACK_UPPER)
					new_selection.insert({ (int)back->getIndex(), ItemType::WallTop });
				if (back && textures & TEX_BACK_LOWER)
					new_selection.insert({ (int)back->getIndex(), ItemType::WallBottom });

				// Also include any two-sided middle textures
				if (front && (textures & TEX_FRONT_MIDDLE || !front->getTexMiddle().empty()))
					new_selection.insert({ (int)front->getIndex(), ItemType::WallMiddle });
				if (back && (textures & TEX_BACK_MIDDLE || !back->getTexMiddle().empty()))
					new_selection.insert({ (int)back->getIndex(), ItemType::WallMiddle });
			}

			// Thing
			else if (baseItemType(item.type) == ItemType::Thing)
				new_selection.insert(item);
		}
	}

	// Otherwise, 2D to 2D

	// Sectors can be migrated to anything
	else if (from_edit_mode == MapEditContext::MODE_SECTORS)
	{
		for (auto& item : selection_)
		{
			auto sector = context_->getMap().getSector(item.index);
			if (!sector) continue;

			// To lines mode
			if (to_edit_mode == MapEditContext::MODE_LINES)
			{
				vector<MapLine*> lines;
				sector->getLines(lines);
				for (auto line : lines)
					new_selection.insert({ (int)line->getIndex(), ItemType::Line });
			}

			// To vertices mode
			else if (to_edit_mode == MapEditContext::MODE_VERTICES)
			{
				vector<MapVertex*> vertices;
				sector->getVertices(vertices);
				for (auto vertex : vertices)
					new_selection.insert({ (int)vertex->getIndex(), ItemType::Vertex });
			}

			// To things mode
			else if (to_edit_mode == MapEditContext::MODE_THINGS)
			{
				// TODO this is much harder
			}
		}
	}

	// Lines can only reliably be migrated to vertices
	else if (from_edit_mode == MapEditContext::MODE_LINES && to_edit_mode == MapEditContext::MODE_VERTICES)
	{
		for (auto& item : selection_)
		{
			auto line = context_->getMap().getLine(item.index);
			if (!line) continue;
			new_selection.insert({ (int)line->v1()->getIndex(), ItemType::Vertex });
			new_selection.insert({ (int)line->v2()->getIndex(), ItemType::Vertex });
		}
	}

	// Apply new selection
	selection_.assign(new_selection.begin(), new_selection.end());
}

void ItemSelection::selectItem(const MapEditor::Item& item, bool select)
{
	// Check if already selected
	bool selected = VECTOR_EXISTS(selection_, item);

	// (De)Select item
	if (select)	selection_.push_back(item);
	else		VECTOR_REMOVE(selection_, item);

	// Update change set
	if (select && !selected)
		last_change_[item] = true;
	if (!select && selected)
		last_change_[item] = false;
}
