
#include "Main.h"
#include "MapEditor.h"
#include "ColourConfiguration.h"
#include "ArchiveManager.h"
#include "WxStuff.h"
#include "MapEditorWindow.h"
#include "GameConfiguration.h"
#include "MathStuff.h"
#include "Console.h"
#include "MapCanvas.h"
#include "MapObjectPropsPanel.h"
#include "SectorBuilder.h"
#include "Clipboard.h"
#include "UndoRedo.h"

double grid_sizes[] = { 0.05, 0.1, 0.25, 0.5, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536 };
CVAR(Bool, map_merge_undo_step, true, CVAR_SAVE);


EXTERN_CVAR(Int, shapedraw_sides)
EXTERN_CVAR(Int, shapedraw_shape)
EXTERN_CVAR(Bool, shapedraw_centered)
EXTERN_CVAR(Bool, shapedraw_lockratio)


#pragma region UNDO STEPS

class PropertyChangeUS : public UndoStep
{
private:
	mobj_backup_t*	backup;

public:
	PropertyChangeUS(MapObject* object)
	{
		backup = new mobj_backup_t();
		object->backup(backup);
	}

	~PropertyChangeUS()
	{
		delete backup;
	}

	void doSwap(MapObject* obj)
	{
		mobj_backup_t* temp = new mobj_backup_t();
		obj->backup(temp);
		obj->loadFromBackup(backup);
		delete backup;
		backup = temp;
	}

	bool doUndo()
	{
		MapObject* obj = UndoRedo::currentMap()->getObjectById(backup->id);
		if (obj) doSwap(obj);

		return true;
	}

	bool doRedo()
	{
		MapObject* obj = UndoRedo::currentMap()->getObjectById(backup->id);
		if (obj) doSwap(obj);

		return true;
	}
};

class MapObjectCreateDeleteUS : public UndoStep
{
private:
	vector<mobj_cd_t>	objects;

public:
	MapObjectCreateDeleteUS()
	{
		// Get recently created and deleted object ids from map
		// (in the order they were created/deleted)
		vector<mobj_cd_t>& cd_objects = UndoRedo::currentMap()->createdDeletedObjectIds();
		for (unsigned a = 0; a < cd_objects.size(); a++)
			objects.push_back(cd_objects[a]);
	}

	~MapObjectCreateDeleteUS() {}

	bool doUndo()
	{
		// Undo creation/deletion
		for (int a = objects.size() - 1; a >= 0; a--)
		{
			if (objects[a].created)
				UndoRedo::currentMap()->removeObjectById(objects[a].id);
			else
				UndoRedo::currentMap()->restoreObjectById(objects[a].id);
			//LOG_MESSAGE(4, "Restored object id %d (%d: %s)", object_ids[a], UndoRedo::currentMap()->getObjectById(object_ids[a])->getIndex(), CHR(UndoRedo::currentMap()->getObjectById(object_ids[a])->getTypeName()));
		}

		return true;
	}

	bool doRedo()
	{
		// Redo creation/deletion
		for (unsigned a = 0; a < objects.size(); a++)
		{
			if (!objects[a].created)
				UndoRedo::currentMap()->removeObjectById(objects[a].id);
			else
				UndoRedo::currentMap()->restoreObjectById(objects[a].id);
			//LOG_MESSAGE(4, "Removed object id %d (%s)", object_ids[a], CHR(UndoRedo::currentMap()->getObjectById(object_ids[a])->getTypeName()));
		}

		return true;
	}

	bool isOk()
	{
		return !objects.empty();
	}
};

class MultiMapObjectPropertyChangeUS : public UndoStep
{
private:
	vector<mobj_backup_t*>	backups;

public:
	MultiMapObjectPropertyChangeUS()
	{
		// Get backups of recently modified map objects
		vector<MapObject*> objects = UndoRedo::currentMap()->getAllModifiedObjects(MapObject::propBackupTime());
		for (unsigned a = 0; a < objects.size(); a++)
		{
			mobj_backup_t* bak = objects[a]->getBackup(true);
			if (bak)
			{
				backups.push_back(bak);
				//wxLogMessage("%s #%d modified", CHR(objects[a]->getTypeName()), objects[a]->getIndex());
			}
		}

		if (Global::log_verbosity >= 2)
		{
			string msg = "Modified ids: ";
			for (unsigned a = 0; a < backups.size(); a++)
				msg += S_FMT("%d, ", backups[a]->id);
			wxLogMessage(msg);
		}
	}

	~MultiMapObjectPropertyChangeUS()
	{
		for (unsigned a = 0; a < backups.size(); a++)
			delete backups[a];
	}

	void doSwap(MapObject* obj, unsigned index)
	{
		mobj_backup_t* temp = new mobj_backup_t();
		obj->backup(temp);
		obj->loadFromBackup(backups[index]);
		delete backups[index];
		backups[index] = temp;
	}

	bool doUndo()
	{
		for (unsigned a = 0; a < backups.size(); a++)
		{
			MapObject* obj = UndoRedo::currentMap()->getObjectById(backups[a]->id);
			if (obj) doSwap(obj, a);
		}

		return true;
	}

	bool doRedo()
	{
		LOG_MESSAGE(2, S_FMT("Restore %d objects", backups.size()));
		for (unsigned a = 0; a < backups.size(); a++)
		{
			MapObject* obj = UndoRedo::currentMap()->getObjectById(backups[a]->id);
			if (obj) doSwap(obj, a);
		}

		return true;
	}

	bool isOk()
	{
		return !backups.empty();
	}
};

#pragma endregion


MapEditor::MapEditor()
{
	// Init variables
	edit_mode = MODE_LINES;
	hilight_item = -1;
	gridsize = 9;
	canvas = NULL;
	hilight_locked = false;
	sector_mode = SECTOR_BOTH;
	grid_snap = true;
	copy_thing = NULL;
	copy_sector = NULL;
	copy_line = NULL;
	link_3d_light = true;
	link_3d_offset = true;
	undo_manager = new UndoManager(&map);
	undo_manager_3d = new UndoManager(&map);
	current_tag = 0;
}

MapEditor::~MapEditor()
{
	if (copy_thing) delete copy_thing;
	if (copy_sector) delete copy_sector;
	if (copy_line)
	{
		if(copy_line->s1())
		{
			delete copy_line->s1();
		}

		if(copy_line->s2())
		{
			delete copy_line->s2();
		}

		delete copy_line;
	}
	delete undo_manager;
	delete undo_manager_3d;
}

void MapEditor::setEditMode(int mode)
{
	// Check if we are changing to the same mode
	if (mode == edit_mode)
	{
		// Cycle sector edit mode
		if (mode == MODE_SECTORS)
			setSectorEditMode(sector_mode + 1);

		// Do nothing otherwise
		return;
	}

	// Clear 3d mode undo manager on exiting 3d mode
	if (edit_mode == MODE_3D && mode != MODE_3D)
		undo_manager_3d->clear();

	// Set edit mode
	edit_mode = mode;
	sector_mode = SECTOR_BOTH;

	// Clear hilight and selection stuff
	hilight_item = -1;
	selection.clear();
	tagged_sectors.clear();
	tagged_lines.clear();
	tagged_things.clear();
	last_undo_level = "";

	// Add editor message
	switch (edit_mode)
	{
	case MODE_VERTICES: addEditorMessage("Vertices mode"); break;
	case MODE_LINES:	addEditorMessage("Lines mode"); break;
	case MODE_SECTORS:	addEditorMessage("Sectors mode (Normal)"); break;
	case MODE_THINGS:	addEditorMessage("Things mode"); break;
	case MODE_3D:		addEditorMessage("3d mode"); break;
	default: break;
	};
}

void MapEditor::setSectorEditMode(int mode)
{
	// Set sector mode
	sector_mode = mode;
	if (sector_mode > SECTOR_CEILING || sector_mode < 0)
		sector_mode = SECTOR_BOTH;

	// Editor message
	if (sector_mode == SECTOR_BOTH)
		addEditorMessage("Sectors mode (Normal)");
	else if (sector_mode == SECTOR_FLOOR)
		addEditorMessage("Sectors mode (Floors)");
	else
		addEditorMessage("Sectors mode (Ceilings)");
}

bool MapEditor::openMap(Archive::mapdesc_t map)
{
	wxLogMessage("Opening map %s", CHR(map.name));
	if (!this->map.readMap(map))
		return false;

	// Find camera thing
	if (canvas)
	{
		MapThing* cam = NULL;
		MapThing* pstart = NULL;
		for (unsigned a = 0; a < this->map.nThings(); a++)
		{
			MapThing* thing = this->map.getThing(a);
			if (thing->getType() == 32000)
				cam = thing;
			if (thing->getType() == 1)
				pstart = thing;

			if (cam)
				break;
		}

		// Set canvas 3d camera
		if (cam)
			canvas->set3dCameraThing(cam);
		else if (pstart)
			canvas->set3dCameraThing(pstart);

		// Reset rendering data
		canvas->forceRefreshRenderer();
	}

	link_3d_light = true;
	link_3d_offset = true;

	return true;
}

void MapEditor::clearMap()
{
	// Clear map
	map.clearMap();

	// Clear selection
	selection.clear();
	hilight_item = -1;
	link_3d_light = true;
	link_3d_offset = true;

	// Clear undo manager
	undo_manager->clear();
	last_undo_level = "";
}

#pragma region GENERAL

void MapEditor::showItem(int index)
{
	selection.clear();
	int max = 0;
	switch (edit_mode)
	{
	case MODE_VERTICES: max = map.nVertices(); break;
	case MODE_LINES: max = map.nLines(); break;
	case MODE_SECTORS: max = map.nSectors(); break;
	case MODE_THINGS: max = map.nThings(); break;
	default: max = 0; break;
	}

	if (index < max)
	{
		selection.push_back(index);
		if (canvas) canvas->viewShowObject();
	}
}

string MapEditor::getModeString()
{
	switch (edit_mode)
	{
	case MODE_VERTICES: return "Vertices";
	case MODE_LINES: return "Lines";
	case MODE_SECTORS: return "Sectors";
	case MODE_THINGS: return "Things";
	case MODE_3D: return "3D";
	default: return "Items";
	};
}

#pragma endregion

#pragma region HILIGHT

bool MapEditor::updateHilight(fpoint2_t mouse_pos, double dist_scale)
{
	// Do nothing if hilight is locked
	if (hilight_locked)
		return false;

	int current = hilight_item;

	// Update hilighted object depending on mode
	if (edit_mode == MODE_VERTICES)
		hilight_item = map.nearestVertex(mouse_pos.x, mouse_pos.y, 32/dist_scale);
	else if (edit_mode == MODE_LINES)
		hilight_item = map.nearestLine(mouse_pos.x, mouse_pos.y, 32/dist_scale);
	else if (edit_mode == MODE_SECTORS)
		hilight_item = map.sectorAt(mouse_pos.x, mouse_pos.y);
	else if (edit_mode == MODE_THINGS)
	{
		hilight_item = -1;

		// Get (possibly multiple) nearest-thing(s)
		vector<int> nearest = map.nearestThingMulti(mouse_pos.x, mouse_pos.y);
		if (nearest.size() == 1)
		{
			MapThing* t = map.getThing(nearest[0]);
			ThingType* type = theGameConfiguration->thingType(t->getType());
			double dist = MathStuff::distance(mouse_pos.x, mouse_pos.y, t->xPos(), t->yPos());
			if (dist <= type->getRadius() + (32/dist_scale))
				hilight_item = nearest[0];
		}
		else
		{
			for (unsigned a = 0; a < nearest.size(); a++)
			{
				MapThing* t = map.getThing(nearest[a]);
				ThingType* type = theGameConfiguration->thingType(t->getType());
				double dist = MathStuff::distance(mouse_pos.x, mouse_pos.y, t->xPos(), t->yPos());
				if (dist <= type->getRadius() + (32/dist_scale))
					hilight_item = nearest[a];
			}
		}
	}

	// Update tagged lists if the hilight changed
	if (current != hilight_item)
		updateTagged();

	// Update map object properties panel if the hilight changed
	if (current != hilight_item && selection.empty())
	{
		switch (edit_mode)
		{
		case MODE_VERTICES: theMapEditor->propsPanel()->openObject(map.getVertex(hilight_item)); break;
		case MODE_LINES: theMapEditor->propsPanel()->openObject(map.getLine(hilight_item)); break;
		case MODE_SECTORS: theMapEditor->propsPanel()->openObject(map.getSector(hilight_item)); break;
		case MODE_THINGS: theMapEditor->propsPanel()->openObject(map.getThing(hilight_item)); break;
		default: break;
		}

		last_undo_level = "";
	}

	return current != hilight_item;
}

MapVertex* MapEditor::getHilightedVertex()
{
	// Check edit mode is correct
	if (edit_mode != MODE_VERTICES)
		return NULL;

	// Having one item selected counts as a hilight
	if (hilight_item == -1 && selection.size() == 1)
		return map.getVertex(selection[0]);

	return map.getVertex(hilight_item);
}

MapLine* MapEditor::getHilightedLine()
{
	// Check edit mode is correct
	if (edit_mode != MODE_LINES)
		return NULL;

	// Having one item selected counts as a hilight
	if (hilight_item == -1 && selection.size() == 1)
		return map.getLine(selection[0]);

	return map.getLine(hilight_item);
}

MapSector* MapEditor::getHilightedSector()
{
	// Check edit mode is correct
	if (edit_mode != MODE_SECTORS)
		return NULL;

	// Having one item selected counts as a hilight
	if (hilight_item == -1 && selection.size() == 1)
		return map.getSector(selection[0]);

	return map.getSector(hilight_item);
}

MapThing* MapEditor::getHilightedThing()
{
	// Check edit mode is correct
	if (edit_mode != MODE_THINGS)
		return NULL;

	// Having one item selected counts as a hilight
	if (hilight_item == -1 && selection.size() == 1)
		return map.getThing(selection[0]);

	return map.getThing(hilight_item);
}

MapObject* MapEditor::getHilightedObject()
{
	if (edit_mode == MODE_VERTICES)
		return getHilightedVertex();
	else if (edit_mode == MODE_LINES)
		return getHilightedLine();
	else if (edit_mode == MODE_SECTORS)
		return getHilightedSector();
	else if (edit_mode == MODE_THINGS)
		return getHilightedThing();
	else
		return NULL;
}

#pragma endregion

#pragma region TAGGING

void MapEditor::updateTagged()
{
	// Clear tagged lists
	tagged_sectors.clear();
	tagged_lines.clear();
	tagged_things.clear();

	tagging_lines.clear();
	tagging_things.clear();

	// Special
	if (hilight_item >= 0)
	{
		// Gather affecting objects
		int type, tag = 0;
		if (edit_mode == MODE_LINES)
		{
			type = SLADEMap::LINEDEFS;
			tag = map.getLine(hilight_item)->intProperty("id");
		}
		else if (edit_mode == MODE_THINGS)
		{
			type = SLADEMap::THINGS;
			tag = map.getThing(hilight_item)->intProperty("id");
		}
		else if (edit_mode == MODE_SECTORS)
		{
			type = SLADEMap::SECTORS;
			tag = map.getSector(hilight_item)->intProperty("id");
		}
		if (tag)
		{
			map.getTaggingLinesById(tag, type, tagging_lines);
			map.getTaggingThingsById(tag, type, tagging_things);
		}

		// Gather affected objects
		if (edit_mode == MODE_LINES || edit_mode == MODE_THINGS)
		{
			MapSector* back = NULL;
			MapSector* front = NULL;
			int needs_tag, tag, arg2, arg3, arg4, arg5;
			// Line specials have front and possibly back sectors
			if (edit_mode == MODE_LINES)
			{
				MapLine* line = map.getLine(hilight_item);
				if (line->s2()) back = line->s2()->getSector();
				if (line->s1()) front = line->s1()->getSector();
				needs_tag = theGameConfiguration->actionSpecial(line->intProperty("special"))->needsTag();
				tag = line->intProperty("arg0");
				arg2 = line->intProperty("arg1");
				arg3 = line->intProperty("arg2");
				arg4 = line->intProperty("arg3");
				arg5 = line->intProperty("arg4");

				// Hexen and UDMF things can have specials too
			}
			else /* edit_mode == MODE_THINGS */
			{
				MapThing* thing = map.getThing(hilight_item);
				needs_tag = theGameConfiguration->actionSpecial(thing->intProperty("special"))->needsTag();
				tag = thing->intProperty("arg0");
				arg2 = thing->intProperty("arg1");
				arg3 = thing->intProperty("arg2");
				arg4 = thing->intProperty("arg3");
				arg5 = thing->intProperty("arg4");
			}

			// Sector tag
			if (needs_tag == AS_TT_SECTOR ||
					needs_tag == AS_TT_SECTOR_AND_BACK && tag > 0)
				map.getSectorsByTag(tag, tagged_sectors);

			// Backside sector (for local doors)
			else if ((needs_tag == AS_TT_SECTOR_BACK || needs_tag == AS_TT_SECTOR_AND_BACK) && back)
				tagged_sectors.push_back(back);

			// Sector tag *or* backside sector (for zdoom local doors)
			else if (needs_tag == AS_TT_SECTOR_OR_BACK)
			{
				if (tag > 0)
					map.getSectorsByTag(tag, tagged_sectors);
				else if (back)
					tagged_sectors.push_back(back);
			}

			// Thing ID
			else if (needs_tag == AS_TT_THING)
				map.getThingsById(tag, tagged_things);

			// Line ID
			else if (needs_tag == AS_TT_LINE)
				map.getLinesById(tag, tagged_lines);

			// ZDoom quirkiness
			else if (needs_tag)
			{
				switch (needs_tag)
				{
				case AS_TT_1THING_2SECTOR:
				case AS_TT_1THING_3SECTOR:
				case AS_TT_1SECTOR_2THING:
				{
					int thingtag = (needs_tag == AS_TT_1SECTOR_2THING) ? arg2 : tag;
					int sectag = (needs_tag == AS_TT_1SECTOR_2THING) ? tag :
								 (needs_tag == AS_TT_1THING_2SECTOR) ? arg2 : arg3;
					if ((thingtag | sectag) == 0)
						break;
					else if (thingtag == 0)
						map.getSectorsByTag(sectag, tagged_sectors);
					else if (sectag == 0)
						map.getThingsById(thingtag, tagged_things);
					else // neither thingtag nor sectag are 0
						map.getThingsByIdInSectorTag(thingtag, sectag, tagged_things);
				}	break;
				case AS_TT_1THING_2THING_3THING:
					map.getThingsById(arg3, tagged_things);
				case AS_TT_1THING_2THING:
					map.getThingsById(arg2, tagged_things);
				case AS_TT_1THING_4THING:
					map.getThingsById(tag, tagged_things);
				case AS_TT_4THING:
					if (needs_tag == AS_TT_1THING_4THING || needs_tag == AS_TT_4THING)
						map.getThingsById(arg4, tagged_things);
					break;
				case AS_TT_5THING:
					map.getThingsById(arg5, tagged_things);
					break;
				case AS_TT_LINE_NEGATIVE:
					map.getLinesById(abs(tag), tagged_lines);
					break;
				case AS_TT_1LINEID_2LINE:
					map.getLinesById(arg2, tagged_lines);
					break;
				case AS_TT_1LINE_2SECTOR:
					map.getLinesById(tag, tagged_lines);
					map.getSectorsByTag(arg2, tagged_sectors);
					break;
				case AS_TT_1SECTOR_2THING_3THING_5THING:
					if (arg5) map.getThingsById(arg5, tagged_things);
					map.getThingsById(arg3, tagged_things);
				case AS_TT_1SECTOR_2SECTOR_3SECTOR_4SECTOR:
					if (arg4) map.getSectorsByTag(arg4, tagged_sectors);
					if (arg3) map.getSectorsByTag(arg3, tagged_sectors);
				case AS_TT_1SECTOR_2SECTOR:
					if (arg2) map.getSectorsByTag(arg2, tagged_sectors);
					if (tag ) map.getSectorsByTag(tag , tagged_sectors);
					break;
				case AS_TT_SECTOR_2IS3_LINE:
					if (arg2 == 3) map.getLinesById(tag, tagged_lines);
					else map.getSectorsByTag(tag, tagged_sectors);
					break;
				default:
					break;
				}
			}
		}
	}
}

#pragma endregion

#pragma region SELECTION

void MapEditor::selectionUpdated()
{
	// Open selected objects in properties panel
	vector<MapObject*> objects;

	if (edit_mode == MODE_VERTICES)
	{
		for (unsigned a = 0; a < selection.size(); a++)
			objects.push_back(map.getVertex(selection[a]));
	}
	else if (edit_mode == MODE_LINES)
	{
		for (unsigned a = 0; a < selection.size(); a++)
			objects.push_back(map.getLine(selection[a]));
	}
	else if (edit_mode == MODE_SECTORS)
	{
		for (unsigned a = 0; a < selection.size(); a++)
			objects.push_back(map.getSector(selection[a]));
	}
	else if (edit_mode == MODE_THINGS)
	{
		for (unsigned a = 0; a < selection.size(); a++)
			objects.push_back(map.getThing(selection[a]));
	}

	theMapEditor->propsPanel()->openObjects(objects);

	last_undo_level = "";
}

void MapEditor::clearSelection(bool animate)
{
	if (edit_mode == MODE_3D)
	{
		if (animate && canvas) canvas->itemsSelected3d(selection_3d, false);
		selection_3d.clear();
	}
	else
	{
		if (animate && canvas) canvas->itemsSelected(selection, false);
		selection.clear();
		theMapEditor->propsPanel()->openObject(NULL);
	}
}

void MapEditor::selectAll()
{
	// Clear selection initially
	selection.clear();

	// Select all items depending on mode
	if (edit_mode == MODE_VERTICES)
	{
		for (unsigned a = 0; a < map.vertices.size(); a++)
			selection.push_back(a);
	}
	else if (edit_mode == MODE_LINES)
	{
		for (unsigned a = 0; a < map.lines.size(); a++)
			selection.push_back(a);
	}
	else if (edit_mode == MODE_SECTORS)
	{
		for (unsigned a = 0; a < map.sectors.size(); a++)
			selection.push_back(a);
	}
	else if (edit_mode == MODE_THINGS)
	{
		for (unsigned a = 0; a < map.things.size(); a++)
			selection.push_back(a);
	}

	addEditorMessage(S_FMT("Selected all %d %s", selection.size(), CHR(getModeString())));

	if (canvas)
		canvas->itemsSelected(selection);

	selectionUpdated();
}

bool MapEditor::selectCurrent(bool clear_none)
{
	// --- 3d mode ---
	if (edit_mode == MODE_3D)
	{
		// If nothing is hilighted
		if (hilight_3d.index == -1)
		{
			// Clear selection if specified
			if (clear_none)
			{
				if (canvas) canvas->itemsSelected3d(selection_3d, false);
				selection_3d.clear();
				addEditorMessage("Selection cleared");
			}

			return false;
		}

		// Otherwise, check if item is in selection
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].index == hilight_3d.index &&
					selection_3d[a].type == hilight_3d.type)
			{
				// Already selected, deselect
				selection_3d.erase(selection_3d.begin() + a);
				if (canvas) canvas->itemSelected3d(hilight_3d, false);
				return true;
			}
		}

		// Not already selected, add to selection
		selection_3d.push_back(hilight_3d);
		if (canvas) canvas->itemSelected3d(hilight_3d);

		return true;
	}

	// --- 2d mode ---
	else
	{
		// If nothing is hilighted
		if (hilight_item == -1)
		{
			// Clear selection if specified
			if (clear_none)
			{
				if (canvas) canvas->itemsSelected(selection, false);
				selection.clear();
				selectionUpdated();
				addEditorMessage("Selection cleared");
			}

			return false;
		}

		// Otherwise, check if item is in selection
		for (unsigned a = 0; a < selection.size(); a++)
		{
			if (selection[a] == hilight_item)
			{
				// Already selected, deselect
				selection.erase(selection.begin() + a);
				if (canvas) canvas->itemSelected(hilight_item, false);
				selectionUpdated();
				return true;
			}
		}

		// Not already selected, add to selection
		selection.push_back(hilight_item);
		if (canvas) canvas->itemSelected(hilight_item, true);

		selectionUpdated();

		return true;
	}
}

bool MapEditor::selectWithin(double xmin, double ymin, double xmax, double ymax, bool add)
{
	// Select depending on editing mode
	bool selected;
	vector<int> nsel;
	vector<int> asel;

	// Vertices
	if (edit_mode == MODE_VERTICES)
	{
		// Go through vertices
		double x, y;
		for (unsigned a = 0; a < map.vertices.size(); a++)
		{
			// Check if already selected
			if (std::find(selection.begin(), selection.end(), a) != selection.end())
				selected = true;
			else
				selected = false;

			// Get position
			x = map.vertices[a]->xPos();
			y = map.vertices[a]->yPos();

			// Select if vertex is within bounds
			if (xmin <= x && x <= xmax && ymin <= y && y <= ymax)
			{
				if (selected)
					asel.push_back(a);
				else
					nsel.push_back(a);
			}
		}
	}

	// Lines
	else if (edit_mode == MODE_LINES)
	{
		// Go through lines
		MapLine* line;
		double x1, y1, x2, y2;
		for (unsigned a = 0; a < map.lines.size(); a++)
		{
			// Check if already selected
			if (std::find(selection.begin(), selection.end(), a) != selection.end())
				selected = true;
			else
				selected = false;

			// Get vertex positions
			line = map.lines[a];
			x1 = line->v1()->xPos();
			y1 = line->v1()->yPos();
			x2 = line->v2()->xPos();
			y2 = line->v2()->yPos();

			// Select if both vertices are within bounds
			if (xmin <= x1 && x1 <= xmax && ymin <= y1 && y1 <= ymax &&
					xmin <= x2 && x2 <= xmax && ymin <= y2 && y2 <= ymax)
			{
				if (selected)
					asel.push_back(a);
				else
					nsel.push_back(a);
			}
		}
	}

	// Sectors
	else if (edit_mode == MODE_SECTORS)
	{
		// Go through sectors
		fpoint2_t pmin(xmin, ymin);
		fpoint2_t pmax(xmax, ymax);
		for (unsigned a = 0; a < map.sectors.size(); a++)
		{
			// Check if already selected
			if (std::find(selection.begin(), selection.end(), a) != selection.end())
				selected = true;
			else
				selected = false;

			// Check if sector's bbox fits within the selection box
			if (map.sectors[a]->boundingBox().is_within(pmin, pmax))
			{
				if (selected)
					asel.push_back(a);
				else
					nsel.push_back(a);
			}
		}
	}

	// Things
	else if (edit_mode == MODE_THINGS)
	{
		// Go through things
		double x, y;
		for (unsigned a = 0; a < map.things.size(); a++)
		{
			// Check if already selected
			if (std::find(selection.begin(), selection.end(), a) != selection.end())
				selected = true;
			else
				selected = false;

			// Get position
			x = map.things[a]->xPos();
			y = map.things[a]->yPos();

			// Select if thing is within bounds
			if (xmin <= x && x <= xmax && ymin <= y && y <= ymax)
			{
				if (selected)
					asel.push_back(a);
				else
					nsel.push_back(a);
			}
		}
	}


	// Clear selection if anything was within the box
	if (!add && (nsel.size() > 0 || asel.size() > 0))
		clearSelection();

	// Update selection
	if (!add)
	{
		for (unsigned a = 0; a < asel.size(); a++)
			selection.push_back(asel[a]);
	}
	for (unsigned a = 0; a < nsel.size(); a++)
		selection.push_back(nsel[a]);

	if (add)
		addEditorMessage(S_FMT("Selected %d %s", asel.size(), CHR(getModeString())));
	else
		addEditorMessage(S_FMT("Selected %d %s", selection.size(), CHR(getModeString())));

	// Animate newly selected items
	if (canvas && nsel.size() > 0) canvas->itemsSelected(nsel);

	selectionUpdated();

	return (nsel.size() > 0);
}

void MapEditor::getSelectedVertices(vector<MapVertex*>& list)
{
	if (edit_mode != MODE_VERTICES)
		return;

	// Multiple selection
	if (selection.size() > 1)
	{
		for (unsigned a = 0; a < selection.size(); a++)
			list.push_back(map.getVertex(selection[a]));
	}

	// Single selection
	else if (selection.size() == 1)
		list.push_back(map.getVertex(selection[0]));

	// No selection (use hilight)
	else if (hilight_item >= 0)
		list.push_back(map.getVertex(hilight_item));
}

void MapEditor::getSelectedLines(vector<MapLine*>& list)
{
	if (edit_mode == MODE_LINES)
	{
		// Multiple selection
		if (selection.size() > 1)
		{
			for (unsigned a = 0; a < selection.size(); a++)
				list.push_back(map.getLine(selection[a]));
		}

		// Single selection
		else if (selection.size() == 1)
			list.push_back(map.getLine(selection[0]));

		// No selection (use hilight)
		else if (hilight_item >= 0)
			list.push_back(map.getLine(hilight_item));
	}
	else if (edit_mode == MODE_SECTORS)
	{
		// Get selected sectors
		vector<MapSector*> sectors;
		getSelectedSectors(sectors);

		// Add lines of selected sectors
		for (unsigned a = 0; a < sectors.size(); a++)
		{
			vector<MapLine*> seclines;
			sectors[a]->getLines(seclines);
			for (unsigned b = 0; b < seclines.size(); b++)
			{
				if (std::find(list.begin(), list.end(), seclines[b]) == list.end())
					list.push_back(seclines[b]);
			}
		}
	}
}

void MapEditor::getSelectedSectors(vector<MapSector*>& list)
{
	if (edit_mode != MODE_SECTORS)
		return;

	// Multiple selection
	if (selection.size() > 1)
	{
		for (unsigned a = 0; a < selection.size(); a++)
			list.push_back(map.getSector(selection[a]));
	}

	// Single selection
	else if (selection.size() == 1)
		list.push_back(map.getSector(selection[0]));

	// No selection (use hilight)
	else if (hilight_item >= 0)
		list.push_back(map.getSector(hilight_item));
}

void MapEditor::getSelectedThings(vector<MapThing*>& list)
{
	if (edit_mode == MODE_3D)
	{
		// Multiple selection
		if (selection_3d.size() > 1)
		{
			for (unsigned a = 0; a < selection_3d.size(); a++)
			{
				if (selection_3d[a].type == SEL_THING)
					list.push_back(map.getThing(selection_3d[a].index));
			}
		}

		// Single selection
		else if (selection_3d.size() == 1 && selection_3d[0].type == SEL_THING)
			list.push_back(map.getThing(selection_3d[0].index));

		// No selection (use hilight
		else if (hilight_3d.index >= 0 && hilight_3d.type == SEL_THING)
			list.push_back(map.getThing(hilight_3d.index));
	}
	else if (edit_mode == MODE_THINGS)
	{
		// Multiple selection
		if (selection.size() > 1)
		{
			for (unsigned a = 0; a < selection.size(); a++)
				list.push_back(map.getThing(selection[a]));
		}

		// Single selection
		else if (selection.size() == 1)
			list.push_back(map.getThing(selection[0]));

		// No selection (use hilight)
		else if (hilight_item >= 0)
			list.push_back(map.getThing(hilight_item));
	}
}

void MapEditor::getSelectedObjects(vector<MapObject*>& list)
{
	// Go through selection
	if (selection.size() > 0)
	{
		for (unsigned a = 0; a < selection.size(); a++)
		{
			if (edit_mode == MODE_VERTICES)
				list.push_back(map.getVertex(selection[a]));
			else if (edit_mode == MODE_LINES)
				list.push_back(map.getLine(selection[a]));
			else if (edit_mode == MODE_SECTORS)
				list.push_back(map.getSector(selection[a]));
			else if (edit_mode == MODE_THINGS)
				list.push_back(map.getThing(selection[a]));
		}
	}
	else
	{
		if (edit_mode == MODE_VERTICES)
			list.push_back(map.getVertex(hilight_item));
		else if (edit_mode == MODE_LINES)
			list.push_back(map.getLine(hilight_item));
		else if (edit_mode == MODE_SECTORS)
			list.push_back(map.getSector(hilight_item));
		else if (edit_mode == MODE_THINGS)
			list.push_back(map.getThing(hilight_item));
	}
}

void MapEditor::selectItem3d(selection_3d_t item, int sel)
{
	// Go through selection
	for (unsigned a = 0; a < selection_3d.size(); a++)
	{
		// Check for match
		if (selection_3d[a].index == item.index && selection_3d[a].type == item.type)
		{
			// Selecting, do nothing
			if (sel == SELECT)
				return;

			// Deselecting, remove from selection list
			else if (sel == DESELECT || sel == TOGGLE)
			{
				selection_3d[a] = selection_3d.back();
				selection_3d.pop_back();
				last_undo_level = "";
				return;
			}
		}
	}

	// Selection didn't exist, add if selecting or toggling
	if (sel == SELECT || sel == TOGGLE)
	{
		selection_3d.push_back(item);
		last_undo_level = "";
		if (canvas) canvas->itemSelected3d(item);
	}
}

void MapEditor::get3dSelectionOrHilight(vector<selection_3d_t>& list)
{
	if (selection_3d.empty() && hilight_3d.index >= 0)
		list.push_back(hilight_3d);
	else if (!selection_3d.empty())
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
			list.push_back(selection_3d[a]);
	}
}

#pragma endregion

#pragma region GRID

double MapEditor::gridSize()
{
	return grid_sizes[gridsize];
}

void MapEditor::incrementGrid()
{
	gridsize++;
	if (gridsize > 20)
		gridsize = 20;

	addEditorMessage(S_FMT("Grid Size: %dx%d", (int)gridSize(), (int)gridSize()));
}

void MapEditor::decrementGrid()
{
	gridsize--;
	if (gridsize < 0)
		gridsize = 0;

	addEditorMessage(S_FMT("Grid Size: %dx%d", (int)gridSize(), (int)gridSize()));
}

double MapEditor::snapToGrid(double position)
{
	return ceil(position / gridSize() - 0.5) * gridSize();
}


#pragma endregion

#pragma region EDITING

#pragma region MOVE

bool MapEditor::beginMove(fpoint2_t mouse_pos)
{
	// Check if we have any selection or hilight
	if (selection.size() == 0 && hilight_item == -1)
		return false;

	// Begin move operation
	move_origin = mouse_pos;

	// Create list of objects to move
	if (selection.size() == 0)
		move_items.push_back(hilight_item);
	else
	{
		for (unsigned a = 0; a < selection.size(); a++)
			move_items.push_back(selection[a]);
	}

	// Get list of vertices being moved (if any)
	vector<MapVertex*> move_verts;
	if (edit_mode != MODE_THINGS)
	{
		// Vertices mode
		if (edit_mode == MODE_VERTICES)
		{
			for (unsigned a = 0; a < move_items.size(); a++)
				move_verts.push_back(map.getVertex(move_items[a]));
		}

		// Lines mode
		else if (edit_mode == MODE_LINES)
		{
			for (unsigned a = 0; a < move_items.size(); a++)
			{
				// Duplicate vertices shouldn't matter here
				move_verts.push_back(map.getLine(move_items[a])->v1());
				move_verts.push_back(map.getLine(move_items[a])->v2());
			}
		}

		// Sectors mode
		else if (edit_mode == MODE_SECTORS)
		{
			for (unsigned a = 0; a < move_items.size(); a++)
				map.getSector(move_items[a])->getVertices(move_verts);
		}
	}

	// Filter out map objects being moved
	if (edit_mode == MODE_THINGS)
	{
		// Filter moving things
		for (unsigned a = 0; a < move_items.size(); a++)
			map.getThing(move_items[a])->filter(true);
	}
	else
	{
		// Filter moving lines
		for (unsigned a = 0; a < move_verts.size(); a++)
		{
			for (unsigned l = 0; l < move_verts[a]->nConnectedLines(); l++)
				move_verts[a]->connectedLine(l)->filter(true);
		}
	}

	return true;
}

void MapEditor::doMove(fpoint2_t mouse_pos)
{
	// Special case: single vertex or thing
	if (move_items.size() == 1 && (edit_mode == MODE_VERTICES || edit_mode == MODE_THINGS))
	{
		// Get new position
		double nx = snapToGrid(mouse_pos.x);
		double ny = snapToGrid(mouse_pos.y);

		// Update move vector
		if (edit_mode == MODE_VERTICES)
			move_vec.set(nx - map.getVertex(move_items[0])->xPos(), ny - map.getVertex(move_items[0])->yPos());
		else if (edit_mode == MODE_THINGS)
			move_vec.set(nx - map.getThing(move_items[0])->xPos(), ny - map.getThing(move_items[0])->yPos());

		return;
	}

	// Get amount moved
	double dx = mouse_pos.x - move_origin.x;
	double dy = mouse_pos.y - move_origin.y;

	// Update move vector
	move_vec.set(snapToGrid(dx), snapToGrid(dy));
}

void MapEditor::endMove(bool accept)
{
	long move_time = theApp->runTimer();

	// Un-filter objects
	for (unsigned a = 0; a < map.nLines(); a++)
		map.getLine(a)->filter(false);
	for (unsigned a = 0; a < map.nThings(); a++)
		map.getThing(a)->filter(false);

	// Move depending on edit mode
	if (edit_mode == MODE_THINGS && accept)
	{
		// Move things
		beginUndoRecord("Move Things", true, false, false);
		for (unsigned a = 0; a < move_items.size(); a++)
		{
			MapThing* t = map.getThing(move_items[a]);
			undo_manager->recordUndoStep(new PropertyChangeUS(t));
			map.moveThing(move_items[a], t->xPos() + move_vec.x, t->yPos() + move_vec.y);
		}
		endUndoRecord(true);
	}
	else if (accept)
	{
		// Any other edit mode we're technically moving vertices
		beginUndoRecord(S_FMT("Move %s", CHR(getModeString())));

		// Get list of vertices being moved
		bool* move_verts = new bool[map.nVertices()];
		memset(move_verts, 0, map.nVertices());

		if (edit_mode == MODE_VERTICES)
		{
			for (unsigned a = 0; a < move_items.size(); a++)
				move_verts[move_items[a]] = true;
		}
		else if (edit_mode == MODE_LINES)
		{
			for (unsigned a = 0; a < move_items.size(); a++)
			{
				MapLine* line = map.getLine(move_items[a]);
				if (line->v1()) move_verts[line->v1()->getIndex()] = true;
				if (line->v2()) move_verts[line->v2()->getIndex()] = true;
			}
		}
		else if (edit_mode == MODE_SECTORS)
		{
			vector<MapVertex*> sv;
			for (unsigned a = 0; a < move_items.size(); a++)
				map.getSector(move_items[a])->getVertices(sv);

			for (unsigned a = 0; a < sv.size(); a++)
				move_verts[sv[a]->getIndex()] = true;
		}

		// Move vertices
		vector<MapVertex*> moved_verts;
		for (unsigned a = 0; a < map.nVertices(); a++)
		{
			if (!move_verts[a])
				continue;
			fpoint2_t np(map.getVertex(a)->xPos() + move_vec.x, map.getVertex(a)->yPos() + move_vec.y);
			map.moveVertex(a, np.x, np.y);
			moved_verts.push_back(map.getVertex(a));
		}

		// Begin extra 'Merge' undo step if wanted
		bool merge = true;
		if (map_merge_undo_step)
		{
			endUndoRecord(true);
			beginUndoRecord("Merge");
		}

		//mergeLines(move_time, merge_points);
		merge = map.mergeArch(moved_verts);

		endUndoRecord(merge);
	}

	// Clear selection
	if (accept)
		clearSelection(false);

	// Clear moving items
	move_items.clear();

	// Update map item indices
	map.refreshIndices();
}

void MapEditor::mergeLines(long move_time, vector<fpoint2_t>& merge_points)
{

	// Merge vertices and split lines
	for (unsigned a = 0; a < merge_points.size(); a++)
	{
		MapVertex* v = map.mergeVerticesPoint(merge_points[a].x, merge_points[a].y);
		if (v) map.splitLinesAt(v, 1);
	}

	// Split lines overlapping vertices
	for (unsigned a = 0; a < map.nLines(); a++)
	{
		MapLine* line = map.getLine(a);
		if (line->modifiedTime() >= move_time)
		{
			MapVertex* split = map.lineCrossVertex(line->x1(), line->y1(), line->x2(), line->y2());
			if (split)
			{
				map.splitLine(a, split->getIndex());
				a = 0;
			}
		}
	}

	// Merge lines
	for (unsigned a = 0; a < map.nLines(); a++)
	{
		if (map.getLine(a)->modifiedTime() >= move_time)
		{
			if (map.mergeLine(a) > 0 && a < map.nLines())
			{
				map.getLine(a)->clearUnneededTextures();
				a = 0;
			}
		}
	}

	// Remove any resulting zero-length lines
	map.removeZeroLengthLines();

}

#pragma endregion

#pragma region LINES

void MapEditor::splitLine(double x, double y, double min_dist)
{
	// Get the closest line
	int lindex = map.nearestLine(x, y, min_dist);
	MapLine* line = map.getLine(lindex);

	// Do nothing if no line is close enough
	if (!line)
		return;

	// Begin recording undo level
	beginUndoRecord("Split Line", true, true, false);

	// Get closest point on the line
	fpoint2_t closest = MathStuff::closestPointOnLine(x, y, line->x1(), line->y1(), line->x2(), line->y2());

	// Create vertex there
	MapVertex* vertex = map.createVertex(closest.x, closest.y);

	// Do line split
	map.splitLine(lindex, vertex->getIndex());

	// Finish recording undo level
	endUndoRecord();
}

void MapEditor::flipLines(bool sides)
{
	// Get selected/hilighted line(s)
	vector<MapLine*> lines;
	getSelectedLines(lines);

	if (lines.empty())
		return;

	// Go through list
	undo_manager->beginRecord("Flip Line");
	for (unsigned a = 0; a < lines.size(); a++)
	{
		undo_manager->recordUndoStep(new PropertyChangeUS(lines[a]));
		lines[a]->flip(sides);
	}
	undo_manager->endRecord(true);

	// Update display
	canvas->forceRefreshRenderer();
	updateDisplay();
}

#pragma endregion

#pragma region SECTORS

void MapEditor::changeSectorHeight(int amount, bool floor, bool ceiling)
{
	// Do nothing if not in sectors mode
	if (edit_mode != MODE_SECTORS)
		return;

	// Get selected sectors (if any)
	vector<MapSector*> selection;
	getSelectedSectors(selection);

	// Do nothing if no selection or hilight
	if (selection.size() == 0)
		return;

	// If we're modifying both heights, take sector_mode into account
	if (floor && ceiling)
	{
		if (sector_mode == SECTOR_FLOOR)
			ceiling = false;
		if (sector_mode == SECTOR_CEILING)
			floor = false;
	}

	// Begin record undo level
	beginUndoRecordLocked("Change Sector Height", true, false, false);

	// Go through selection
	for (unsigned a = 0; a < selection.size(); a++)
	{
		// Change floor height
		if (floor)
		{
			int height = selection[a]->intProperty("heightfloor");
			selection[a]->setIntProperty("heightfloor", height + amount);
		}

		// Change ceiling height
		if (ceiling)
		{
			int height = selection[a]->intProperty("heightceiling");
			selection[a]->setIntProperty("heightceiling", height + amount);
		}
	}

	// End record undo level
	endUndoRecord();

	// Add editor message
	string what = "";
	if (floor && !ceiling)
		what = "Floor";
	else if (!floor && ceiling)
		what = "Ceiling";
	else
		what = "Floor and ceiling";
	string inc = "increased";
	if (amount < 0)
		inc = "decreased";
	if (amount < 0)
		amount = -amount;
	addEditorMessage(S_FMT("%s height %s by %d", CHR(what), CHR(inc), amount));

	// Update display
	updateDisplay();
}

void MapEditor::changeSectorLight(bool up, bool fine)
{
	// Do nothing if not in sectors mode
	if (edit_mode != MODE_SECTORS)
		return;

	// Get selected sectors (if any)
	vector<MapSector*> selection;
	getSelectedSectors(selection);

	// Do nothing if no selection or hilight
	if (selection.size() == 0)
		return;

	// Begin record undo level
	beginUndoRecordLocked("Change Sector Light", true, false, false);

	// Go through selection
	for (unsigned a = 0; a < selection.size(); a++)
	{
		// Get current light
		int light = selection[a]->intProperty("lightlevel");

		// Increment/decrement
		if (up)
			light = fine ? light+1 : theGameConfiguration->upLightLevel(light);
		else
			light = fine ? light-1 : theGameConfiguration->downLightLevel(light);

		// Change light level
		selection[a]->setIntProperty("lightlevel", light);
	}

	// End record undo level
	endUndoRecord();

	// Add editor message
	string dir = up ? "increased" : "decreased";
	int amount = fine ? 1 : theGameConfiguration->lightLevelInterval();
	addEditorMessage(S_FMT("Light level %s by %d", CHR(dir), amount));

	// Update display
	updateDisplay();
}

void MapEditor::joinSectors(bool remove_lines)
{
	// Check edit mode
	if (edit_mode != MODE_SECTORS)
		return;

	// Check selection
	if (selection.size() < 2)
		return;

	// Get 'target' sector
	MapSector* target = map.getSector(selection[0]);

	// Get sectors to merge
	vector<MapSector*> sectors;
	getSelectedSectors(sectors);

	// Clear selection
	clearSelection();

	// Init list of lines
	vector<MapLine*> lines;

	// Begin recording undo level
	beginUndoRecord("Join/Merge Sectors", true, false, true);

	// Go through merge sectors
	for (unsigned a = 1; a < sectors.size(); a++)
	{
		// Go through sector sides
		MapSector* sector = sectors[a];
		while (sector->connectedSides().size() > 0)
		{
			// Set sector
			MapSide* side = sector->connectedSides()[0];
			side->setSector(target);

			// Add line to list if not already there
			bool exists = false;
			for (unsigned l = 0; l < lines.size(); l++)
			{
				if (side->getParentLine() == lines[l])
				{
					exists = true;
					break;
				}
			}
			if (!exists)
				lines.push_back(side->getParentLine());
		}

		// Delete sector
		map.removeSector(sector);
	}

	// Remove any changed lines that now have the target sector on both sides (if needed)
	int nlines = 0;
	if (remove_lines)
	{
		for (unsigned a = 0; a < lines.size(); a++)
		{
			if (lines[a]->frontSector() == target && lines[a]->backSector() == target)
			{
				map.removeLine(lines[a]);
				nlines++;
			}
		}
	}

	// Finish recording undo level
	endUndoRecord();

	// Editor message
	if (nlines == 0)
		addEditorMessage(S_FMT("Joined %d Sectors", selection.size()));
	else
		addEditorMessage(S_FMT("Joined %d Sectors (removed %d Lines)", selection.size(), nlines));
}

#pragma endregion

#pragma region THINGS

void MapEditor::changeThingType(int newtype)
{
	// Do nothing if not in things mode
	if (edit_mode != MODE_THINGS && edit_mode != MODE_3D)
		return;

	// Get selected things (if any)
	vector<MapThing*> selection;
	getSelectedThings(selection);

	// Do nothing if no selection or hilight
	if (selection.size() == 0)
		return;

	// Go through selection
	beginUndoRecord("Thing Type Change", true, false, false);
	for (unsigned a = 0; a < selection.size(); a++)
		selection[a]->setIntProperty("type", newtype);
	endUndoRecord(true);

	// Add editor message
	string type_name = theGameConfiguration->thingType(newtype)->getName();
	if (selection.size() == 1)
		addEditorMessage(S_FMT("Changed type to \"%s\"", CHR(type_name)));
	else
		addEditorMessage(S_FMT("Changed %d things to type \"%s\"", selection.size(), CHR(type_name)));

	// Update display
	updateDisplay();
}

void MapEditor::thingQuickAngle(fpoint2_t mouse_pos)
{
	// Do nothing if not in things mode
	if (edit_mode != MODE_THINGS)
		return;

	// If selection is empty, check for hilight
	if (selection.size() == 0 && hilight_item >= 0)
	{
		MapThing* thing = map.getThing(hilight_item);
		map.getThing(hilight_item)->setAnglePoint(mouse_pos);
		return;
	}

	// Go through selection
	for (unsigned a = 0; a < selection.size(); a++)
	{
		map.getThing(selection[a])->setAnglePoint(mouse_pos);
	}
}

#pragma endregion

#pragma region TAG EDIT

int MapEditor::beginTagEdit()
{
	// Check lines mode
	if (edit_mode != MODE_LINES)
		return 0;

	// Get selected lines
	vector<MapLine*> lines;
	getSelectedLines(lines);
	if (lines.size() == 0)
		return 0;

	// Get current tag
	int tag = lines[0]->intProperty("arg0");
	if (tag == 0)
		tag = map.findUnusedSectorTag();
	current_tag = tag;

	// Clear tagged lists
	tagged_lines.clear();
	tagged_sectors.clear();
	tagged_things.clear();

	// Sector tag (for now, 2 will be thing id tag)
	for (unsigned a = 0; a < map.nSectors(); a++)
	{
		MapSector* sector = map.getSector(a);
		if (sector->intProperty("id") == current_tag)
			tagged_sectors.push_back(sector);
	}
	return 1;
}

void MapEditor::tagSectorAt(double x, double y)
{
	int index = map.sectorAt(x, y);
	if (index < 0)
		return;

	MapSector* sector = map.getSector(index);
	for (unsigned a = 0; a < tagged_sectors.size(); a++)
	{
		// Check if already tagged
		if (tagged_sectors[a] == sector)
		{
			// Un-tag
			tagged_sectors[a] = tagged_sectors.back();
			tagged_sectors.pop_back();
			addEditorMessage(S_FMT("Untagged sector %d", sector->getIndex()));
			return;
		}
	}

	// Tag
	tagged_sectors.push_back(sector);
	addEditorMessage(S_FMT("Tagged sector %d", sector->getIndex()));
}

void MapEditor::endTagEdit(bool accept)
{
	// Get selected lines
	vector<MapLine*> lines;
	getSelectedLines(lines);

	if (accept)
	{
		// Begin undo level
		beginUndoRecord("Tag Edit", true, false, false);

		// Clear sector tags
		for (unsigned a = 0; a < map.nSectors(); a++)
		{
			MapSector* sector = map.getSector(a);
			if (sector->intProperty("id") == current_tag)
				sector->setIntProperty("id", 0);
		}

		// If nothing selected, clear line tags
		if (tagged_sectors.size() == 0)
			current_tag = 0;

		// Set line tags (in case of multiple selection)
		for (unsigned a = 0; a < lines.size(); a++)
			lines[a]->setIntProperty("arg0", current_tag);

		// Set sector tags
		for (unsigned a = 0; a < tagged_sectors.size(); a++)
			tagged_sectors[a]->setIntProperty("id", current_tag);

		// Editor message
		if (tagged_sectors.size() == 0)
			addEditorMessage("Cleared tags");
		else
			addEditorMessage(S_FMT("Set tag %d", current_tag));

		endUndoRecord(true);
	}
	else
		addEditorMessage("Tag edit cancelled");
}

#pragma endregion

#pragma region OBJECT CREATION

void MapEditor::createObject(double x, double y)
{
	// Vertices mode
	if (edit_mode == MODE_VERTICES)
	{
		// If there are less than 2 vertices currently selected, just create a vertex at x,y
		if (selection.size() < 2)
			createVertex(x, y);
		else
		{
			// Otherwise, create lines between selected vertices
			beginUndoRecord("Create Lines", false, true, false);
			for (unsigned a = 0; a < selection.size() - 1; a++)
				map.createLine(map.getVertex(selection[a]), map.getVertex(selection[a+1]));
			endUndoRecord(true);

			// Editor message
			addEditorMessage(S_FMT("Created %d line(s)", selection.size() - 1));

			// Clear selection
			clearSelection();
		}

		return;
	}

	// Sectors mode
	if (edit_mode == MODE_SECTORS)
	{
		// Sector
		if (map.nLines() > 0)
		{
			createSector(x, y);
		}
		else
		{
			// Just create a vertex
			createVertex(x, y);
			setEditMode(MODE_LINES);
		}
		return;
	}

	// Things mode
	if (edit_mode == MODE_THINGS)
	{
		createThing(x, y);
		return;
	}
}

void MapEditor::createVertex(double x, double y)
{
	// Snap coordinates to grid if necessary
	if (grid_snap)
	{
		x = snapToGrid(x);
		y = snapToGrid(y);
	}

	// Create vertex
	beginUndoRecord("Create Vertex", true, true, false);
	MapVertex* vertex = map.createVertex(x, y, 2);
	endUndoRecord(true);

	// Editor message
	if (vertex)
		addEditorMessage(S_FMT("Created vertex at (%d, %d)", (int)vertex->xPos(), (int)vertex->yPos()));
}

void MapEditor::createThing(double x, double y)
{
	// Snap coordinates to grid if necessary
	if (grid_snap)
	{
		x = snapToGrid(x);
		y = snapToGrid(y);
	}

	// Begin undo step
	beginUndoRecord("Create Thing", false, true, false);

	// Create thing
	MapThing* thing = map.createThing(x, y);

	// Setup properties
	if (copy_thing)
	{
		// Copy properties from the last copied thing (except position)
		double x = thing->xPos();
		double y = thing->yPos();
		thing->copy(copy_thing);
		thing->setFloatProperty("x", x);
		thing->setFloatProperty("y", y);
	}
	else
		theGameConfiguration->applyDefaults(thing);	// No thing properties to copy, get defaults from game configuration

	// End undo step
	endUndoRecord(true);

	// Editor message
	if (thing)
		addEditorMessage(S_FMT("Created thing at (%d, %d)", (int)thing->xPos(), (int)thing->yPos()));
}

void MapEditor::createSector(double x, double y)
{
	// Find nearest line
	int nearest = map.nearestLine(x, y, 99999999);
	MapLine* line = map.getLine(nearest);

	// Determine side
	double side = MathStuff::lineSide(x, y, line->x1(), line->y1(), line->x2(), line->y2());

	// Get sector to copy if we're in sectors mode
	MapSector* sector_copy = NULL;
	if (edit_mode == MODE_SECTORS && selection.size() > 0)
		sector_copy = map.getSector(selection[0]);

	// Run sector builder
	SectorBuilder builder;
	bool ok;
	if (side >= 0)
		ok = builder.traceSector(&map, line, true);
	else
		ok = builder.traceSector(&map, line, false);

	// Create sector from builder result
	if (ok)
	{
		beginUndoRecord("Create Sector", true, true, false);
		builder.createSector(NULL, sector_copy);
	}

	// Set some sector defaults from game configuration if needed
	if (!sector_copy && ok)
	{
		MapSector* n_sector = map.getSector(map.nSectors()-1);
		if (n_sector->getCeilingTex().IsEmpty())
			theGameConfiguration->applyDefaults(n_sector);
	}

	// Editor message
	if (ok)
	{
		addEditorMessage(S_FMT("Created sector #%d", map.nSectors() - 1));
		endUndoRecord(true);
	}
	else
		addEditorMessage("Sector creation failed: " + builder.getError());

	// Refresh map canvas
	canvas->forceRefreshRenderer();
}

#pragma endregion

#pragma region OBJECT DELETION

void MapEditor::deleteObject()
{
	vector<MapObject*> objects_prop_change;

	// Vertices mode
	if (edit_mode == MODE_VERTICES)
	{
		// Get selected vertices
		vector<MapVertex*> verts;
		getSelectedVertices(verts);
		int index = -1;
		if (verts.size() == 1)
			index = verts[0]->getIndex();

		// Begin undo step
		beginUndoRecord("Delete Vertices", false, false, true);
		//undo_manager->beginRecord("Delete Vertices");
		//map.clearDeletedObjectIds();

		// Delete them (if any)
		for (unsigned a = 0; a < verts.size(); a++)
			map.removeVertex(verts[a]);

		// Remove detached vertices
		map.removeDetachedVertices();

		// Editor message
		if (verts.size() == 1)
			addEditorMessage(S_FMT("Deleted vertex #%d", index));
		else if (verts.size() > 1)
			addEditorMessage(S_FMT("Deleted %d vertices", verts.size()));
	}

	// Lines mode
	else if (edit_mode == MODE_LINES)
	{
		// Get selected lines
		vector<MapLine*> lines;
		getSelectedLines(lines);
		int index = -1;
		if (lines.size() == 1)
			index = lines[0]->getIndex();

		// Begin undo step
		beginUndoRecord("Delete Lines", false, false, true);

		// Delete them (if any)
		for (unsigned a = 0; a < lines.size(); a++)
			map.removeLine(lines[a]);

		// Remove detached vertices
		map.removeDetachedVertices();

		// Editor message
		if (lines.size() == 1)
			addEditorMessage(S_FMT("Deleted line #%d", index));
		else if (lines.size() > 1)
			addEditorMessage(S_FMT("Deleted %d lines", lines.size()));
	}

	// Sectors mode
	else if (edit_mode == MODE_SECTORS)
	{
		// Get selected sectors
		vector<MapSector*> sectors;
		getSelectedSectors(sectors);
		int index = -1;
		if (sectors.size() == 1)
			index = sectors[0]->getIndex();

		// Begin undo step
		beginUndoRecord("Delete Sectors", true, false, true);

		// Delete them (if any), and keep lists of connected lines and sides
		vector<MapSide*> connected_sides;
		vector<MapLine*> connected_lines;
		for (unsigned a = 0; a < sectors.size(); a++)
		{
			for (unsigned s = 0; s < sectors[a]->connectedSides().size(); s++)
				connected_sides.push_back(sectors[a]->connectedSides()[s]);
			sectors[a]->getLines(connected_lines);

			map.removeSector(sectors[a]);
		}

		// Backup connected line properties
		//for (unsigned a = 0; a < connected_lines.size(); a++)
		//	undo_manager->recordUndoStep(new PropertyChangeUS(connected_lines[a]));

		// Remove all connected sides
		for (unsigned a = 0; a < connected_sides.size(); a++)
		{
			// Before removing the side, check if we should flip the line
			MapLine* line = connected_sides[a]->getParentLine();
			if (connected_sides[a] == line->s1() && line->s2())
				line->flip();

			map.removeSide(connected_sides[a]);
		}

		// Editor message
		if (sectors.size() == 1)
			addEditorMessage(S_FMT("Deleted sector #%d", index));
		else if (sectors.size() > 1)
			addEditorMessage(S_FMT("Deleted %d sector", sectors.size()));

		// Remove detached vertices
		map.removeDetachedVertices();

		// Refresh map view
		theMapEditor->forceRefresh(true);
	}

	// Things mode
	else if (edit_mode == MODE_THINGS)
	{
		// Get selected things
		vector<MapThing*> things;
		getSelectedThings(things);
		int index = -1;
		if (things.size() == 1)
			index = things[0]->getIndex();

		// Begin undo step
		beginUndoRecord("Delete Things", false, false, true);

		// Delete them (if any)
		for (unsigned a = 0; a < things.size(); a++)
			map.removeThing(things[a]);

		// Editor message
		if (things.size() == 1)
			addEditorMessage(S_FMT("Deleted thing #%d", index));
		else if (things.size() > 1)
			addEditorMessage(S_FMT("Deleted %d things", things.size()));
	}

	// Record undo step
	endUndoRecord(true);

	// Clear hilight and selection
	selection.clear();
	hilight_item = -1;
}

#pragma endregion

#pragma region LINE DRAWING

fpoint2_t MapEditor::lineDrawPoint(unsigned index)
{
	// Check index
	if (index >= draw_points.size())
		return fpoint2_t(0, 0);

	return draw_points[index];
}

bool MapEditor::addLineDrawPoint(fpoint2_t point, bool nearest)
{
	// Snap to nearest vertex if necessary
	if (nearest)
	{
		int vertex = map.nearestVertex(point.x, point.y);
		if (vertex >= 0)
		{
			point.x = map.getVertex(vertex)->xPos();
			point.y = map.getVertex(vertex)->yPos();
		}
	}

	// Otherwise, snap to grid if necessary
	else if (grid_snap)
	{
		point.x = snapToGrid(point.x);
		point.y = snapToGrid(point.y);
	}

	// Check if this is the same as the last point
	if (draw_points.size() > 0 && point.x == draw_points.back().x && point.y == draw_points.back().y)
	{
		// End line drawing
		endLineDraw(true);
		theMapEditor->showShapeDrawPanel(false);
		return true;
	}

	// Add point
	draw_points.push_back(point);

	// Check if first and last points match
	if (draw_points.size() > 1 && point.x == draw_points[0].x && point.y == draw_points[0].y)
	{
		endLineDraw(true);
		theMapEditor->showShapeDrawPanel(false);
		return true;
	}

	return false;
}

void MapEditor::removeLineDrawPoint()
{
	if(draw_points.empty())
	{
		endLineDraw(false);
		theMapEditor->showShapeDrawPanel(false);
	}
	else
	{
		draw_points.pop_back();
	}
}

void MapEditor::setShapeDrawOrigin(fpoint2_t point, bool nearest)
{
	// Snap to nearest vertex if necessary
	if (nearest)
	{
		int vertex = map.nearestVertex(point.x, point.y);
		if (vertex >= 0)
		{
			point.x = map.getVertex(vertex)->xPos();
			point.y = map.getVertex(vertex)->yPos();
		}
	}

	// Otherwise, snap to grid if necessary
	else if (grid_snap)
	{
		point.x = snapToGrid(point.x);
		point.y = snapToGrid(point.y);
	}

	draw_origin = point;
}

void MapEditor::updateShapeDraw(fpoint2_t point)
{
	// Clear line draw points
	draw_points.clear();

	// Snap edge to grid if needed
	if (grid_snap)
	{
		point.x = snapToGrid(point.x);
		point.y = snapToGrid(point.y);
	}

	// Lock width:height at 1:1 if needed
	fpoint2_t origin = draw_origin;
	double width = abs(point.x - origin.x);
	double height = abs(point.y - origin.y);
	if (shapedraw_lockratio)
	{
		if (width < height)
		{
			if (origin.x < point.x)
				point.x = origin.x + height;
			else
				point.x = origin.x - height;
		}

		if (height < width)
		{
			if (origin.y < point.y)
				point.y = origin.y + width;
			else
				point.y = origin.y - width;
		}
	}

	// Center on origin if needed
	if (shapedraw_centered)
	{
		origin.x -= (point.x - origin.x);
		origin.y -= (point.y - origin.y);
	}

	// Get box from tl->br
	fpoint2_t tl(min(origin.x, point.x), min(origin.y, point.y));
	fpoint2_t br(max(origin.x, point.x), max(origin.y, point.y));
	width = br.x - tl.x;
	height = br.y - tl.y;

	// Rectangle
	if (shapedraw_shape == 0)
	{
		draw_points.push_back(fpoint2_t(tl.x, tl.y));
		draw_points.push_back(fpoint2_t(tl.x, br.y));
		draw_points.push_back(fpoint2_t(br.x, br.y));
		draw_points.push_back(fpoint2_t(br.x, tl.y));
		draw_points.push_back(fpoint2_t(tl.x, tl.y));
	}

	// Ellipse
	else if (shapedraw_shape == 1)
	{
		// Get midpoint
		fpoint2_t mid;
		mid.x = tl.x + ((br.x - tl.x) * 0.5);
		mid.y = tl.y + ((br.y - tl.y) * 0.5);

		// Get x/y radius
		width *= 0.5;
		height *= 0.5;

		// Add ellipse points
		double rot = 0;
		fpoint2_t start;
		for (int a = 0; a < shapedraw_sides; a++)
		{
			fpoint2_t p(mid.x + sin(rot) * width, mid.y - cos(rot) * height);
			draw_points.push_back(p);
			rot -= (3.1415926535897932384626433832795 * 2) / (double)shapedraw_sides;

			if (a == 0)
				start = p;
		}

		// Close ellipse
		draw_points.push_back(start);
	}
}

struct me_ls_t
{
	MapLine*	line;
	bool		front;
	bool		ignore;
	me_ls_t(MapLine* line, bool front) { this->line = line; this->front = front; ignore = false; }
};

void MapEditor::endLineDraw(bool apply)
{
	// Check if we want to 'apply' the line draw (ie. create the lines)
	if (apply && draw_points.size() > 1)
	{
		// Begin undo level
		beginUndoRecord("Line Draw");

		// Add extra points if any lines overlap existing vertices
		for (unsigned a = 0; a < draw_points.size() - 1; a++)
		{
			MapVertex* v = map.lineCrossVertex(draw_points[a].x, draw_points[a].y, draw_points[a+1].x, draw_points[a+1].y);
			while (v)
			{
				draw_points.insert(draw_points.begin()+a+1, fpoint2_t(v->xPos(), v->yPos()));
				a++;
				v = map.lineCrossVertex(draw_points[a].x, draw_points[a].y, draw_points[a+1].x, draw_points[a+1].y);
			}
		}

		// Create vertices
		for (unsigned a = 0; a < draw_points.size(); a++)
			map.createVertex(draw_points[a].x, draw_points[a].y, 1);

		// Create lines
		unsigned nl_start = map.nLines();
		for (unsigned a = 0; a < draw_points.size() - 1; a++)
		{
			// Check for intersections
			vector<fpoint2_t> intersect = map.cutLines(draw_points[a].x, draw_points[a].y, draw_points[a+1].x, draw_points[a+1].y);
			LOG_MESSAGE(2, S_FMT("%d intersect points", intersect.size()));

			// Create line normally if no intersections
			if (intersect.size() == 0)
				map.createLine(draw_points[a].x, draw_points[a].y, draw_points[a+1].x, draw_points[a+1].y, 1);
			else
			{
				// Intersections exist, create multiple lines between intersection points

				// From first point to first intersection
				map.createLine(draw_points[a].x, draw_points[a].y, intersect[0].x, intersect[0].y, 1);

				// Between intersection points
				for (unsigned p = 0; p < intersect.size() - 1; p++)
					map.createLine(intersect[p].x, intersect[p].y, intersect[p+1].x, intersect[p+1].y, 1);

				// From last intersection to next point
				map.createLine(intersect.back().x, intersect.back().y, draw_points[a+1].x, draw_points[a+1].y, 1);
			}
		}

		// Build new sectors
		vector<MapLine*> new_lines;
		for (unsigned a = nl_start; a < map.nLines(); a++)
			new_lines.push_back(map.getLine(a));
		map.correctSectors(new_lines);

		// End recording undo level
		endUndoRecord(true);
	}

	// Clear draw points
	draw_points.clear();
}

#pragma endregion

#pragma region OBJECT EDIT

bool MapEditor::beginObjectEdit()
{
	vector<MapObject*> edit_objects;

	// Things mode
	if (edit_mode == MODE_THINGS)
	{
		// Get selected things
		getSelectedObjects(edit_objects);

		// Setup object group
		edit_object_group.clear();
		for (unsigned a = 0; a < edit_objects.size(); a++)
			edit_object_group.addThing((MapThing*)edit_objects[a]);

		// Filter objects
		edit_object_group.filterObjects(true);
	}
	else
	{
		// Vertices mode
		if (edit_mode == MODE_VERTICES)
		{
			// Get selected vertices
			getSelectedObjects(edit_objects);
		}

		// Lines mode
		else if (edit_mode == MODE_LINES)
		{
			// Get vertices of selected lines
			vector<MapLine*> lines;
			getSelectedLines(lines);
			for (unsigned a = 0; a < lines.size(); a++)
			{
				VECTOR_ADD_UNIQUE(edit_objects, lines[a]->v1());
				VECTOR_ADD_UNIQUE(edit_objects, lines[a]->v2());
			}
		}

		// Sectors mode
		else if (edit_mode == MODE_SECTORS)
		{
			// Get vertices of selected sectors
			vector<MapSector*> sectors;
			getSelectedSectors(sectors);
			for (unsigned a = 0; a < sectors.size(); a++)
				sectors[a]->getVertices(edit_objects);
		}

		// Setup object group
		edit_object_group.clear();
		for (unsigned a = 0; a < edit_objects.size(); a++)
			edit_object_group.addVertex((MapVertex*)edit_objects[a]);
		edit_object_group.addConnectedLines();

		// Filter objects
		edit_object_group.filterObjects(true);
	}

	theMapEditor->showObjectEditPanel(&edit_object_group);

	return true;
}

void MapEditor::endObjectEdit(bool accept)
{
	// Un-filter objects
	edit_object_group.filterObjects(false);

	// Apply change if accepted
	if (accept)
	{
		// Begin recording undo level
		beginUndoRecord(S_FMT("Edit %s", CHR(getModeString())));

		// Apply changes
		edit_object_group.applyEdit();

		// Do merge
		bool merge = true;
		if (edit_mode != MODE_THINGS)
		{
			// Begin extra 'Merge' undo step if wanted
			if (map_merge_undo_step)
			{
				endUndoRecord(true);
				beginUndoRecord("Merge");
			}

			vector<MapVertex*> vertices;
			edit_object_group.getVertices(vertices);
			merge = map.mergeArch(vertices);
		}

		// Clear selection
		clearSelection(false);

		endUndoRecord(merge);
	}

	theMapEditor->hideObjectEditPanel();
}

#pragma endregion

#pragma region COPY / PASTE

void MapEditor::copyProperties(MapObject* object)
{
	// Do nothing if no selection or hilight
	if (selection.size() == 0 && hilight_item < 0)
		return;

	// Sectors mode
	if (edit_mode == MODE_SECTORS)
	{
		// Create copy sector if needed
		if (!copy_sector)
			copy_sector = new MapSector(NULL);

		// Copy selection/hilight properties
		if (selection.size() > 0)
			copy_sector->copy(map.getSector(selection[0]));
		else if (hilight_item >= 0)
			copy_sector->copy(map.getSector(hilight_item));

		// Editor message
		if (!object)
			addEditorMessage("Copied sector properties");
	}

	// Things mode
	else if (edit_mode == MODE_THINGS)
	{
		// Create copy thing if needed
		if (!copy_thing)
			copy_thing = new MapThing(NULL);

		// Copy given object properties (if any)
		if (object && object->getObjType() == MOBJ_THING)
			copy_thing->copy(object);
		else
		{
			// Otherwise copy selection/hilight properties
			if (selection.size() > 0)
				copy_thing->copy(map.getThing(selection[0]));
			else if (hilight_item >= 0)
				copy_thing->copy(map.getThing(hilight_item));
			else
				return;
		}

		// Editor message
		if (!object)
			addEditorMessage("Copied thing properties");
	}

	else if (edit_mode == MODE_LINES)
	{
		if (!copy_line)
			copy_line = new MapLine(NULL, NULL, new MapSide(NULL, NULL), new MapSide(NULL, NULL), NULL);

		if (selection.size() > 0)
			copy_line->copy(map.getLine(selection[0]));
		else if (hilight_item >= 0)
			copy_line->copy(map.getLine(hilight_item));

		if(!object)
			addEditorMessage("Copied line properties");
	}
}

void MapEditor::pasteProperties()
{
	// Do nothing if no selection or hilight
	if (selection.size() == 0 && hilight_item < 0)
		return;

	// Sectors mode
	if (edit_mode == MODE_SECTORS)
	{
		// Do nothing if no properties have been copied
		if (!copy_sector)
			return;

		// Paste properties to selection/hilight
		beginUndoRecord("Paste Sector Properties", true, false, false);
		if (selection.size() > 0)
		{
			for (unsigned a = 0; a < selection.size(); a++)
				map.getSector(selection[a])->copy(copy_sector);
		}
		else if (hilight_item >= 0)
			map.getSector(hilight_item)->copy(copy_sector);
		endUndoRecord();

		// Editor message
		addEditorMessage("Pasted sector properties");
	}

	// Things mode
	if (edit_mode == MODE_THINGS)
	{
		// Do nothing if no properties have been copied
		if (!copy_thing)
			return;

		// Paste properties to selection/hilight
		beginUndoRecord("Paste Thing Properties", true, false, false);
		if (selection.size() > 0)
		{
			for (unsigned a = 0; a < selection.size(); a++)
			{
				// Paste properties (but keep position)
				MapThing* thing = map.getThing(selection[a]);
				double x = thing->xPos();
				double y = thing->yPos();
				thing->copy(copy_thing);
				thing->setFloatProperty("x", x);
				thing->setFloatProperty("y", y);
			}
		}
		else if (hilight_item >= 0)
		{
			// Paste properties (but keep position)
			MapThing* thing = map.getThing(hilight_item);
			double x = thing->xPos();
			double y = thing->yPos();
			thing->copy(copy_thing);
			thing->setFloatProperty("x", x);
			thing->setFloatProperty("y", y);
		}
		endUndoRecord();

		// Editor message
		addEditorMessage("Pasted thing properties");
	}

	// Lines mode
	else if (edit_mode == MODE_LINES)
	{
		// Do nothing if no properties have been copied
		if (!copy_line)
			return;

		// Paste properties to selection/hilight
		beginUndoRecord("Paste Line Properties", true, false, false);
		if (selection.size() > 0)
		{
			for (unsigned a = 0; a < selection.size(); a++)
				map.getLine(selection[a])->copy(copy_line);
		}
		else if (hilight_item >= 0)
			map.getLine(hilight_item)->copy(copy_line);
		endUndoRecord();

		// Editor message
		addEditorMessage("Pasted line properties");
	}

	// Update display
	updateDisplay();
}

void MapEditor::copy()
{
	// Can't copy/paste vertices (no point)
	if (edit_mode == MODE_VERTICES)
	{
		//addEditorMessage("Copy/Paste not supported for vertices");
		return;
	}

	// Clear current clipboard contents
	theClipboard->clear();

	// Copy lines
	if (edit_mode == MODE_LINES || edit_mode == MODE_SECTORS)
	{
		// Get selected lines
		vector<MapLine*> lines;
		getSelectedLines(lines);

		// Add to clipboard
		MapArchClipboardItem* c = new MapArchClipboardItem();
		c->addLines(lines);
		theClipboard->addItem(c);

		// Editor message
		addEditorMessage(S_FMT("Copied %s", CHR(c->getInfo())));
	}

	// Copy things
	else if (edit_mode == MODE_THINGS)
	{
		// Get selected things
		vector<MapThing*> things;
		getSelectedThings(things);

		// Add to clipboard
		MapThingsClipboardItem* c = new MapThingsClipboardItem();
		c->addThings(things);
		theClipboard->addItem(c);

		// Editor message
		addEditorMessage(S_FMT("Copied %s", CHR(c->getInfo())));
	}
}

void MapEditor::paste(fpoint2_t mouse_pos)
{
	// Go through clipboard items
	for (unsigned a = 0; a < theClipboard->nItems(); a++)
	{
		// Map architecture
		if (theClipboard->getItem(a)->getType() == CLIPBOARD_MAP_ARCH)
		{
			beginUndoRecord("Paste Map Architecture");
			long move_time = theApp->runTimer();
			MapArchClipboardItem* p = (MapArchClipboardItem*)theClipboard->getItem(a);
			vector<MapVertex*> new_verts = p->pasteToMap(&map, mouse_pos);
			map.mergeArch(new_verts);
			addEditorMessage(S_FMT("Pasted %s", CHR(p->getInfo())));
			endUndoRecord(true);
		}

		// Things
		else if (theClipboard->getItem(a)->getType() == CLIPBOARD_MAP_THINGS)
		{
			beginUndoRecord("Paste Things", false, true, false);
			MapThingsClipboardItem* p = (MapThingsClipboardItem*)theClipboard->getItem(a);
			p->pasteToMap(&map, mouse_pos);
			addEditorMessage(S_FMT("Pasted %s", CHR(p->getInfo())));
			endUndoRecord(true);
		}
	}
}

#pragma endregion

#pragma endregion

#pragma region EDITING_3D

bool MapEditor::set3dHilight(selection_3d_t item)
{
	bool changed = false;
	if (item.index != hilight_3d.index || item.type != hilight_3d.type)
	{
		last_undo_level = "";
		changed = true;
	}

	hilight_3d = item;

	return changed;
}

bool MapEditor::wallMatches(MapSide* side, uint8_t part, string tex)
{
	// Check for blank texture where it isn't needed
	if (tex == "-")
	{
		MapLine* line = side->getParentLine();
		int needed = line->needsTexture();
		if (side == line->s1())
		{
			if (part == SEL_SIDE_TOP && (needed & TEX_FRONT_UPPER) == 0)
				return false;
			if (part == SEL_SIDE_MIDDLE && (needed & TEX_FRONT_MIDDLE) == 0)
				return false;
			if (part == SEL_SIDE_BOTTOM && (needed & TEX_FRONT_LOWER) == 0)
				return false;
		}
		else if (side == line->s2())
		{
			if (part == SEL_SIDE_TOP && (needed & TEX_BACK_UPPER) == 0)
				return false;
			if (part == SEL_SIDE_MIDDLE && (needed & TEX_BACK_MIDDLE) == 0)
				return false;
			if (part == SEL_SIDE_BOTTOM && (needed & TEX_BACK_LOWER) == 0)
				return false;
		}
	}

	// Check texture
	if (part == SEL_SIDE_TOP && side->stringProperty("texturetop") != tex)
		return false;
	if (part == SEL_SIDE_MIDDLE && side->stringProperty("texturemiddle") != tex)
		return false;
	if (part == SEL_SIDE_BOTTOM && side->stringProperty("texturebottom") != tex)
		return false;

	return true;
}

void MapEditor::getAdjacentWalls3d(selection_3d_t item, vector<selection_3d_t>& list)
{
	// Add item to list if needed
	for (unsigned a = 0; a < list.size(); a++)
	{
		if (list[a].type == item.type && list[a].index == item.index)
			return;
	}
	list.push_back(item);

	// Get initial side
	MapSide* side = map.getSide(item.index);
	if (!side)
		return;

	// Get initial line
	MapLine* line = side->getParentLine();
	if (!line)
		return;

	// Get texture to match
	string tex;
	if (item.type == SEL_SIDE_BOTTOM)
		tex = side->stringProperty("texturebottom");
	else if (item.type == SEL_SIDE_MIDDLE)
		tex = side->stringProperty("texturemiddle");
	else
		tex = side->stringProperty("texturetop");

	// Go through attached lines (vertex 1)
	for (unsigned a = 0; a < line->v1()->nConnectedLines(); a++)
	{
		MapLine* oline = line->v1()->connectedLine(a);
		if (!oline || oline == line)
			continue;

		// Get line sides
		MapSide* side1 = oline->s1();
		MapSide* side2 = oline->s2();

		// Front side
		if (side1)
		{
			// Upper texture
			if (wallMatches(side1, SEL_SIDE_TOP, tex))
				getAdjacentWalls3d(selection_3d_t(side1->getIndex(), SEL_SIDE_TOP), list);

			// Middle texture
			if (wallMatches(side1, SEL_SIDE_MIDDLE, tex))
				getAdjacentWalls3d(selection_3d_t(side1->getIndex(), SEL_SIDE_MIDDLE), list);

			// Lower texture
			if (wallMatches(side1, SEL_SIDE_BOTTOM, tex))
				getAdjacentWalls3d(selection_3d_t(side1->getIndex(), SEL_SIDE_BOTTOM), list);
		}

		// Back side
		if (side2)
		{
			// Upper texture
			if (wallMatches(side2, SEL_SIDE_TOP, tex))
				getAdjacentWalls3d(selection_3d_t(side2->getIndex(), SEL_SIDE_TOP), list);

			// Middle texture
			if (wallMatches(side2, SEL_SIDE_MIDDLE, tex))
				getAdjacentWalls3d(selection_3d_t(side2->getIndex(), SEL_SIDE_MIDDLE), list);

			// Lower texture
			if (wallMatches(side2, SEL_SIDE_BOTTOM, tex))
				getAdjacentWalls3d(selection_3d_t(side2->getIndex(), SEL_SIDE_BOTTOM), list);
		}
	}

	// Go through attached lines (vertex 2)
	for (unsigned a = 0; a < line->v2()->nConnectedLines(); a++)
	{
		MapLine* oline = line->v2()->connectedLine(a);
		if (!oline || oline == line)
			continue;

		// Get line sides
		MapSide* side1 = oline->s1();
		MapSide* side2 = oline->s2();

		// Front side
		if (side1)
		{
			// Upper texture
			if (wallMatches(side1, SEL_SIDE_TOP, tex))
				getAdjacentWalls3d(selection_3d_t(side1->getIndex(), SEL_SIDE_TOP), list);

			// Middle texture
			if (wallMatches(side1, SEL_SIDE_MIDDLE, tex))
				getAdjacentWalls3d(selection_3d_t(side1->getIndex(), SEL_SIDE_MIDDLE), list);

			// Lower texture
			if (wallMatches(side1, SEL_SIDE_BOTTOM, tex))
				getAdjacentWalls3d(selection_3d_t(side1->getIndex(), SEL_SIDE_BOTTOM), list);
		}

		// Back side
		if (side2)
		{
			// Upper texture
			if (wallMatches(side2, SEL_SIDE_TOP, tex))
				getAdjacentWalls3d(selection_3d_t(side2->getIndex(), SEL_SIDE_TOP), list);

			// Middle texture
			if (wallMatches(side2, SEL_SIDE_MIDDLE, tex))
				getAdjacentWalls3d(selection_3d_t(side2->getIndex(), SEL_SIDE_MIDDLE), list);

			// Lower texture
			if (wallMatches(side2, SEL_SIDE_BOTTOM, tex))
				getAdjacentWalls3d(selection_3d_t(side2->getIndex(), SEL_SIDE_BOTTOM), list);
		}
	}
}

void MapEditor::selectAdjacent3d(selection_3d_t item)
{
	// Check item
	if (item.index < 0)
		return;

	// Select item
	selectItem3d(item, SELECT);

	// Flat
	if (item.type == SEL_FLOOR || item.type == SEL_CEILING)
	{
		// Get initial sector
		MapSector* sector = map.getSector(item.index);
		if (!sector) return;

		// Go through sector lines
		vector<MapLine*> lines;
		sector->getLines(lines);
		for (unsigned a = 0; a < lines.size(); a++)
		{
			// Get sector on opposite side
			MapSector* osector = NULL;
			if (lines[a]->frontSector() == sector)
				osector = lines[a]->backSector();
			else
				osector = lines[a]->frontSector();

			// Skip if no sector
			if (!osector || osector == sector)
				continue;

			// Check for match
			if (item.type == SEL_FLOOR)
			{
				// Check sector floor height
				if (osector->intProperty("heightfloor") != sector->intProperty("heightfloor"))
					continue;

				// Check sector floor texture
				if (osector->getFloorTex() != sector->getFloorTex())
					continue;
			}
			else
			{
				// Check sector ceiling height
				if (osector->intProperty("heightceiling") != sector->intProperty("heightceiling"))
					continue;

				// Check sector ceiling texture
				if (osector->getCeilingTex() != sector->getCeilingTex())
					continue;
			}

			// Check flat isn't already selected
			bool selected = false;
			for (unsigned s = 0; s < selection_3d.size(); s++)
			{
				if (selection_3d[s].type == item.type && selection_3d[s].index == osector->getIndex())
				{
					selected = true;
					break;
				}
			}

			// Recursively select adjacent flats
			if (!selected)
				selectAdjacent3d(selection_3d_t(osector->getIndex(), item.type));
		}
	}

	// Wall
	else if (item.type != SEL_THING)
	{
		vector<selection_3d_t> list;
		getAdjacentWalls3d(item, list);
		for (unsigned a = 0; a < list.size(); a++)
			selectItem3d(list[a], SELECT);
	}
}

void MapEditor::changeSectorLight3d(int amount)
{
	// Get items to process
	vector<selection_3d_t> items;
	if (selection_3d.empty() && hilight_3d.index >= 0 && hilight_3d.type != SEL_THING)
		items.push_back(hilight_3d);
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != SEL_THING)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	beginUndoRecordLocked("Change Sector Light", true, false, false);

	// Go through items
	vector<MapSector*> processed_sectors;
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall
		if (items[a].type == SEL_SIDE_BOTTOM || items[a].type == SEL_SIDE_MIDDLE || items[a].type == SEL_SIDE_TOP)
		{
			// Get side
			MapSide* side = map.getSide(items[a].index);
			if (!side) continue;
			MapSector* sector = side->getSector();
			if (!sector) continue;

			// Ignore if sector already processed
			if (VECTOR_EXISTS(processed_sectors, sector))
				continue;
			else
				processed_sectors.push_back(sector);

			// Check for decrease when light = 255
			if (sector->getLight(0) == 255 && amount < -1)
				amount++;

			// Change sector light level
			sector->changeLight(amount);
		}

		// Flat
		if (items[a].type == SEL_FLOOR || items[a].type == SEL_CEILING)
		{
			// Get sector
			MapSector* s = map.getSector(items[a].index);

			// Change light level
			if (items[a].type == SEL_FLOOR && !link_3d_light)
				s->changeLight(amount, 1);
			else if (items[a].type == SEL_CEILING && !link_3d_light)
				s->changeLight(amount, 2);
			else
			{
				// Ignore if sector already processed
				if (VECTOR_EXISTS(processed_sectors, s))
					continue;
				else
					processed_sectors.push_back(s);

				// Check for decrease when light = 255
				if (s->getLight(0) == 255 && amount < -1)
					amount++;

				s->changeLight(amount, 0);
			}
		}
	}

	// End undo level
	endUndoRecord();

	// Editor message
	if (items.size() > 0)
	{
		if (amount > 0)
			addEditorMessage(S_FMT("Light increased by %d", amount));
		else
			addEditorMessage(S_FMT("Light decreased by %d", -amount));
	}
}

void MapEditor::changeOffset3d(int amount, bool x)
{
	// Get items to process
	vector<selection_3d_t> items;
	if (selection_3d.empty())
	{
		if (hilight_3d.index >= 0 && hilight_3d.type != SEL_THING)
			items.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != SEL_THING)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	beginUndoRecordLocked("Change Offset", true, false, false);

	// Go through items
	vector<int> done;
	bool changed = false;
	bool udmf_ext = (map.currentFormat() == MAP_UDMF && theGameConfiguration->udmfNamespace() == "zdoom");
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall
		if (items[a].type >= SEL_SIDE_TOP && items[a].type <= SEL_SIDE_BOTTOM)
		{
			MapSide* side = map.getSide(items[a].index);

			// If offsets are linked, just change the whole side offset
			if (link_3d_offset)
			{
				// Check we haven't processed this side already
				if (VECTOR_EXISTS(done, items[a].index))
					continue;

				// Change the appropriate offset
				if (x)
				{
					int offset = side->intProperty("offsetx");
					side->setIntProperty("offsetx", offset + amount);
				}
				else
				{
					int offset = side->intProperty("offsety");
					side->setIntProperty("offsety", offset + amount);
				}

				// Add to done list
				done.push_back(items[a].index);
			}

			// Unlinked offsets
			else
			{
				// Build property string (offset[x/y]_[top/mid/bottom])
				string ofs = "offsetx";
				if (!x) ofs = "offsety";
				if (items[a].type == SEL_SIDE_BOTTOM)
					ofs += "_bottom";
				else if (items[a].type == SEL_SIDE_TOP)
					ofs += "_top";
				else
					ofs += "_mid";

				// Change the offset
				int offset = side->floatProperty(ofs);
				side->setFloatProperty(ofs, offset + amount);
			}

			changed = true;
		}

		// Flat (UDMF+ZDoom only)
		else if (udmf_ext)
		{
			MapSector* sector = map.getSector(items[a].index);

			if (items[a].type == SEL_FLOOR)
			{
				if (x)
				{
					double offset = sector->floatProperty("xpanningfloor");
					sector->setFloatProperty("xpanningfloor", offset + amount);
				}
				else
				{
					double offset = sector->floatProperty("ypanningfloor");
					sector->setFloatProperty("ypanningfloor", offset + amount);
				}

				changed = true;
			}
			else if (items[a].type == SEL_CEILING)
			{
				if (x)
				{
					double offset = sector->floatProperty("xpanningceiling");
					sector->setFloatProperty("xpanningceiling", offset + amount);
				}
				else
				{
					double offset = sector->floatProperty("ypanningceiling");
					sector->setFloatProperty("ypanningceiling", offset + amount);
				}

				changed = true;
			}
		}
	}

	// End undo level
	endUndoRecord(changed);

	// Editor message
	if (items.size() > 0 && changed)
	{
		string axis = "X";
		if (!x) axis = "Y";

		if (amount > 0)
			addEditorMessage(S_FMT("%s offset increased by %d", CHR(axis), amount));
		else
			addEditorMessage(S_FMT("%s offset decreased by %d", CHR(axis), -amount));
	}
}

void MapEditor::changeSectorHeight3d(int amount)
{
	// Get items to process
	vector<selection_3d_t> items;
	if (selection_3d.empty() && hilight_3d.type != SEL_THING && hilight_3d.index >= 0)
		items.push_back(hilight_3d);
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != SEL_THING)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	beginUndoRecordLocked("Change Sector Height", true, false, false);

	// Go through items
	vector<int> ceilings;
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall (ceiling only for now)
		if (items[a].type == SEL_SIDE_BOTTOM || items[a].type == SEL_SIDE_MIDDLE || items[a].type == SEL_SIDE_TOP)
		{
			// Get sector
			MapSector* sector = map.getSide(items[a].index)->getSector();

			// Check this sector's ceiling hasn't already been changed
			int index = sector->getIndex();
			if (VECTOR_EXISTS(ceilings, index))
				continue;

			// Change height
			int height = sector->intProperty("heightceiling");
			sector->setIntProperty("heightceiling", height + amount);

			// Set to changed
			ceilings.push_back(index);
		}

		// Floor
		else if (items[a].type == SEL_FLOOR)
		{
			// Get sector
			MapSector* sector = map.getSector(items[a].index);

			// Change height
			int height = sector->intProperty("heightfloor");
			sector->setIntProperty("heightfloor", height + amount);
		}

		// Ceiling
		else if (items[a].type == SEL_CEILING)
		{
			// Get sector
			MapSector* sector = map.getSector(items[a].index);

			// Check this sector's ceiling hasn't already been changed
			bool done = false;
			int index = sector->getIndex();
			for (unsigned b = 0; b < ceilings.size(); b++)
			{
				if (ceilings[b] == index)
				{
					done = true;
					break;
				}
			}
			if (done)
				continue;

			// Change height
			int height = sector->intProperty("heightceiling");
			sector->setIntProperty("heightceiling", height + amount);

			// Set to changed
			ceilings.push_back(sector->getIndex());
		}
	}

	// End undo level
	endUndoRecord();

	// Editor message
	if (items.size() > 0)
	{
		if (amount > 0)
			addEditorMessage(S_FMT("Height increased by %d", amount));
		else
			addEditorMessage(S_FMT("Height decreased by %d", -amount));
	}
}

void MapEditor::doAlignX3d(MapSide* side, int offset, string tex, vector<selection_3d_t>& walls_done)
{
	// Check if this wall has already been processed
	for (unsigned a = 0; a < walls_done.size(); a++)
	{
		if (walls_done[a].index == side->getIndex())
			return;
	}

	// Add to 'done' list
	walls_done.push_back(selection_3d_t(side->getIndex(), SEL_SIDE_MIDDLE));

	// Set offset
	side->setIntProperty("offsetx", offset);

	// Get 'next' vertex
	MapLine* line = side->getParentLine();
	MapVertex* vertex = line->v2();
	if (side == line->s2())
		vertex = line->v1();

	// Get integral length of line
	int intlen = MathStuff::round(line->getLength());

	// Go through connected lines
	for (unsigned a = 0; a < vertex->nConnectedLines(); a++)
	{
		MapLine* l = vertex->connectedLine(a);

		// First side
		MapSide* s = l->s1();
		if (s)
		{
			// Check for matching texture
			if (s->stringProperty("texturetop") == tex || s->stringProperty("texturemiddle") == tex || s->stringProperty("texturebottom") == tex)
				doAlignX3d(s, offset + intlen, tex, walls_done);
		}

		// Second side
		s = l->s2();
		if (s)
		{
			// Check for matching texture
			if (s->stringProperty("texturetop") == tex || s->stringProperty("texturemiddle") == tex || s->stringProperty("texturebottom") == tex)
				doAlignX3d(s, offset + intlen, tex, walls_done);
		}
	}
}

void MapEditor::autoAlignX3d(selection_3d_t start)
{
	// Check start is a wall
	if (start.type != SEL_SIDE_BOTTOM && start.type != SEL_SIDE_MIDDLE && start.type != SEL_SIDE_TOP)
		return;

	// Get starting side
	MapSide* side = map.getSide(start.index);
	if (!side) return;

	// Get texture to match
	string tex;
	if (start.type == SEL_SIDE_BOTTOM)
		tex = side->stringProperty("texturebottom");
	else if (start.type == SEL_SIDE_MIDDLE)
		tex = side->stringProperty("texturemiddle");
	else if (start.type == SEL_SIDE_TOP)
		tex = side->stringProperty("texturetop");

	// Init aligned wall list
	vector<selection_3d_t> walls_done;

	// Begin undo level
	beginUndoRecord("Auto Align X", true, false, false);

	// Do alignment
	doAlignX3d(side, side->intProperty("offsetx"), tex, walls_done);

	// End undo level
	endUndoRecord();

	// Editor message
	addEditorMessage("Auto-aligned on X axis");
}

void MapEditor::resetWall3d()
{
	// Get items to process
	vector<selection_3d_t> items;
	if (selection_3d.size() == 0)
	{
		if (hilight_3d.type == SEL_SIDE_TOP || hilight_3d.type == SEL_SIDE_BOTTOM || hilight_3d.type == SEL_SIDE_MIDDLE)
			items.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type == SEL_SIDE_TOP || selection_3d[a].type == SEL_SIDE_BOTTOM || selection_3d[a].type == SEL_SIDE_MIDDLE)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.size() == 0)
		return;

	// Begin undo level
	beginUndoRecord("Reset Wall", true, false, false);

	// Go through items
	for (unsigned a = 0; a < items.size(); a++)
	{
		MapSide* side = map.getSide(items[a].index);
		if (!side) continue;

		// Reset offsets
		if (link_3d_offset)
		{
			// If offsets are linked, reset base offsets
			side->setIntProperty("offsetx", 0);
			side->setIntProperty("offsety", 0);
		}
		else
		{
			// Otherwise, reset offsets for the current wall part
			if (items[a].type == SEL_SIDE_TOP)
			{
				side->setFloatProperty("offsetx_top", 0);
				side->setFloatProperty("offsety_top", 0);
			}
			else if (items[a].type == SEL_SIDE_MIDDLE)
			{
				side->setFloatProperty("offsetx_mid", 0);
				side->setFloatProperty("offsety_mid", 0);
			}
			else
			{
				side->setFloatProperty("offsetx_bottom", 0);
				side->setFloatProperty("offsety_bottom", 0);
			}
		}

		// Reset scaling
		if (theGameConfiguration->udmfNamespace() == "zdoom")
		{
			if (items[a].type == SEL_SIDE_TOP)
			{
				side->setFloatProperty("scalex_top", 1);
				side->setFloatProperty("scaley_top", 1);
			}
			else if (items[a].type == SEL_SIDE_MIDDLE)
			{
				side->setFloatProperty("scalex_mid", 1);
				side->setFloatProperty("scaley_mid", 1);
			}
			else
			{
				side->setFloatProperty("scalex_bottom", 1);
				side->setFloatProperty("scaley_bottom", 1);
			}
		}
	}

	// End undo level
	endUndoRecord();

	// Editor message
	if (theGameConfiguration->udmfNamespace() == "zdoom")
		addEditorMessage("Offsets and scaling reset");
	else
		addEditorMessage("Offsets reset");
}

void MapEditor::toggleUnpegged3d(bool lower)
{
	// Get items to process
	vector<selection_3d_t> items;
	if (selection_3d.size() == 0)
	{
		if (hilight_3d.type == SEL_SIDE_TOP || hilight_3d.type == SEL_SIDE_BOTTOM || hilight_3d.type == SEL_SIDE_MIDDLE)
			items.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type == SEL_SIDE_TOP || selection_3d[a].type == SEL_SIDE_BOTTOM || selection_3d[a].type == SEL_SIDE_MIDDLE)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.size() == 0)
		return;

	// Begin undo level
	string undo_type = lower ? "Toggle Lower Unpegged" : "Toggle Upper Unpegged";
	undo_manager_3d->beginRecord(undo_type);

	// Go through items
	vector<MapLine*> processed_lines;
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Get line
		MapLine* line = map.getSide(items[a].index)->getParentLine();
		if (!line) continue;

		// Skip if line already processed
		if (VECTOR_EXISTS(processed_lines, line))
			continue;
		else
			processed_lines.push_back(line);

		// Toggle flag
		recordPropertyChangeUndoStep(line);
		if (lower)
		{
			bool unpegged = theGameConfiguration->lineBasicFlagSet("dontpegbottom", line, theMapEditor->currentMapDesc().format);
			theGameConfiguration->setLineBasicFlag("dontpegbottom", line, map.currentFormat(), !unpegged);
		}
		else
		{
			bool unpegged = theGameConfiguration->lineBasicFlagSet("dontpegtop", line, theMapEditor->currentMapDesc().format);
			theGameConfiguration->setLineBasicFlag("dontpegtop", line, map.currentFormat(), !unpegged);
		}
	}

	// End undo level
	undo_manager_3d->endRecord(true);

	// Editor message
	if (lower)
		addEditorMessage("Lower Unpegged flag toggled");
	else
		addEditorMessage("Upper Unpegged flag toggled");
}

void MapEditor::copy3d(int type)
{
	// Check hilight
	if (hilight_3d.index < 0)
		return;

	// Upper wall
	else if (hilight_3d.type == SEL_SIDE_TOP)
	{
		// Texture
		if (type == COPY_TEXTYPE)
			copy_texture = map.getSide(hilight_3d.index)->stringProperty("texturetop");
	}

	// Middle wall
	else if (hilight_3d.type == SEL_SIDE_MIDDLE)
	{
		// Texture
		if (type == COPY_TEXTYPE)
			copy_texture = map.getSide(hilight_3d.index)->stringProperty("texturemiddle");
	}

	// Lower wall
	else if (hilight_3d.type == SEL_SIDE_BOTTOM)
	{
		// Texture
		if (type == COPY_TEXTYPE)
			copy_texture = map.getSide(hilight_3d.index)->stringProperty("texturebottom");
	}

	// Floor
	else if (hilight_3d.type == SEL_FLOOR)
	{
		// Texture
		if (type == COPY_TEXTYPE)
			copy_texture = map.getSector(hilight_3d.index)->getFloorTex();
	}

	// Ceiling
	else if (hilight_3d.type == SEL_CEILING)
	{
		// Texture
		if (type == COPY_TEXTYPE)
			copy_texture = map.getSector(hilight_3d.index)->getCeilingTex();
	}

	// Thing
	else if (hilight_3d.type == SEL_THING)
	{
		if (!copy_thing)
			copy_thing = new MapThing();

		copy_thing->copy(map.getThing(hilight_3d.index));
	}

	// Flash
	if (canvas)
		canvas->itemSelected3d(hilight_3d);

	// Editor message
	if (type == COPY_TEXTYPE)
	{
		if (hilight_3d.type == SEL_THING)
			addEditorMessage("Copied Thing Type");
		else
			addEditorMessage("Copied Texture");
	}
}

void MapEditor::paste3d(int type)
{
	// Get items to paste to
	vector<selection_3d_t> items;
	if (selection_3d.size() == 0 && hilight_3d.index >= 0)
		items.push_back(hilight_3d);
	else if (selection_3d.size() > 0)
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
			items.push_back(selection_3d[a]);
	}
	else
		return;

	// Begin undo step
	string ptype = "Paste Properties";
	if (type == COPY_TEXTYPE)
		ptype = "Paste Texture/Type";
	undo_manager_3d->beginRecord(ptype);

	// Go through items
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall
		if (items[a].type == SEL_SIDE_TOP || items[a].type == SEL_SIDE_MIDDLE || items[a].type == SEL_SIDE_BOTTOM)
		{
			MapSide* side = map.getSide(items[a].index);
			recordPropertyChangeUndoStep(side);

			// Upper wall
			if (items[a].type == SEL_SIDE_TOP)
			{
				// Texture
				if (type == COPY_TEXTYPE)
					side->setStringProperty("texturetop", copy_texture);
			}

			// Middle wall
			else if (items[a].type == SEL_SIDE_MIDDLE)
			{
				// Texture
				if (type == COPY_TEXTYPE)
					side->setStringProperty("texturemiddle", copy_texture);
			}

			// Lower wall
			else if (items[a].type == SEL_SIDE_BOTTOM)
			{
				// Texture
				if (type == COPY_TEXTYPE)
					side->setStringProperty("texturebottom", copy_texture);
			}
		}

		// Flat
		else if (items[a].type == SEL_FLOOR || items[a].type == SEL_CEILING)
		{
			MapSector* sector = map.getSector(items[a].index);
			recordPropertyChangeUndoStep(sector);

			// Floor
			if (items[a].type == SEL_FLOOR)
			{
				// Texture
				if (type == COPY_TEXTYPE)
					sector->setStringProperty("texturefloor", copy_texture);
			}

			// Ceiling
			if (items[a].type == SEL_CEILING)
			{
				// Texture
				if (type == COPY_TEXTYPE)
					sector->setStringProperty("textureceiling", copy_texture);
			}
		}

		// Thing
		else if (items[a].type == SEL_THING)
		{
			MapThing* thing = map.getThing(items[a].index);
			recordPropertyChangeUndoStep(thing);

			// Type
			if (type == COPY_TEXTYPE && copy_thing)
				thing->setIntProperty("type", copy_thing->getType());
		}
	}

	// Editor message
	if (type == COPY_TEXTYPE)
	{
		if (hilight_3d.type == SEL_THING)
			addEditorMessage("Pasted Thing Type");
		else
			addEditorMessage("Pasted Texture");
	}

	undo_manager_3d->endRecord(true);
}

void MapEditor::changeThingZ3d(int amount)
{
	// Ignore for doom format
	if (map.currentFormat() == MAP_DOOM)
		return;

	// Go through 3d selection
	for (unsigned a = 0; a < selection_3d.size(); a++)
	{
		// Check if thing
		if (selection_3d[a].type == SEL_THING)
		{
			MapThing* thing = map.getThing(selection_3d[a].index);
			if (thing)
			{
				// Change z height
				recordPropertyChangeUndoStep(thing);
				double z = thing->intProperty("height");
				z += amount;
				thing->setIntProperty("height", z);
			}
		}
	}
}

void MapEditor::deleteThing3d()
{
	// Begin undo level
	beginUndoRecord("Delete Thing", false, false, true);

	// Go through 3d selection
	for (unsigned a = 0; a < selection_3d.size(); a++)
	{
		// Check if thing
		if (selection_3d[a].type == SEL_THING)
			map.removeThing(selection_3d[a].index);
	}

	endUndoRecord();
}

void MapEditor::changeScale3d(double amount, bool x)
{
	// Get items to process
	vector<selection_3d_t> items;
	if (selection_3d.empty())
	{
		if (hilight_3d.index >= 0 && hilight_3d.type != SEL_THING)
			items.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != SEL_THING)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	beginUndoRecordLocked("Change Scale", true, false, false);

	// Go through selection
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall
		if (items[a].type >= SEL_SIDE_TOP && items[a].type <= SEL_SIDE_BOTTOM)
		{
			MapSide* side = map.getSide(items[a].index);

			// Build property string (offset[x/y]_[top/mid/bottom])
			string ofs = "scalex";
			if (!x) ofs = "scaley";
			if (items[a].type == SEL_SIDE_BOTTOM)
				ofs += "_bottom";
			else if (items[a].type == SEL_SIDE_TOP)
				ofs += "_top";
			else
				ofs += "_mid";

			// Change the offset
			double scale = side->floatProperty(ofs);
			if (scale + amount > 0)
				side->setFloatProperty(ofs, scale + amount);
		}

		// Flat (UDMF+ZDoom only)
		else
		{
			MapSector* sector = map.getSector(items[a].index);

			// Build property string
			string prop = x ? "xpanning" : "ypanning";
			prop += (items[a].type == SEL_FLOOR) ? "floor" : "ceiling";

			// Set
			double scale = sector->floatProperty(prop);
			if (scale + amount > 0)
				sector->setFloatProperty(prop, scale + amount);
		}
	}

	// End undo record
	endUndoRecord(true);

	// Editor message
}

#pragma endregion

#pragma region EDITOR MESSAGES

string MapEditor::getEditorMessage(int index)
{
	// Check index
	if (index < 0 || index >= (int)editor_messages.size())
		return "";

	return editor_messages[index].message;
}

long MapEditor::getEditorMessageTime(int index)
{
	// Check index
	if (index < 0 || index >= (int)editor_messages.size())
		return -1;

	return theApp->runTimer() - editor_messages[index].act_time;
}

void MapEditor::addEditorMessage(string message)
{
	// Remove oldest message if there are too many active
	if (editor_messages.size() >= 4)
		editor_messages.erase(editor_messages.begin());

	// Add message to list
	editor_msg_t msg;
	msg.message = message;
	msg.act_time = theApp->runTimer();
	editor_messages.push_back(msg);
}

unsigned MapEditor::numEditorMessages()
{
	return editor_messages.size();
}

#pragma endregion

bool MapEditor::handleKeyBind(string key, fpoint2_t position)
{
	// --- General keybinds ---

	bool handled = true;
	if (edit_mode != MODE_3D)
	{
		// Increment grid
		if (key == "me2d_grid_inc")
			incrementGrid();

		// Decrement grid
		else if (key == "me2d_grid_dec")
			decrementGrid();

		// Select all
		else if (key == "select_all")
			selectAll();

		// Clear selection
		else if (key == "me2d_clear_selection")
		{
			clearSelection();
			addEditorMessage("Selection cleared");
		}

		// Lock/unlock hilight
		else if (key == "me2d_lock_hilight")
		{
			// Toggle lock
			hilight_locked = !hilight_locked;

			// Add editor message
			if (hilight_locked)
				addEditorMessage("Locked current hilight");
			else
				addEditorMessage("Unlocked hilight");
		}

		// Copy
		else if (key == "copy")
			copy();

		else
			handled = false;
	}

	// --- Sector mode keybinds ---
	if (key.StartsWith("me2d_sector") && edit_mode == MODE_SECTORS)
	{
		// Height changes
		if		(key == "me2d_sector_floor_up8")	changeSectorHeight(8, true, false);
		else if (key == "me2d_sector_floor_up")		changeSectorHeight(1, true, false);
		else if (key == "me2d_sector_floor_down8")	changeSectorHeight(-8, true, false);
		else if (key == "me2d_sector_floor_down")	changeSectorHeight(-1, true, false);
		else if (key == "me2d_sector_ceil_up8")		changeSectorHeight(8, false, true);
		else if (key == "me2d_sector_ceil_up")		changeSectorHeight(1, false, true);
		else if (key == "me2d_sector_ceil_down8")	changeSectorHeight(-8, false, true);
		else if (key == "me2d_sector_ceil_down")	changeSectorHeight(-1, false, true);
		else if (key == "me2d_sector_height_up8")	changeSectorHeight(8, true, true);
		else if (key == "me2d_sector_height_up")	changeSectorHeight(1, true, true);
		else if (key == "me2d_sector_height_down8")	changeSectorHeight(-8, true, true);
		else if (key == "me2d_sector_height_down")	changeSectorHeight(-1, true, true);

		// Light changes
		else if (key == "me2d_sector_light_up16")	changeSectorLight(true, false);
		else if (key == "me2d_sector_light_up")		changeSectorLight(true, true);
		else if (key == "me2d_sector_light_down16")	changeSectorLight(false, false);
		else if (key == "me2d_sector_light_down")	changeSectorLight(false, true);

		// Join
		else if (key == "me2d_sector_join")			joinSectors(true);
		else if (key == "me2d_sector_join_keep")	joinSectors(false);

		else
			return false;
	}

	// --- 3d mode keybinds ---
	else if (key.StartsWith("me3d_") && edit_mode == MODE_3D)
	{
		// Check for extended udmf properties
		bool ext = false;
		if (theMapEditor->currentMapDesc().format == MAP_UDMF &&
				S_CMPNOCASE(theGameConfiguration->udmfNamespace(), "zdoom"))
			ext = true;

		// Clear selection
		if (key == "me3d_clear_selection")
		{
			clearSelection();
			addEditorMessage("Selection cleared");
		}

		// Toggle linked light levels
		else if (key == "me3d_light_toggle_link")
		{
			if (!ext)
				addEditorMessage("Unlinked light levels not supported in this game configuration");
			else
			{
				link_3d_light = !link_3d_light;
				if (link_3d_light)
					addEditorMessage("Flat light levels linked");
				else
					addEditorMessage("Flat light levels unlinked");
			}
		}

		// Toggle linked offsets
		else if (key == "me3d_wall_toggle_link_ofs")
		{
			if (!ext)
				addEditorMessage("Unlinked wall offsets not supported in this game configuration");
			else
			{
				link_3d_offset = !link_3d_offset;
				if (link_3d_offset)
					addEditorMessage("Wall offsets linked");
				else
					addEditorMessage("Wall offsets unlinked");
			}
		}

		// Copy/paste
		else if (key == "me3d_copy_tex_type")	copy3d(COPY_TEXTYPE);
		else if (key == "me3d_paste_tex_type")	paste3d(COPY_TEXTYPE);

		// Light changes
		else if	(key == "me3d_light_up16")		changeSectorLight3d(16);
		else if (key == "me3d_light_up")		changeSectorLight3d(1);
		else if (key == "me3d_light_down16")	changeSectorLight3d(-16);
		else if (key == "me3d_light_down")		changeSectorLight3d(-1);

		// Wall/Flat offset changes
		else if	(key == "me3d_xoff_up8")	changeOffset3d(8, true);
		else if	(key == "me3d_xoff_up")		changeOffset3d(1, true);
		else if	(key == "me3d_xoff_down8")	changeOffset3d(-8, true);
		else if	(key == "me3d_xoff_down")	changeOffset3d(-1, true);
		else if	(key == "me3d_yoff_up8")	changeOffset3d(8, false);
		else if	(key == "me3d_yoff_up")		changeOffset3d(1, false);
		else if	(key == "me3d_yoff_down8")	changeOffset3d(-8, false);
		else if	(key == "me3d_yoff_down")	changeOffset3d(-1, false);

		// Height changes
		else if	(key == "me3d_flat_height_up8")		changeSectorHeight3d(8);
		else if	(key == "me3d_flat_height_up")		changeSectorHeight3d(1);
		else if	(key == "me3d_flat_height_down8")	changeSectorHeight3d(-8);
		else if	(key == "me3d_flat_height_down")	changeSectorHeight3d(-1);

		// Thing height changes
		else if (key == "me3d_thing_up")	changeThingZ3d(1);
		else if (key == "me3d_thing_up8")	changeThingZ3d(8);
		else if (key == "me3d_thing_down")	changeThingZ3d(-1);
		else if (key == "me3d_thing_down8")	changeThingZ3d(-8);

		// Wall/Flat scale changes
		else if (key == "me3d_scalex_up_l" && ext) changeScale3d(1, true);
		else if (key == "me3d_scalex_up_s" && ext) changeScale3d(0.1, true);
		else if (key == "me3d_scalex_down_l" && ext) changeScale3d(-1, true);
		else if (key == "me3d_scalex_down_s" && ext) changeScale3d(-0.1, true);
		else if (key == "me3d_scaley_up_l" && ext) changeScale3d(1, false);
		else if (key == "me3d_scaley_up_s" && ext) changeScale3d(0.1, false);
		else if (key == "me3d_scaley_down_l" && ext) changeScale3d(-1, false);
		else if (key == "me3d_scaley_down_s" && ext) changeScale3d(-0.1, false);

		// Auto-align
		else if (key == "me3d_wall_autoalign_x")
			autoAlignX3d(hilight_3d);

		// Reset wall offsets
		else if (key == "me3d_wall_reset")
			resetWall3d();

		// Toggle lower unpegged
		else if (key == "me3d_wall_unpeg_lower")
			toggleUnpegged3d(true);

		// Toggle upper unpegged
		else if (key == "me3d_wall_unpeg_upper")
			toggleUnpegged3d(false);

		// Remove thing
		else if (key == "me3d_thing_remove")
			deleteThing3d();

		else
			return false;
	}

	return handled;
}

void MapEditor::updateDisplay()
{
	// Update map object properties panel
	vector<MapObject*> selection;
	getSelectedObjects(selection);
	theMapEditor->propsPanel()->openObjects(selection);

	// Update canvas info overlay
	if (canvas)
	{
		canvas->updateInfoOverlay();
		canvas->Refresh();
	}
}

#pragma region UNDO / REDO

void MapEditor::beginUndoRecord(string name, bool mod, bool create, bool del)
{
	// Setup
	undo_modified = mod;
	undo_deleted = del;
	undo_created = create;
	UndoManager* manager = (edit_mode == MODE_3D) ? undo_manager_3d : undo_manager;

	// Begin recording
	manager->beginRecord(name);

	// Init map/objects for recording
	if (undo_modified)
		MapObject::beginPropBackup(theApp->runTimer());
	if (undo_deleted || undo_created)
		map.clearCreatedDeletedOjbectIds();

	last_undo_level = "";
}

void MapEditor::beginUndoRecordLocked(string name, bool mod, bool create, bool del)
{
	if (name != last_undo_level)
	{
		beginUndoRecord(name, mod, create, del);
		last_undo_level = name;
	}
}

void MapEditor::endUndoRecord(bool success)
{
	UndoManager* manager = (edit_mode == MODE_3D) ? undo_manager_3d : undo_manager;

	if (manager->currentlyRecording())
	{
		// Record necessary undo steps
		MapObject::beginPropBackup(-1);
		bool modified = false;
		bool created_deleted = false;
		if (undo_modified)
			modified = manager->recordUndoStep(new MultiMapObjectPropertyChangeUS());
		if (undo_created || undo_deleted)
			created_deleted = manager->recordUndoStep(new MapObjectCreateDeleteUS());

		// End recording
		manager->endRecord(success && (modified || created_deleted));
	}
}

void MapEditor::recordPropertyChangeUndoStep(MapObject* object)
{
	UndoManager* manager = (edit_mode == MODE_3D) ? undo_manager_3d : undo_manager;
	manager->recordUndoStep(new PropertyChangeUS(object));
}

void MapEditor::doUndo()
{
	// Undo
	int time = theApp->runTimer() - 1;
	UndoManager* manager = (edit_mode == MODE_3D) ? undo_manager_3d : undo_manager;
	string undo_name = manager->undo();

	// Editor message
	if (undo_name != "")
	{
		addEditorMessage(S_FMT("Undo: %s", CHR(undo_name)));

		// Refresh stuff
		//updateTagged();
		map.rebuildConnectedLines();
		map.geometry_updated = theApp->runTimer();
		map.updateGeometryInfo(time);
		last_undo_level = "";
	}
}

void MapEditor::doRedo()
{
	// Redo
	int time = theApp->runTimer() - 1;
	UndoManager* manager = (edit_mode == MODE_3D) ? undo_manager_3d : undo_manager;
	string undo_name = manager->redo();

	// Editor message
	if (undo_name != "")
	{
		addEditorMessage(S_FMT("Redo: %s", CHR(undo_name)));

		// Refresh stuff
		//updateTagged();
		map.rebuildConnectedLines();
		map.geometry_updated = theApp->runTimer();
		map.updateGeometryInfo(time);
		last_undo_level = "";
	}
}

#pragma endregion

#pragma region CONSOLE COMMANDS

CONSOLE_COMMAND(m_show_item, 1, true)
{
	int index = atoi(CHR(args[0]));
	theMapEditor->mapEditor().showItem(index);
}

#pragma endregion






// testing stuff

CONSOLE_COMMAND(m_test_sector, 0, false)
{
	sf::Clock clock;
	SLADEMap& map = theMapEditor->mapEditor().getMap();
	for (unsigned a = 0; a < map.nThings(); a++)
		map.sectorAt(map.getThing(a)->xPos(), map.getThing(a)->yPos());
	long ms = clock.getElapsedTime().asMilliseconds();
	wxLogMessage("Took %dms", ms);
}

CONSOLE_COMMAND(m_test_mobj_backup, 0, false)
{
	sf::Clock clock;
	sf::Clock totalClock;
	SLADEMap& map = theMapEditor->mapEditor().getMap();
	mobj_backup_t* backup = new mobj_backup_t();

	// Vertices
	for (unsigned a = 0; a < map.nVertices(); a++)
		map.getVertex(a)->backup(backup);
	wxLogMessage("Vertices: %dms", clock.getElapsedTime().asMilliseconds());

	// Lines
	clock.restart();
	for (unsigned a = 0; a < map.nLines(); a++)
		map.getLine(a)->backup(backup);
	wxLogMessage("Lines: %dms", clock.getElapsedTime().asMilliseconds());

	// Sides
	clock.restart();
	for (unsigned a = 0; a < map.nSides(); a++)
		map.getSide(a)->backup(backup);
	wxLogMessage("Sides: %dms", clock.getElapsedTime().asMilliseconds());

	// Sectors
	clock.restart();
	for (unsigned a = 0; a < map.nSectors(); a++)
		map.getSector(a)->backup(backup);
	wxLogMessage("Sectors: %dms", clock.getElapsedTime().asMilliseconds());

	// Things
	clock.restart();
	for (unsigned a = 0; a < map.nThings(); a++)
		map.getThing(a)->backup(backup);
	wxLogMessage("Things: %dms", clock.getElapsedTime().asMilliseconds());

	wxLogMessage("Total: %dms", totalClock.getElapsedTime().asMilliseconds());
}

CONSOLE_COMMAND(m_vertex_attached, 1, false)
{
	MapVertex* vertex = theMapEditor->mapEditor().getMap().getVertex(atoi(CHR(args[0])));
	if (vertex)
	{
		wxLogMessage("Attached lines:");
		for (unsigned a = 0; a < vertex->nConnectedLines(); a++)
			wxLogMessage("Line #%d", vertex->connectedLine(a)->getIndex());
	}
}

CONSOLE_COMMAND(m_n_polys, 0, false)
{
	SLADEMap& map = theMapEditor->mapEditor().getMap();
	int npoly = 0;
	for (unsigned a = 0; a < map.nSectors(); a++)
		npoly += map.getSector(a)->getPolygon()->nSubPolys();

	theConsole->logMessage(S_FMT("%d polygons total", npoly));
}

CONSOLE_COMMAND(mobj_info, 1, false)
{
	long id;
	args[0].ToLong(&id);

	MapObject* obj = theMapEditor->mapEditor().getMap().getObjectById(id);
	if (!obj)
		theConsole->logMessage("Object id out of range");
	else
	{
		mobj_backup_t bak;
		obj->backup(&bak);
		theConsole->logMessage(S_FMT("Object %d: %s #%d", id, CHR(obj->getTypeName()), obj->getIndex()));
		theConsole->logMessage("Properties:");
		theConsole->logMessage(bak.properties.toString());
		theConsole->logMessage("Properties (internal):");
		theConsole->logMessage(bak.props_internal.toString());
	}
}

//CONSOLE_COMMAND(m_test_save, 1, false) {
//	vector<ArchiveEntry*> entries;
//	theMapEditor->mapEditor().getMap().writeDoomMap(entries);
//	WadArchive temp;
//	temp.addNewEntry("MAP01");
//	for (unsigned a = 0; a < entries.size(); a++)
//		temp.addEntry(entries[a]);
//	temp.save(args[0]);
//	temp.close();
//}
