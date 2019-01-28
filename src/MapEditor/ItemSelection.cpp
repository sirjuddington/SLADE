
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ItemSelection.cpp
// Description: ItemSelection class - A container for a map editor's current
//              selection and hilight, along with various utility functions for
//              handling selection and hilight for a MapEditContext
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ItemSelection.h"
#include "Game/Configuration.h"
#include "MapEditContext.h"
#include "UI/MapCanvas.h"
#include "Utility/MathStuff.h"


// -----------------------------------------------------------------------------
//
// ItemSelection Class Functions
//
// -----------------------------------------------------------------------------
using MapEditor::ItemType;
using MapEditor::Mode;
using MapEditor::SectorMode;

// -----------------------------------------------------------------------------
// Returns either the highlighted item if anything is highlighted, or the
// currently selected items if not
// -----------------------------------------------------------------------------
vector<MapEditor::Item> ItemSelection::selectionOrHilight()
{
	vector<MapEditor::Item> list;

	if (!selection_.empty())
		list.assign(selection_.begin(), selection_.end());
	else if (hilight_.index >= 0)
		list.push_back(hilight_);

	return list;
}

// -----------------------------------------------------------------------------
// Sets the current hilight to [item]. Returns true if the hilight was changed
// -----------------------------------------------------------------------------
bool ItemSelection::setHilight(const MapEditor::Item& item)
{
	if (item != hilight_)
	{
		if (context_)
			context_->resetLastUndoLevel();
		hilight_ = item;
		return true;
	}

	hilight_ = item;
	return false;
}

// -----------------------------------------------------------------------------
// Sets the current hilight item index to [index].
// Returns true if the hilight was changed
// -----------------------------------------------------------------------------
bool ItemSelection::setHilight(int index)
{
	if (index != hilight_.index)
	{
		if (context_)
			context_->resetLastUndoLevel();
		hilight_.index = index;
		return true;
	}

	hilight_.index = index;
	return false;
}

// -----------------------------------------------------------------------------
// Hilights the map object closest to [mouse_pos], and updates anything needed
// if the hilight is changed
// -----------------------------------------------------------------------------
bool ItemSelection::updateHilight(Vec2f mouse_pos, double dist_scale)
{
	// Do nothing if hilight is locked or we have no context
	if (hilight_lock_ || !context_)
		return false;

	int current = hilight_.index;

	// Update hilighted object depending on mode
	auto& map = context_->map();
	if (context_->editMode() == Mode::Vertices)
	{
		auto vertex = map.vertices().nearest(mouse_pos, 32 / dist_scale);
		hilight_    = { vertex ? (int)vertex->index() : -1, ItemType::Vertex };
	}
	else if (context_->editMode() == Mode::Lines)
	{
		auto line = map.lines().nearest(mouse_pos, 32 / dist_scale);
		hilight_  = { line ? (int)line->index() : -1, ItemType::Line };
	}
	else if (context_->editMode() == Mode::Sectors)
	{
		auto sector = map.sectors().atPos(mouse_pos);
		hilight_    = { sector ? (int)sector->index() : -1, ItemType::Sector };
	}
	else if (context_->editMode() == Mode::Things)
	{
		hilight_ = { -1, ItemType::Thing };

		// Get (possibly multiple) nearest-thing(s)
		auto nearest = map.things().multiNearest(mouse_pos);
		if (nearest.size() == 1)
		{
			auto& type = Game::configuration().thingType(nearest[0]->type());
			if (MathStuff::distance(mouse_pos, nearest[0]->position()) <= type.radius() + (32 / dist_scale))
				hilight_.index = nearest[0]->index();
		}
		else
		{
			for (auto& t : nearest)
			{
				auto& type = Game::configuration().thingType(t->type());
				if (MathStuff::distance(mouse_pos, t->position()) <= type.radius() + (32 / dist_scale))
					hilight_.index = t->index();
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
		case Mode::Vertices: MapEditor::openObjectProperties(map.vertex(hilight_.index)); break;
		case Mode::Lines: MapEditor::openObjectProperties(map.line(hilight_.index)); break;
		case Mode::Sectors: MapEditor::openObjectProperties(map.sector(hilight_.index)); break;
		case Mode::Things: MapEditor::openObjectProperties(map.thing(hilight_.index)); break;
		default: break;
		}

		context_->resetLastUndoLevel();
	}

	return current != hilight_.index;
}

// -----------------------------------------------------------------------------
// Clears the current selection
// -----------------------------------------------------------------------------
void ItemSelection::clear()
{
	// Update change set
	last_change_.clear();
	for (auto& item : selection_)
		last_change_[item] = false;

	// Clear selection
	selection_.clear();

	if (context_)
		context_->selectionUpdated();
}

// -----------------------------------------------------------------------------
// Changes the selection status of [item] to [select].
// If [new_change] is true, a new change set is started
// -----------------------------------------------------------------------------
void ItemSelection::select(const MapEditor::Item& item, bool select, bool new_change)
{
	// Start new change set if specified
	if (new_change)
		last_change_.clear();

	selectItem(item, select);
}

// -----------------------------------------------------------------------------
// Changes the selection status of all items in [items] to [select].
// If [new_change] is true, a new change set is started
// -----------------------------------------------------------------------------
void ItemSelection::select(const vector<MapEditor::Item>& items, bool select, bool new_change)
{
	// Start new change set if specified
	if (new_change)
		last_change_.clear();

	for (auto& item : items)
		selectItem(item, select);
}

// -----------------------------------------------------------------------------
// Selects all objects depending on the context's edit mode
// -----------------------------------------------------------------------------
void ItemSelection::selectAll()
{
	// Do nothing with no context
	if (!context_)
		return;

	// Start new change set
	last_change_.clear();

	// Select all items depending on mode
	auto& map = context_->map();
	if (context_->editMode() == Mode::Vertices)
	{
		for (unsigned a = 0; a < map.nVertices(); a++)
			selectItem({ (int)a, ItemType::Vertex });
	}
	else if (context_->editMode() == Mode::Lines)
	{
		for (unsigned a = 0; a < map.nLines(); a++)
			selectItem({ (int)a, ItemType::Line });
	}
	else if (context_->editMode() == Mode::Sectors)
	{
		for (unsigned a = 0; a < map.nSectors(); a++)
			selectItem({ (int)a, ItemType::Sector });
	}
	else if (context_->editMode() == Mode::Things)
	{
		for (unsigned a = 0; a < map.nThings(); a++)
			selectItem({ (int)a, ItemType::Thing });
	}

	context_->addEditorMessage(S_FMT("Selected all %lu %s", selection_.size(), context_->modeString()));
	context_->selectionUpdated();
}

// -----------------------------------------------------------------------------
// Toggles selection on the currently hilighted object
// -----------------------------------------------------------------------------
bool ItemSelection::toggleCurrent(bool clear_none)
{
	// If nothing is hilighted
	if (hilight_.index == -1)
	{
		// Clear selection if specified
		if (clear_none)
		{
			clear();
			if (context_)
			{
				context_->selectionUpdated();
				context_->addEditorMessage("Selection cleared");
			}
		}

		return false;
	}

	// Toggle selection on item
	select(hilight_, !isSelected(hilight_), true);

	if (context_)
		context_->selectionUpdated();

	return true;
}

// -----------------------------------------------------------------------------
// Selects all vertices in [map] that are within [rect]
// -----------------------------------------------------------------------------
void ItemSelection::selectVerticesWithin(const SLADEMap& map, const Rectf& rect)
{
	// Start new change set
	last_change_.clear();

	// Select vertices within bounds
	for (unsigned a = 0; a < map.nVertices(); a++)
		if (rect.contains(map.vertex(a)->position()))
			selectItem({ (int)a, ItemType::Vertex });
}

// -----------------------------------------------------------------------------
// Selects all lines in [map] that are within [rect]
// -----------------------------------------------------------------------------
void ItemSelection::selectLinesWithin(const SLADEMap& map, const Rectf& rect)
{
	// Start new change set
	last_change_.clear();

	// Select lines within bounds
	for (unsigned a = 0; a < map.nLines(); a++)
		if (rect.contains(map.line(a)->v1()->position()) && rect.contains(map.line(a)->v2()->position()))
			selectItem({ (int)a, ItemType::Line });
}

// -----------------------------------------------------------------------------
// Selects all sectors in [map] that are within [rect]
// -----------------------------------------------------------------------------
void ItemSelection::selectSectorsWithin(const SLADEMap& map, const Rectf& rect)
{
	// Start new change set
	last_change_.clear();

	// Select sectors within bounds
	for (unsigned a = 0; a < map.nSectors(); a++)
		if (map.sector(a)->boundingBox().isWithin(rect.tl, rect.br))
			selectItem({ (int)a, ItemType::Sector });
}

// -----------------------------------------------------------------------------
// Selects all things in [map] that are within [rect]
// -----------------------------------------------------------------------------
void ItemSelection::selectThingsWithin(const SLADEMap& map, const Rectf& rect)
{
	// Start new change set
	last_change_.clear();

	// Select vertices within bounds
	for (unsigned a = 0; a < map.nThings(); a++)
		if (rect.contains(map.thing(a)->position()))
			selectItem({ (int)a, ItemType::Thing });
}

// -----------------------------------------------------------------------------
// Selects all objects within [rect].
// If [add] is false, the selection will be cleared first
// -----------------------------------------------------------------------------
bool ItemSelection::selectWithin(const Rectf& rect, bool add)
{
	// Do nothing if no context
	if (!context_)
		return false;

	// Clear current selection if not adding
	if (!add)
		clear();

	// Select depending on edit mode
	switch (context_->editMode())
	{
	case Mode::Vertices: selectVerticesWithin(context_->map(), rect); break;
	case Mode::Lines: selectLinesWithin(context_->map(), rect); break;
	case Mode::Sectors: selectSectorsWithin(context_->map(), rect); break;
	case Mode::Things: selectThingsWithin(context_->map(), rect); break;
	default: break;
	}

	context_->selectionUpdated();

	// Return true if any new items were selected
	for (auto& change : last_change_)
		if (change.second)
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Returns the currently hilighted MapVertex
// -----------------------------------------------------------------------------
MapVertex* ItemSelection::hilightedVertex() const
{
	if (context_ && hilight_.index >= 0 && hilight_.type == ItemType::Vertex)
		return context_->map().vertex(hilight_.index);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the currently hilighted MapLine
// -----------------------------------------------------------------------------
MapLine* ItemSelection::hilightedLine() const
{
	if (context_ && hilight_.index >= 0 && hilight_.type == ItemType::Line)
		return context_->map().line(hilight_.index);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the currently hilighted MapSector
// -----------------------------------------------------------------------------
MapSector* ItemSelection::hilightedSector() const
{
	if (context_ && hilight_.index >= 0 && hilight_.type == ItemType::Sector)
		return context_->map().sector(hilight_.index);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the currently hilighted MapThing
// -----------------------------------------------------------------------------
MapThing* ItemSelection::hilightedThing() const
{
	if (context_ && hilight_.index >= 0 && hilight_.type == ItemType::Thing)
		return context_->map().thing(hilight_.index);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the currently hilighted MapObject
// -----------------------------------------------------------------------------
MapObject* ItemSelection::hilightedObject() const
{
	if (!context_)
		return nullptr;

	switch (context_->editMode())
	{
	case Mode::Vertices: return hilightedVertex();
	case Mode::Lines: return hilightedLine();
	case Mode::Sectors: return hilightedSector();
	case Mode::Things: return hilightedThing();
	default: return nullptr;
	}
}

// -----------------------------------------------------------------------------
// Returns as list of the currently selected vertices.
// If [try_hilight] is true, the hilighted vertex will be added to the list if
// nothing is selected
// -----------------------------------------------------------------------------
vector<MapVertex*> ItemSelection::selectedVertices(bool try_hilight) const
{
	vector<MapVertex*> list;

	if (!context_)
		return list;

	// Get selected vertices
	for (auto& item : selection_)
		if (item.type == ItemType::Vertex)
			list.push_back(context_->map().vertex(item.index));

	// If no vertices were selected, try the hilight
	if (try_hilight && list.empty() && hilight_.index >= 0 && hilight_.type == ItemType::Vertex)
		list.push_back(context_->map().vertex(hilight_.index));

	return list;
}

// -----------------------------------------------------------------------------
// Returns as list of the currently selected lines.
// If [try_hilight] is true, the hilighted line will be added to the list if
// nothing is selected
// -----------------------------------------------------------------------------
vector<MapLine*> ItemSelection::selectedLines(bool try_hilight) const
{
	vector<MapLine*> list;

	if (!context_)
		return list;

	// Get selected lines
	for (auto& item : selection_)
		if (item.type == ItemType::Line)
			list.push_back(context_->map().line(item.index));

	// If no lines were selected, try the hilight
	if (try_hilight && list.empty() && hilight_.index >= 0 && hilight_.type == ItemType::Line)
		list.push_back(context_->map().line(hilight_.index));

	return list;
}

// -----------------------------------------------------------------------------
// Returns as list of the currently selected sectors.
// If [try_hilight] is true, the hilighted sector will be added to the list if
// nothing is selected
// -----------------------------------------------------------------------------
vector<MapSector*> ItemSelection::selectedSectors(bool try_hilight) const
{
	vector<MapSector*> list;

	if (!context_)
		return list;

	// Get selected sectors
	for (auto& item : selection_)
		if (item.type == ItemType::Sector)
			list.push_back(context_->map().sector(item.index));

	// If no sectors were selected, try the hilight
	if (try_hilight && list.empty() && hilight_.index >= 0 && hilight_.type == ItemType::Sector)
		list.push_back(context_->map().sector(hilight_.index));

	return list;
}

// -----------------------------------------------------------------------------
// Returns as list of the currently selected things.
// If [try_hilight] is true, the hilighted thing will be added to the list if
// nothing is selected
// -----------------------------------------------------------------------------
vector<MapThing*> ItemSelection::selectedThings(bool try_hilight) const
{
	vector<MapThing*> list;

	if (!context_)
		return list;

	// Get selected things
	for (auto& item : selection_)
		if (item.type == ItemType::Thing)
			list.push_back(context_->map().thing(item.index));

	// If no things were selected, try the hilight
	if (try_hilight && list.empty() && hilight_.index >= 0 && hilight_.type == ItemType::Thing)
		list.push_back(context_->map().thing(hilight_.index));

	return list;
}

// -----------------------------------------------------------------------------
// Returns as list of the currently selected objects depending on the context's
// edit mode.
// If [try_hilight] is true, the hilighted object will be added to the list if
// nothing is selected
// -----------------------------------------------------------------------------
vector<MapObject*> ItemSelection::selectedObjects(bool try_hilight) const
{
	if (!context_)
		return {};

	// Get object type depending on edit mode
	MapObject::Type type;
	switch (context_->editMode())
	{
	case Mode::Vertices: type = MapObject::Type::Vertex; break;
	case Mode::Lines: type = MapObject::Type::Line; break;
	case Mode::Sectors: type = MapObject::Type::Sector; break;
	case Mode::Things: type = MapObject::Type::Thing; break;
	default: return {};
	}

	// Get selected objects
	vector<MapObject*> list;
	for (auto& item : selection_)
		list.push_back(context_->map().object(type, item.index));

	// If no objects were selected, try the hilight
	if (try_hilight && list.empty() && hilight_.index >= 0)
		list.push_back(context_->map().object(type, hilight_.index));

	return list;
}

// -----------------------------------------------------------------------------
// Converts the selection from [from_edit_mode] to one appropriate for
// [to_edit_mode].
// For example, selecting a sector and then switching to lines mode will select
// all its lines
// -----------------------------------------------------------------------------
void ItemSelection::migrate(Mode from_edit_mode, Mode to_edit_mode)
{
	std::set<MapEditor::Item> new_selection;

	// 3D to 2D: select anything of the right type
	if (from_edit_mode == Mode::Visual)
	{
		for (auto& item : selection_)
		{
			// To things mode
			if (to_edit_mode == Mode::Things && baseItemType(item.type) == ItemType::Thing)
				new_selection.insert({ item.index, ItemType::Thing });

			// To sectors mode
			else if (to_edit_mode == Mode::Sectors && baseItemType(item.type) == ItemType::Sector)
				new_selection.insert({ item.index, ItemType::Sector });

			// To lines mode
			else if (to_edit_mode == Mode::Lines && baseItemType(item.type) == ItemType::Side)
			{
				auto side = context_->map().side(item.index);
				if (!side)
					continue;
				new_selection.insert({ (int)side->parentLine()->index(), ItemType::Line });
			}
		}
	}

	// 2D to 3D: can be done perfectly
	else if (to_edit_mode == Mode::Visual)
	{
		for (auto& item : selection_)
		{
			// Sector
			if (baseItemType(item.type) == ItemType::Sector)
			{
				// Select floor+ceiling
				new_selection.insert({ item.index, ItemType::Floor });
				new_selection.insert({ item.index, ItemType::Ceiling });
			}

			// Line
			else if (baseItemType(item.type) == ItemType::Line)
			{
				auto line  = context_->map().line(item.index);
				auto front = line->s1();
				auto back  = line->s2();

				// Only select the visible areas -- i.e., the ones that need texturing
				int textures = line->needsTexture();
				if (front && textures & MapLine::Part::FrontUpper)
					new_selection.insert({ (int)front->index(), ItemType::WallTop });
				if (front && textures & MapLine::Part::FrontLower)
					new_selection.insert({ (int)front->index(), ItemType::WallBottom });
				if (back && textures & MapLine::Part::BackUpper)
					new_selection.insert({ (int)back->index(), ItemType::WallTop });
				if (back && textures & MapLine::Part::BackLower)
					new_selection.insert({ (int)back->index(), ItemType::WallBottom });

				// Also include any two-sided middle textures
				if (front && (textures & MapLine::Part::FrontMiddle || !front->texMiddle().empty()))
					new_selection.insert({ (int)front->index(), ItemType::WallMiddle });
				if (back && (textures & MapLine::Part::BackMiddle || !back->texMiddle().empty()))
					new_selection.insert({ (int)back->index(), ItemType::WallMiddle });
			}

			// Thing
			else if (baseItemType(item.type) == ItemType::Thing)
				new_selection.insert(item);
		}
	}

	// Otherwise, 2D to 2D

	// Sectors can be migrated to anything
	else if (from_edit_mode == Mode::Sectors)
	{
		for (auto& item : selection_)
		{
			auto sector = context_->map().sector(item.index);
			if (!sector)
				continue;

			// To lines mode
			if (to_edit_mode == Mode::Lines)
			{
				vector<MapLine*> lines;
				sector->putLines(lines);
				for (auto line : lines)
					new_selection.insert({ (int)line->index(), ItemType::Line });
			}

			// To vertices mode
			else if (to_edit_mode == Mode::Vertices)
			{
				vector<MapVertex*> vertices;
				sector->putVertices(vertices);
				for (auto vertex : vertices)
					new_selection.insert({ (int)vertex->index(), ItemType::Vertex });
			}

			// To things mode
			else if (to_edit_mode == Mode::Things)
			{
				// TODO this is much harder
			}
		}
	}

	// Lines can only reliably be migrated to vertices
	else if (from_edit_mode == Mode::Lines && to_edit_mode == Mode::Vertices)
	{
		for (auto& item : selection_)
		{
			auto line = context_->map().line(item.index);
			if (!line)
				continue;
			new_selection.insert({ (int)line->v1()->index(), ItemType::Vertex });
			new_selection.insert({ (int)line->v2()->index(), ItemType::Vertex });
		}
	}

	// Apply new selection
	selection_.assign(new_selection.begin(), new_selection.end());
}

// -----------------------------------------------------------------------------
// Selects or deselects [item] depending on the value of [select] and updates
// the current ChangeSet
// -----------------------------------------------------------------------------
void ItemSelection::selectItem(const MapEditor::Item& item, bool select)
{
	// Check if already selected
	bool selected = VECTOR_EXISTS(selection_, item);

	// (De)Select and update change set
	if (select && !selected)
	{
		selection_.push_back(item);
		last_change_[item] = true;
	}
	if (!select && selected)
	{
		VECTOR_REMOVE(selection_, item);
		last_change_[item] = false;
	}
}
