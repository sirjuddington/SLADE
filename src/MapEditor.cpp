
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

EXTERN_CVAR(Int, shapedraw_sides)
EXTERN_CVAR(Int, shapedraw_shape)
EXTERN_CVAR(Bool, shapedraw_centered)
EXTERN_CVAR(Bool, shapedraw_lockratio)


MapEditor::MapEditor() {
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
	link_3d_light = true;
	link_3d_offset = true;
	undo_manager = new UndoManager();
}

MapEditor::~MapEditor() {
	if (copy_thing) delete copy_thing;
	if (copy_sector) delete copy_sector;
	delete undo_manager;
}

double MapEditor::gridSize() {
	return grid_sizes[gridsize];
}

void MapEditor::setEditMode(int mode) {
	// Check if we are changing to the same mode
	if (mode == edit_mode) {
		// Cycle sector edit mode
		if (mode == MODE_SECTORS)
			setSectorEditMode(sector_mode + 1);
		
		// Do nothing otherwise
		return;
	}
	
	// Set edit mode
	edit_mode = mode;
	sector_mode = SECTOR_BOTH;
	
	// Clear hilight and selection stuff
	hilight_item = -1;
	selection.clear();
	tagged_sectors.clear();
	tagged_lines.clear();
	tagged_things.clear();

	// Add editor message
	switch (edit_mode) {
	case MODE_VERTICES: addEditorMessage("Vertices mode"); break;
	case MODE_LINES:	addEditorMessage("Lines mode"); break;
	case MODE_SECTORS:	addEditorMessage("Sectors mode (Normal)"); break;
	case MODE_THINGS:	addEditorMessage("Things mode"); break;
	case MODE_3D:		addEditorMessage("3d mode"); break;
	default: break;
	};
}

void MapEditor::setSectorEditMode(int mode) {
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

bool MapEditor::openMap(Archive::mapdesc_t map) {
	wxLogMessage("Opening map %s", CHR(map.name));
	if (!this->map.readMap(map))
		return false;

	// Find camera thing
	if (canvas) {
		MapThing* cam = NULL;
		MapThing* pstart = NULL;
		for (unsigned a = 0; a < this->map.nThings(); a++) {
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

void MapEditor::clearMap() {
	// Clear map
	map.clearMap();

	// Clear selection
	selection.clear();
	hilight_item = -1;
	link_3d_light = true;
	link_3d_offset = true;
}

bool MapEditor::updateHilight(fpoint2_t mouse_pos, double dist_scale) {
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
	else if (edit_mode == MODE_THINGS) {
		hilight_item = -1;

		// Get (possibly multiple) nearest-thing(s)
		vector<int> nearest = map.nearestThingMulti(mouse_pos.x, mouse_pos.y);
		if (nearest.size() == 1) {
			MapThing* t = map.getThing(nearest[0]);
			ThingType* type = theGameConfiguration->thingType(t->getType());
			double dist = MathStuff::distance(mouse_pos.x, mouse_pos.y, t->xPos(), t->yPos());
			if (dist <= type->getRadius() + (32/dist_scale))
				hilight_item = nearest[0];
		}
		else {
			for (unsigned a = 0; a < nearest.size(); a++) {
				MapThing* t = map.getThing(nearest[a]);
				ThingType* type = theGameConfiguration->thingType(t->getType());
				double dist = MathStuff::distance(mouse_pos.x, mouse_pos.y, t->xPos(), t->yPos());
				if (dist <= type->getRadius() + (32/dist_scale))
					hilight_item = nearest[a];
			}
		}
	}

	// Update tagged lists if the hilight changed
	if (current != hilight_item) {
		// Clear tagged lists
		tagged_sectors.clear();
		tagged_lines.clear();
		tagged_things.clear();

		tagging_lines.clear();
		tagging_things.clear();

		// Special
		if (hilight_item >= 0) {
			// Gather affecting objects
			int type, tag = 0;
			if (edit_mode == MODE_LINES) { 
				type = SLADEMap::LINEDEFS;
				tag = map.getLine(hilight_item)->intProperty("id");
			} else if (edit_mode == MODE_THINGS) {
				type = SLADEMap::THINGS;
				tag = map.getThing(hilight_item)->intProperty("id");
			} else if (edit_mode == MODE_SECTORS) {
				type = SLADEMap::SECTORS;
				tag = map.getSector(hilight_item)->intProperty("id");
			}
			if (tag) {
				map.getTaggingLinesById(tag, type, tagging_lines);
				map.getTaggingThingsById(tag, type, tagging_things);
			}

			// Gather affected objects
			if (edit_mode == MODE_LINES || edit_mode == MODE_THINGS) {
				MapSector* back = NULL;
				MapSector* front = NULL;
				int needs_tag, tag, arg2, arg3, arg4, arg5;
				// Line specials have front and possibly back sectors
				if (edit_mode == MODE_LINES) {
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
				} else /* edit_mode == MODE_THINGS */ {
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
				else if (needs_tag == AS_TT_SECTOR_OR_BACK) {
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
				else if (needs_tag) {
					switch (needs_tag) {
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

	// Update map object properties panel if the hilight changed
	if (current != hilight_item && selection.size() == 0) {
		switch (edit_mode) {
		case MODE_VERTICES: theMapEditor->propsPanel()->openObject(map.getVertex(hilight_item)); break;
		case MODE_LINES: theMapEditor->propsPanel()->openObject(map.getLine(hilight_item)); break;
		case MODE_SECTORS: theMapEditor->propsPanel()->openObject(map.getSector(hilight_item)); break;
		case MODE_THINGS: theMapEditor->propsPanel()->openObject(map.getThing(hilight_item)); break;
		default: break;
		}
	}

	return current != hilight_item;
}

void MapEditor::selectionUpdated() {
	// Open selected objects in properties panel
	vector<MapObject*> objects;

	if (edit_mode == MODE_VERTICES) {
		for (unsigned a = 0; a < selection.size(); a++)
			objects.push_back(map.getVertex(selection[a]));
	}
	else if (edit_mode == MODE_LINES) {
		for (unsigned a = 0; a < selection.size(); a++)
			objects.push_back(map.getLine(selection[a]));
	}
	else if (edit_mode == MODE_SECTORS) {
		for (unsigned a = 0; a < selection.size(); a++)
			objects.push_back(map.getSector(selection[a]));
	}
	else if (edit_mode == MODE_THINGS) {
		for (unsigned a = 0; a < selection.size(); a++)
			objects.push_back(map.getThing(selection[a]));
	}

	theMapEditor->propsPanel()->openObjects(objects);
}

void MapEditor::clearSelection() {
	if (edit_mode == MODE_3D) {
		if (canvas) canvas->itemsSelected3d(selection_3d, false);
		selection_3d.clear();
	}
	else {
		if (canvas) canvas->itemsSelected(selection, false);
		selection.clear();
		theMapEditor->propsPanel()->openObject(NULL);
	}
}

void MapEditor::selectAll() {
	// Clear selection initially
	selection.clear();

	// Select all items depending on mode
	if (edit_mode == MODE_VERTICES) {
		for (unsigned a = 0; a < map.vertices.size(); a++)
			selection.push_back(a);
	}
	else if (edit_mode == MODE_LINES) {
		for (unsigned a = 0; a < map.lines.size(); a++)
			selection.push_back(a);
	}
	else if (edit_mode == MODE_SECTORS) {
		for (unsigned a = 0; a < map.sectors.size(); a++)
			selection.push_back(a);
	}
	else if (edit_mode == MODE_THINGS) {
		for (unsigned a = 0; a < map.things.size(); a++)
			selection.push_back(a);
	}

	addEditorMessage(S_FMT("Selected all %d %s", selection.size(), CHR(getModeString())));

	if (canvas)
		canvas->itemsSelected(selection);

	selectionUpdated();
}

bool MapEditor::selectCurrent(bool clear_none) {
	// --- 3d mode ---
	if (edit_mode == MODE_3D) {
		// If nothing is hilighted
		if (hilight_3d.index == -1) {
			// Clear selection if specified
			if (clear_none) {
				if (canvas) canvas->itemsSelected3d(selection_3d, false);
				selection_3d.clear();
				addEditorMessage("Selection cleared");
			}

			return false;
		}

		// Otherwise, check if item is in selection
		for (unsigned a = 0; a < selection_3d.size(); a++) {
			if (selection_3d[a].index == hilight_3d.index &&
				selection_3d[a].type == hilight_3d.type) {
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
	else {
		// If nothing is hilighted
		if (hilight_item == -1) {
			// Clear selection if specified
			if (clear_none) {
				if (canvas) canvas->itemsSelected(selection, false);
				selection.clear();
				selectionUpdated();
				addEditorMessage("Selection cleared");
			}

			return false;
		}

		// Otherwise, check if item is in selection
		for (unsigned a = 0; a < selection.size(); a++) {
			if (selection[a] == hilight_item) {
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

bool MapEditor::selectWithin(double xmin, double ymin, double xmax, double ymax, bool add) {
	// Select depending on editing mode
	bool selected;
	vector<int> nsel;
	vector<int> asel;

	// Vertices
	if (edit_mode == MODE_VERTICES) {
		// Go through vertices
		double x, y;
		for (unsigned a = 0; a < map.vertices.size(); a++) {
			// Check if already selected
			if (std::find(selection.begin(), selection.end(), a) != selection.end())
				selected = true;
			else
				selected = false;

			// Get position
			x = map.vertices[a]->xPos();
			y = map.vertices[a]->yPos();

			// Select if vertex is within bounds
			if (xmin <= x && x <= xmax && ymin <= y && y <= ymax) {
				if (selected)
					asel.push_back(a);
				else
					nsel.push_back(a);
			}
		}
	}

	// Lines
	else if (edit_mode == MODE_LINES) {
		// Go through lines
		MapLine* line;
		double x1, y1, x2, y2;
		for (unsigned a = 0; a < map.lines.size(); a++) {
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
				xmin <= x2 && x2 <= xmax && ymin <= y2 && y2 <= ymax) {
				if (selected)
					asel.push_back(a);
				else
					nsel.push_back(a);
			}
		}
	}

	// Sectors
	else if (edit_mode == MODE_SECTORS) {
		// Go through sectors
		fpoint2_t pmin(xmin, ymin);
		fpoint2_t pmax(xmax, ymax);
		for (unsigned a = 0; a < map.sectors.size(); a++) {
			// Check if already selected
			if (std::find(selection.begin(), selection.end(), a) != selection.end())
				selected = true;
			else
				selected = false;

			// Check if sector's bbox fits within the selection box
			if (map.sectors[a]->boundingBox().is_within(pmin, pmax)) {
				if (selected)
					asel.push_back(a);
				else
					nsel.push_back(a);
			}
		}
	}

	// Things
	else if (edit_mode == MODE_THINGS) {
		// Go through things
		double x, y;
		for (unsigned a = 0; a < map.things.size(); a++) {
			// Check if already selected
			if (std::find(selection.begin(), selection.end(), a) != selection.end())
				selected = true;
			else
				selected = false;

			// Get position
			x = map.things[a]->xPos();
			y = map.things[a]->yPos();

			// Select if thing is within bounds
			if (xmin <= x && x <= xmax && ymin <= y && y <= ymax) {
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
	if (!add) {
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

MapVertex* MapEditor::getHilightedVertex() {
	// Check edit mode is correct
	if (edit_mode != MODE_VERTICES)
		return NULL;

	// Having one item selected counts as a hilight
	if (hilight_item == -1 && selection.size() == 1)
		return map.getVertex(selection[0]);

	return map.getVertex(hilight_item);
}

MapLine* MapEditor::getHilightedLine() {
	// Check edit mode is correct
	if (edit_mode != MODE_LINES)
		return NULL;

	// Having one item selected counts as a hilight
	if (hilight_item == -1 && selection.size() == 1)
		return map.getLine(selection[0]);

	return map.getLine(hilight_item);
}

MapSector* MapEditor::getHilightedSector() {
	// Check edit mode is correct
	if (edit_mode != MODE_SECTORS)
		return NULL;

	// Having one item selected counts as a hilight
	if (hilight_item == -1 && selection.size() == 1)
		return map.getSector(selection[0]);

	return map.getSector(hilight_item);
}

MapThing* MapEditor::getHilightedThing() {
	// Check edit mode is correct
	if (edit_mode != MODE_THINGS)
		return NULL;

	// Having one item selected counts as a hilight
	if (hilight_item == -1 && selection.size() == 1)
		return map.getThing(selection[0]);

	return map.getThing(hilight_item);
}

MapObject* MapEditor::getHilightedObject() {
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

void MapEditor::getSelectedVertices(vector<MapVertex*>& list) {
	if (edit_mode != MODE_VERTICES)
		return;

	// Multiple selection
	if (selection.size() > 1) {
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

void MapEditor::getSelectedLines(vector<MapLine*>& list) {
	if (edit_mode == MODE_LINES) {
		// Multiple selection
		if (selection.size() > 1) {
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
	else if (edit_mode == MODE_SECTORS) {
		// Get selected sectors
		vector<MapSector*> sectors;
		getSelectedSectors(sectors);

		// Add lines of selected sectors
		for (unsigned a = 0; a < sectors.size(); a++) {
			vector<MapLine*> seclines;
			sectors[a]->getLines(seclines);
			for (unsigned b = 0; b < seclines.size(); b++) {
				if (std::find(list.begin(), list.end(), seclines[b]) == list.end())
					list.push_back(seclines[b]);
			}
		}
	}
}

void MapEditor::getSelectedSectors(vector<MapSector*>& list) {
	if (edit_mode != MODE_SECTORS)
		return;

	// Multiple selection
	if (selection.size() > 1) {
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

void MapEditor::getSelectedThings(vector<MapThing*>& list) {
	if (edit_mode == MODE_3D) {
		// Multiple selection
		if (selection_3d.size() > 1) {
			for (unsigned a = 0; a < selection_3d.size(); a++) {
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
	else if (edit_mode == MODE_THINGS) {
		// Multiple selection
		if (selection.size() > 1) {
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

void MapEditor::getSelectedObjects(vector<MapObject*>& list) {
	// Go through selection
	if (selection.size() > 0) {
		for (unsigned a = 0; a < selection.size(); a++) {
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
	else {
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

void MapEditor::showItem(int index) {
	selection.clear();
	int max = 0;
	switch (edit_mode) {
	case MODE_VERTICES: max = map.nVertices(); break;
	case MODE_LINES: max = map.nLines(); break;
	case MODE_SECTORS: max = map.nSectors(); break;
	case MODE_THINGS: max = map.nThings(); break;
	default: max = 0; break;
	}

	if (index < max) {
		selection.push_back(index);
		if (canvas) canvas->viewShowObject();
	}
}

void MapEditor::selectItem3d(selection_3d_t item, int sel) {
	// Go through selection
	for (unsigned a = 0; a < selection_3d.size(); a++) {
		// Check for match
		if (selection_3d[a].index == item.index && selection_3d[a].type == item.type) {
			// Selecting, do nothing
			if (sel == SELECT)
				return;

			// Deselecting, remove from selection list
			else if (sel == DESELECT || sel == TOGGLE) {
				selection_3d[a] = selection_3d.back();
				selection_3d.pop_back();
				return;
			}
		}
	}

	// Selection didn't exist, add if selecting or toggling
	if (sel == SELECT || sel == TOGGLE) {
		selection_3d.push_back(item);
		if (canvas) canvas->itemSelected3d(item);
	}
}

void MapEditor::incrementGrid() {
	gridsize++;
	if (gridsize > 20)
		gridsize = 20;

	addEditorMessage(S_FMT("Grid Size: %dx%d", (int)gridSize(), (int)gridSize()));
}

void MapEditor::decrementGrid() {
	// Non-integral grid size disabled for now
	gridsize--;
	if (gridsize < 4)
		gridsize = 4;

	addEditorMessage(S_FMT("Grid Size: %dx%d", (int)gridSize(), (int)gridSize()));
}

double MapEditor::snapToGrid(double position) {
	// This won't work with non-integer grid sizes for now
	int upper, lower;

	for (int i = position; i >= (position - gridSize()); i--) {
		if ((i % (int)gridSize()) == 0) {
			lower = i;
			break;
		}
	}

	for (int i = position; i < (position + gridSize()); i++) {
		if ((i % (int)gridSize()) == 0) {
			upper = i;
			break;
		}
	}

	double mid = lower + ((upper - lower) / 2.0);

	if (position > mid)
		return upper;
	else
		return lower;
}

bool MapEditor::beginMove(fpoint2_t mouse_pos) {
	// Check if we have any selection or hilight
	if (selection.size() == 0 && hilight_item == -1)
		return false;

	// Begin move operation
	move_origin = mouse_pos;

	// Create list of objects to move
	if (selection.size() == 0)
		move_items.push_back(hilight_item);
	else {
		for (unsigned a = 0; a < selection.size(); a++)
			move_items.push_back(selection[a]);
	}

	// Get list of vertices being moved (if any)
	vector<MapVertex*> move_verts;
	if (edit_mode != MODE_THINGS) {
		// Vertices mode
		if (edit_mode == MODE_VERTICES) {
			for (unsigned a = 0; a < move_items.size(); a++)
				move_verts.push_back(map.getVertex(move_items[a]));
		}

		// Lines mode
		else if (edit_mode == MODE_LINES) {
			for (unsigned a = 0; a < move_items.size(); a++) {
				// Duplicate vertices shouldn't matter here
				move_verts.push_back(map.getLine(move_items[a])->v1());
				move_verts.push_back(map.getLine(move_items[a])->v2());
			}
		}

		// Sectors mode
		else if (edit_mode == MODE_SECTORS) {
			for (unsigned a = 0; a < move_items.size(); a++)
				map.getSector(move_items[a])->getVertices(move_verts);
		}
	}

	// Filter out map objects being moved
	if (edit_mode == MODE_THINGS) {
		// Filter moving things
		for (unsigned a = 0; a < move_items.size(); a++)
			map.getThing(move_items[a])->filter(true);
	}
	else {
		// Filter moving lines
		for (unsigned a = 0; a < move_verts.size(); a++) {
			for (unsigned l = 0; l < move_verts[a]->nConnectedLines(); l++)
				move_verts[a]->connectedLine(l)->filter(true);
		}
	}

	return true;
}

void MapEditor::doMove(fpoint2_t mouse_pos) {
	// Special case: single vertex or thing
	if (move_items.size() == 1 && (edit_mode == MODE_VERTICES || edit_mode == MODE_THINGS)) {
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

void MapEditor::endMove(bool accept) {
	long move_time = theApp->runTimer();

	// Move depending on edit mode
	if (edit_mode == MODE_THINGS && accept) {
		// Move things
		undo_manager->beginRecord("Move Things");
		for (unsigned a = 0; a < move_items.size(); a++) {
			MapThing* t = map.getThing(move_items[a]);
			map.moveThing(move_items[a], t->xPos() + move_vec.x, t->yPos() + move_vec.y);
		}
		undo_manager->endRecord(true);
	}
	else if (accept) {
		// Any other edit mode we're technically moving vertices
		undo_manager->beginRecord("Move Vertices");

		// Get list of vertices being moved
		bool* move_verts = new bool[map.nVertices()];
		memset(move_verts, 0, map.nVertices());

		if (edit_mode == MODE_VERTICES) {
			for (unsigned a = 0; a < move_items.size(); a++)
				move_verts[move_items[a]] = true;
		}
		else if (edit_mode == MODE_LINES) {
			for (unsigned a = 0; a < move_items.size(); a++) {
				MapLine* line = map.getLine(move_items[a]);
				if (line->v1()) move_verts[map.vertexIndex(line->v1())] = true;
				if (line->v2()) move_verts[map.vertexIndex(line->v2())] = true;
			}
		}
		else if (edit_mode == MODE_SECTORS) {
			vector<MapVertex*> sv;
			for (unsigned a = 0; a < move_items.size(); a++)
				map.getSector(move_items[a])->getVertices(sv);

			for (unsigned a = 0; a < sv.size(); a++)
				move_verts[map.vertexIndex(sv[a])] = true;
		}

		// Move vertices
		vector<fpoint2_t> merge_points;
		vector<unsigned> moved_lines;
		for (unsigned a = 0; a < map.nVertices(); a++) {
			if (!move_verts[a])
				continue;
			fpoint2_t np(map.getVertex(a)->xPos() + move_vec.x, map.getVertex(a)->yPos() + move_vec.y);
			map.moveVertex(a, np.x, np.y);
			merge_points.push_back(np);
		}

		// Merge vertices and split lines
		for (unsigned a = 0; a < merge_points.size(); a++) {
			MapVertex* v = map.mergeVerticesPoint(merge_points[a].x, merge_points[a].y);
			if (v) map.splitLinesAt(v, 1);
		}

		// Split lines overlapping vertices
		for (unsigned a = 0; a < map.nLines(); a++) {
			MapLine* line = map.getLine(a);
			if (line->modifiedTime() >= move_time) {
				MapVertex* split = map.lineCrossVertex(line->x1(), line->y1(), line->x2(), line->y2());
				if (split) {
					map.splitLine(a, split->getIndex());
					a = 0;
				}
			}
		}

		// Merge lines
		for (unsigned a = 0; a < map.nLines(); a++) {
			if (map.getLine(a)->modifiedTime() >= move_time) {
				if (map.mergeLine(a) > 0) {
					map.getLine(a)->clearUnneededTextures();
					a = 0;
				}
			}
		}

		// Remove any resulting zero-length lines
		map.removeZeroLengthLines();

		undo_manager->endRecord(true);
	}

	// Un-filter objects
	for (unsigned a = 0; a < map.nLines(); a++)
		map.getLine(a)->filter(false);
	for (unsigned a = 0; a < map.nThings(); a++)
		map.getThing(a)->filter(false);

	// Clear moving items
	move_items.clear();

	// Update map item indices
	map.refreshIndices();
}

void MapEditor::copyProperties(MapObject* object) {
	// Do nothing if no selection or hilight
	if (selection.size() == 0 && hilight_item < 0)
		return;

	// Sectors mode
	if (edit_mode == MODE_SECTORS) {
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
	else if (edit_mode == MODE_THINGS) {
		// Create copy thing if needed
		if (!copy_thing)
			copy_thing = new MapThing(NULL);

		// Copy given object properties (if any)
		if (object && object->getObjType() == MOBJ_THING)
			copy_thing->copy(object);
		else {
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
}

void MapEditor::pasteProperties() {
	// Do nothing if no selection or hilight
	if (selection.size() == 0 && hilight_item < 0)
		return;

	// Sectors mode
	if (edit_mode == MODE_SECTORS) {
		// Do nothing if no properties have been copied
		if (!copy_sector)
			return;

		// Paste properties to selection/hilight
		if (selection.size() > 0) {
			for (unsigned a = 0; a < selection.size(); a++)
				map.getSector(selection[a])->copy(copy_sector);
		}
		else if (hilight_item >= 0)
			map.getSector(hilight_item)->copy(copy_sector);

		// Editor message
		addEditorMessage("Pasted sector properties");
	}

	// Things mode
	if (edit_mode == MODE_THINGS) {
		// Do nothing if no properties have been copied
		if (!copy_thing)
			return;

		// Paste properties to selection/hilight
		if (selection.size() > 0) {
			for (unsigned a = 0; a < selection.size(); a++) {
				// Paste properties (but keep position)
				MapThing* thing = map.getThing(selection[a]);
				double x = thing->xPos();
				double y = thing->yPos();
				thing->copy(copy_thing);
				thing->setFloatProperty("x", x);
				thing->setFloatProperty("y", y);
			}
		}
		else if (hilight_item >= 0) {
			// Paste properties (but keep position)
			MapThing* thing = map.getThing(hilight_item);
			double x = thing->xPos();
			double y = thing->yPos();
			thing->copy(copy_thing);
			thing->setFloatProperty("x", x);
			thing->setFloatProperty("y", y);
		}

		// Editor message
		addEditorMessage("Pasted thing properties");
	}

	// Update display
	updateDisplay();
}

void MapEditor::splitLine(double x, double y, double min_dist) {
	// Get the closest line
	int lindex = map.nearestLine(x, y, min_dist);
	MapLine* line = map.getLine(lindex);

	// Do nothing if no line is close enough
	if (!line)
		return;

	// Get closest point on the line
	fpoint2_t closest = MathStuff::closestPointOnLine(x, y, line->x1(), line->y1(), line->x2(), line->y2());

	// Create vertex there
	MapVertex* vertex = map.createVertex(closest.x, closest.y);
	int vindex = map.vertexIndex(vertex);

	// Do line split
	map.splitLine(lindex, vindex);
}

void MapEditor::flipLines(bool sides) {
	// Get selected/hilighted line(s)
	vector<MapLine*> lines;
	getSelectedLines(lines);

	// Go through list
	for (unsigned a = 0; a < lines.size(); a++)
		lines[a]->flip(sides);

	// Update display
	canvas->forceRefreshRenderer();
	updateDisplay();
}

void MapEditor::changeSectorHeight(int amount, bool floor, bool ceiling) {
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
	if (floor && ceiling) {
		if (sector_mode == SECTOR_FLOOR)
			ceiling = false;
		if (sector_mode == SECTOR_CEILING)
			floor = false;
	}

	// Go through selection
	for (unsigned a = 0; a < selection.size(); a++) {
		// Change floor height
		if (floor) {
			int height = selection[a]->intProperty("heightfloor");
			selection[a]->setIntProperty("heightfloor", height + amount);
		}

		// Change ceiling height
		if (ceiling) {
			int height = selection[a]->intProperty("heightceiling");
			selection[a]->setIntProperty("heightceiling", height + amount);
		}
	}

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

void MapEditor::changeSectorLight(bool up, bool fine) {
	// Do nothing if not in sectors mode
	if (edit_mode != MODE_SECTORS)
		return;

	// Get selected sectors (if any)
	vector<MapSector*> selection;
	getSelectedSectors(selection);

	// Do nothing if no selection or hilight
	if (selection.size() == 0)
		return;

	// Go through selection
	for (unsigned a = 0; a < selection.size(); a++) {
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

	// Add editor message
	string dir = up ? "increased" : "decreased";
	int amount = fine ? 1 : theGameConfiguration->lightLevelInterval();
	addEditorMessage(S_FMT("Light level %s by %d", CHR(dir), amount));

	// Update display
	updateDisplay();
}

void MapEditor::joinSectors(bool remove_lines) {
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

	// Go through merge sectors
	for (unsigned a = 1; a < sectors.size(); a++) {
		// Go through sector sides
		MapSector* sector = sectors[a];
		while (sector->connectedSides().size() > 0) {
			// Set sector
			MapSide* side = sector->connectedSides()[0];
			side->setSector(target);

			// Add line to list if not already there
			bool exists = false;
			for (unsigned l = 0; l < lines.size(); l++) {
				if (side->getParentLine() == lines[l]) {
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
	if (remove_lines) {
		for (unsigned a = 0; a < lines.size(); a++) {
			if (lines[a]->frontSector() == target && lines[a]->backSector() == target) {
				map.removeLine(lines[a]);
				nlines++;
			}
		}
	}

	// Editor message
	if (nlines == 0)
		addEditorMessage(S_FMT("Joined %d Sectors", selection.size()));
	else
		addEditorMessage(S_FMT("Joined %d Sectors (removed %d Lines)", selection.size(), nlines));
}

void MapEditor::changeThingType(int newtype) {
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
	for (unsigned a = 0; a < selection.size(); a++)
		selection[a]->setIntProperty("type", newtype);

	// Add editor message
	string type_name = theGameConfiguration->thingType(newtype)->getName();
	if (selection.size() == 1)
		addEditorMessage(S_FMT("Changed type to \"%s\"", CHR(type_name)));
	else
		addEditorMessage(S_FMT("Changed %d things to type \"%s\"", selection.size(), CHR(type_name)));

	// Update display
	updateDisplay();
}

void MapEditor::thingQuickAngle(fpoint2_t mouse_pos) {
	// Do nothing if not in things mode
	if (edit_mode != MODE_THINGS)
		return;

	// If selection is empty, check for hilight
	if (selection.size() == 0 && hilight_item >= 0) {
		map.getThing(hilight_item)->setAnglePoint(mouse_pos);
		return;
	}

	// Go through selection
	for (unsigned a = 0; a < selection.size(); a++)
		map.getThing(selection[a])->setAnglePoint(mouse_pos);
}

void MapEditor::createObject(double x, double y) {
	// Vertices mode
	if (edit_mode == MODE_VERTICES) {
		// If there are less than 2 vertices currently selected, just create a vertex at x,y
		if (selection.size() < 2)
			createVertex(x, y);
		else {
			// Otherwise, create lines between selected vertices
			for (unsigned a = 0; a < selection.size() - 1; a++)
				map.createLine(map.getVertex(selection[a]), map.getVertex(selection[a+1]));

			// Editor message
			addEditorMessage(S_FMT("Created %d line(s)", selection.size() - 1));

			// Clear selection
			clearSelection();
		}

		return;
	}

	// Sectors mode
	if (edit_mode == MODE_SECTORS) {
		// Sector 
		if (map.nLines() > 0) {
			createSector(x, y);
		} else {
			// Just create a vertex
			createVertex(x, y);
			setEditMode(MODE_LINES);
		}
		return;
	}

	// Things mode
	if (edit_mode == MODE_THINGS) {
		createThing(x, y);
		return;
	}
}

void MapEditor::createVertex(double x, double y) {
	// Snap coordinates to grid if necessary
	if (grid_snap) {
		x = snapToGrid(x);
		y = snapToGrid(y);
	}

	// Create vertex
	MapVertex* vertex = map.createVertex(x, y, 2);

	// Editor message
	if (vertex)
		addEditorMessage(S_FMT("Created vertex at (%d, %d)", (int)vertex->xPos(), (int)vertex->yPos()));
}

void MapEditor::createThing(double x, double y) {
	// Snap coordinates to grid if necessary
	if (grid_snap) {
		x = snapToGrid(x);
		y = snapToGrid(y);
	}

	// Create thing
	MapThing* thing = map.createThing(x, y);

	// Setup properties
	if (copy_thing) {
		// Copy properties from the last copied thing (except position)
		double x = thing->xPos();
		double y = thing->yPos();
		thing->copy(copy_thing);
		thing->setFloatProperty("x", x);
		thing->setFloatProperty("y", y);
	}
	else
		theGameConfiguration->applyDefaults(thing);	// No thing properties to copy, get defaults from game configuration

	// Editor message
	if (thing)
		addEditorMessage(S_FMT("Created thing at (%d, %d)", (int)thing->xPos(), (int)thing->yPos()));
}

void MapEditor::createSector(double x, double y) {
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
		builder.createSector(NULL, sector_copy);

	// Set some sector defaults from game configuration if needed
	if (!sector_copy && ok) {
		MapSector* n_sector = map.getSector(map.nSectors()-1);
		if (n_sector->ceilingTexture().IsEmpty())
			theGameConfiguration->applyDefaults(n_sector);
	}

	// Editor message
	if (ok)
		addEditorMessage(S_FMT("Created sector #%d", map.nSectors() - 1));
	else
		addEditorMessage("Sector creation failed: " + builder.getError());

	// Refresh map canvas
	canvas->forceRefreshRenderer();
}

void MapEditor::deleteObject() {
	// Vertices mode
	if (edit_mode == MODE_VERTICES) {
		// Get selected vertices
		vector<MapVertex*> verts;
		getSelectedVertices(verts);
		int index = -1;
		if (verts.size() == 1)
			index = verts[0]->getIndex();

		// Delete them (if any)
		for (unsigned a = 0; a < verts.size(); a++)
			map.removeVertex(verts[a]);

		// Editor message
		if (verts.size() == 1)
			addEditorMessage(S_FMT("Deleted vertex #%d", index));
		else if (verts.size() > 1)
			addEditorMessage(S_FMT("Deleted %d vertices", verts.size()));
	}

	// Lines mode
	else if (edit_mode == MODE_LINES) {
		// Get selected lines
		vector<MapLine*> lines;
		getSelectedLines(lines);
		int index = -1;
		if (lines.size() == 1)
			index = lines[0]->getIndex();

		// Delete them (if any)
		for (unsigned a = 0; a < lines.size(); a++)
			map.removeLine(lines[a]);

		// Editor message
		if (lines.size() == 1)
			addEditorMessage(S_FMT("Deleted line #%d", index));
		else if (lines.size() > 1)
			addEditorMessage(S_FMT("Deleted %d lines", lines.size()));
	}

	// Sectors mode
	else if (edit_mode == MODE_SECTORS) {
		// Get selected sectors
		vector<MapSector*> sectors;
		getSelectedSectors(sectors);
		int index = -1;
		if (sectors.size() == 1)
			index = sectors[0]->getIndex();

		// Delete them (if any), and keep a list of connected sides
		vector<MapSide*> connected_sides;
		for (unsigned a = 0; a < sectors.size(); a++) {
			for (unsigned s = 0; s < sectors[a]->connectedSides().size(); s++)
				connected_sides.push_back(sectors[a]->connectedSides()[s]);

			map.removeSector(sectors[a]);
		}

		// Remove all connected sides
		for (unsigned a = 0; a < connected_sides.size(); a++) {
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

		// Refresh map view
		theMapEditor->forceRefresh(true);
	}

	// Things mode
	else if (edit_mode == MODE_THINGS) {
		// Get selected things
		vector<MapThing*> things;
		getSelectedThings(things);
		int index = -1;
		if (things.size() == 1)
			index = things[0]->getIndex();

		// Delete them (if any)
		for (unsigned a = 0; a < things.size(); a++)
			map.removeThing(things[a]);

		// Editor message
		if (things.size() == 1)
			addEditorMessage(S_FMT("Deleted thing #%d", index));
		else if (things.size() > 1)
			addEditorMessage(S_FMT("Deleted %d things", things.size()));
	}

	// Remove detached vertices
	map.removeDetachedVertices();

	// Clear hilight and selection
	selection.clear();
	hilight_item = -1;
}

fpoint2_t MapEditor::lineDrawPoint(unsigned index) {
	// Check index
	if (index >= draw_points.size())
		return fpoint2_t(0, 0);
	
	return draw_points[index];
}

bool MapEditor::addLineDrawPoint(fpoint2_t point, bool nearest) {
	// Snap to nearest vertex if necessary
	if (nearest) {
		int vertex = map.nearestVertex(point.x, point.y);
		if (vertex >= 0) {
			point.x = map.getVertex(vertex)->xPos();
			point.y = map.getVertex(vertex)->yPos();
		}
	}

	// Otherwise, snap to grid if necessary
	else if (grid_snap) {
		point.x = snapToGrid(point.x);
		point.y = snapToGrid(point.y);
	}

	// Check if this is the same as the last point
	if (draw_points.size() > 0 && point.x == draw_points.back().x && point.y == draw_points.back().y) {
		// End line drawing
		endLineDraw(true);
		return true;
	}

	// Add point
	draw_points.push_back(point);

	// Check if first and last points match
	if (draw_points.size() > 1 && point.x == draw_points[0].x && point.y == draw_points[0].y) {
		endLineDraw(true);
		return true;
	}

	return false;
}

void MapEditor::removeLineDrawPoint() {
	draw_points.pop_back();
}

void MapEditor::setShapeDrawOrigin(fpoint2_t point, bool nearest) {
	// Snap to nearest vertex if necessary
	if (nearest) {
		int vertex = map.nearestVertex(point.x, point.y);
		if (vertex >= 0) {
			point.x = map.getVertex(vertex)->xPos();
			point.y = map.getVertex(vertex)->yPos();
		}
	}

	// Otherwise, snap to grid if necessary
	else if (grid_snap) {
		point.x = snapToGrid(point.x);
		point.y = snapToGrid(point.y);
	}

	draw_origin = point;
}

void MapEditor::updateShapeDraw(fpoint2_t point) {
	// Clear line draw points
	draw_points.clear();

	// Snap edge to grid if needed
	if (grid_snap) {
		point.x = snapToGrid(point.x);
		point.y = snapToGrid(point.y);
	}

	// Lock width:height at 1:1 if needed
	fpoint2_t origin = draw_origin;
	double width = abs(point.x - origin.x);
	double height = abs(point.y - origin.y);
	if (shapedraw_lockratio) {
		if (width < height) {
			if (origin.x < point.x)
				point.x = origin.x + height;
			else
				point.x = origin.x - height;
		}

		if (height < width) {
			if (origin.y < point.y)
				point.y = origin.y + width;
			else
				point.y = origin.y - width;
		}
	}

	// Center on origin if needed
	if (shapedraw_centered) {
		origin.x -= (point.x - origin.x);
		origin.y -= (point.y - origin.y);
	}

	// Get box from tl->br
	fpoint2_t tl(min(origin.x, point.x), min(origin.y, point.y));
	fpoint2_t br(max(origin.x, point.x), max(origin.y, point.y));
	width = br.x - tl.x;
	height = br.y - tl.y;

	// Rectangle
	if (shapedraw_shape == 0) {
		draw_points.push_back(fpoint2_t(tl.x, tl.y));
		draw_points.push_back(fpoint2_t(tl.x, br.y));
		draw_points.push_back(fpoint2_t(br.x, br.y));
		draw_points.push_back(fpoint2_t(br.x, tl.y));
		draw_points.push_back(fpoint2_t(tl.x, tl.y));
	}

	// Ellipse
	else if (shapedraw_shape == 1) {
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
		for (int a = 0; a < shapedraw_sides; a++) {
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

struct me_ls_t {
	MapLine*	line;
	bool		front;
	bool		ignore;
	me_ls_t(MapLine* line, bool front) { this->line = line; this->front = front; ignore = false; }
};

void MapEditor::endLineDraw(bool apply) {
	// Check if we want to 'apply' the line draw (ie. create the lines)
	if (apply && draw_points.size() > 1) {
		// Add extra points if any lines overlap existing vertices
		for (unsigned a = 0; a < draw_points.size() - 1; a++) {
			MapVertex* v = map.lineCrossVertex(draw_points[a].x, draw_points[a].y, draw_points[a+1].x, draw_points[a+1].y);
			while (v) {
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
		for (unsigned a = 0; a < draw_points.size() - 1; a++) {
			// Check for intersections
			vector<fpoint2_t> intersect = map.cutLines(draw_points[a].x, draw_points[a].y, draw_points[a+1].x, draw_points[a+1].y);
			//wxLogMessage("%d intersect points", intersect.size());

			// Create line normally if no intersections
			if (intersect.size() == 0)
				map.createLine(draw_points[a].x, draw_points[a].y, draw_points[a+1].x, draw_points[a+1].y, 1);
			else {
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

		// Create a list of line sides (edges) to perform sector creation with
		vector<me_ls_t> edges;
		for (unsigned a = nl_start; a < map.nLines(); a++) {
			edges.push_back(me_ls_t(map.getLine(a), true));
			fpoint2_t mid = map.getLine(a)->midPoint();
			if (map.sectorAt(mid.x, mid.y) >= 0)
				edges.push_back(me_ls_t(map.getLine(a), false));
		}
		
		// Build sectors
		SectorBuilder builder;
		int runs = 0;
		unsigned ns_start = map.nSectors();
		unsigned nsd_start = map.nSides();
		vector<MapSector*> sectors_reused;
		for (unsigned a = 0; a < edges.size(); a++) {
			// Skip if edge is ignored
			if (edges[a].ignore)
				continue;

			// Run sector builder on current edge
			bool ok = builder.traceSector(&map, edges[a].line, edges[a].front);
			runs++;

			// Ignore any subsequent edges that were part of the sector created
			for (unsigned e = a; e < edges.size(); e++) {
				if (edges[e].ignore)
					continue;

				for (unsigned b = 0; b < builder.nEdges(); b++) {
					if (edges[e].line == builder.getEdgeLine(b) &&
						edges[e].front == builder.edgeIsFront(b))
						edges[e].ignore = true;
				}
			}

			// Don't create sector if trace failed
			if (!ok)
				continue;

			// Check if we traced over an existing sector (or part of one)
			MapSector* sector = builder.findExistingSector();
			if (sector) {
				// Check if it's already been (re)used
				bool reused = false;
				for (unsigned s = 0; s < sectors_reused.size(); s++) {
					if (sectors_reused[s] == sector) {
						reused = true;
						break;
					}
				}

				// If we can reuse the sector, do so
				if (!reused)
					sectors_reused.push_back(sector);
				else
					sector = NULL;
			}
			
			// Create sector
			builder.createSector(sector);
		}

		//wxLogMessage("Ran sector builder %d times", runs);

		// Check if any new lines need to be flipped
		for (unsigned a = nl_start; a < map.nLines(); a++) {
			MapLine* line = map.getLine(a);
			if (line->backSector() && !line->frontSector())
				line->flip(true);
		}

		// Find an adjacent sector to copy properties from
		MapSector* sector_copy = NULL;
		for (unsigned a = nl_start; a < map.nLines(); a++) {
			// Check front sector
			MapSector* sector = map.getLine(a)->frontSector();
			if (sector && sector->getIndex() < ns_start) {
				// Copy this sector if it isn't newly created
				sector_copy = sector;
				break;
			}

			// Check back sector
			sector = map.getLine(a)->backSector();
			if (sector && sector->getIndex() < ns_start) {
				// Copy this sector if it isn't newly created
				sector_copy = sector;
				break;
			}
		}

		// Go through newly created sectors
		for (unsigned a = ns_start; a < map.nSectors(); a++) {
			MapSector* sector = map.getSector(a);

			// Skip if sector already has properties
			if (!sector->ceilingTexture().IsEmpty())
				continue;

			// Copy from adjacent sector if any
			if (sector_copy) {
				sector->copy(sector_copy);
				continue;
			}

			// Otherwise, use defaults from game configuration
			theGameConfiguration->applyDefaults(sector);
		}

		// Update line textures
		for (unsigned a = nsd_start; a < map.nSides(); a++) {
			MapSide* side = map.getSide(a);

			// Clear any unneeded textures
			MapLine* line = side->getParentLine();
			line->clearUnneededTextures();

			// Set middle texture if needed
			if (side == line->s1() && !line->s2() && side->stringProperty("texturemiddle") == "-") {
				wxLogMessage("midtex");
				// Find adjacent texture (any)
				string tex = map.getAdjacentLineTexture(line->v1());
				if (tex == "-")
					tex = map.getAdjacentLineTexture(line->v2());

				// If no adjacent texture, get default from game configuration
				if (tex == "-")
					tex = theGameConfiguration->getDefaultString(MOBJ_SIDE, "texturemiddle");

				// Set texture
				side->setStringProperty("texturemiddle", tex);
			}
		}
	}

	// Clear draw points
	draw_points.clear();
}

void MapEditor::copy() {
	// Can't copy/paste vertices (no point)
	if (edit_mode == MODE_VERTICES) {
		addEditorMessage("Copy/Paste not supported for vertices");
		return;
	}

	// Clear current clipboard contents
	theClipboard->clear();

	// Copy lines
	if (edit_mode == MODE_LINES || edit_mode == MODE_SECTORS) {
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
	else if (edit_mode == MODE_THINGS) {
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

void MapEditor::paste(fpoint2_t mouse_pos) {
	// Go through clipboard items
	for (unsigned a = 0; a < theClipboard->nItems(); a++) {
		// Map architecture
		if (theClipboard->getItem(a)->getType() == CLIPBOARD_MAP_ARCH) {
			MapArchClipboardItem* p = (MapArchClipboardItem*)theClipboard->getItem(a);
			p->pasteToMap(&map, mouse_pos);
			addEditorMessage(S_FMT("Pasted %s", CHR(p->getInfo())));
		}

		// Things
		else if (theClipboard->getItem(a)->getType() == CLIPBOARD_MAP_THINGS) {
			MapThingsClipboardItem* p = (MapThingsClipboardItem*)theClipboard->getItem(a);
			p->pasteToMap(&map, mouse_pos);
			addEditorMessage(S_FMT("Pasted %s", CHR(p->getInfo())));
		}
	}
}

bool MapEditor::wallMatches(MapSide* side, uint8_t part, string tex) {
	// Check for blank texture where it isn't needed
	if (tex == "-") {
		MapLine* line = side->getParentLine();
		int needed = line->needsTexture();
		if (side == line->s1()) {
			if (part == SEL_SIDE_TOP && (needed & TEX_FRONT_UPPER) == 0)
				return false;
			if (part == SEL_SIDE_MIDDLE && (needed & TEX_FRONT_MIDDLE) == 0)
				return false;
			if (part == SEL_SIDE_BOTTOM && (needed & TEX_FRONT_LOWER) == 0)
				return false;
		}
		else if (side == line->s2()) {
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

	// Check it isn't already selected
	/*
	for (unsigned s = 0; s < selection_3d.size(); s++) {
		if (selection_3d[s].type == part && selection_3d[s].index == side->getIndex())
			return false;
	}
	*/

	return true;
}

void MapEditor::getAdjacentWalls3d(selection_3d_t item, vector<selection_3d_t>& list) {
	// Add item to list if needed
	for (unsigned a = 0; a < list.size(); a++) {
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
	for (unsigned a = 0; a < line->v1()->nConnectedLines(); a++) {
		MapLine* oline = line->v1()->connectedLine(a);
		if (!oline || oline == line)
			continue;

		// Get line sides
		MapSide* side1 = oline->s1();
		MapSide* side2 = oline->s2();

		// Front side
		if (side1) {
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
		if (side2) {
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
	for (unsigned a = 0; a < line->v2()->nConnectedLines(); a++) {
		MapLine* oline = line->v2()->connectedLine(a);
		if (!oline || oline == line)
			continue;

		// Get line sides
		MapSide* side1 = oline->s1();
		MapSide* side2 = oline->s2();

		// Front side
		if (side1) {
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
		if (side2) {
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

void MapEditor::selectAdjacent3d(selection_3d_t item) {
	// Check item
	if (item.index < 0)
		return;

	// Select item
	selectItem3d(item, SELECT);

	// Flat
	if (item.type == SEL_FLOOR || item.type == SEL_CEILING) {
		// Get initial sector
		MapSector* sector = map.getSector(item.index);
		if (!sector) return;

		// Go through sector lines
		vector<MapLine*> lines;
		sector->getLines(lines);
		for (unsigned a = 0; a < lines.size(); a++) {
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
			if (item.type == SEL_FLOOR) {
				// Check sector floor height
				if (osector->intProperty("heightfloor") != sector->intProperty("heightfloor"))
					continue;

				// Check sector floor texture
				if (osector->floorTexture() != sector->floorTexture())
					continue;
			}
			else {
				// Check sector ceiling height
				if (osector->intProperty("heightceiling") != sector->intProperty("heightceiling"))
					continue;

				// Check sector ceiling texture
				if (osector->ceilingTexture() != sector->ceilingTexture())
					continue;
			}

			// Check flat isn't already selected
			bool selected = false;
			for (unsigned s = 0; s < selection_3d.size(); s++) {
				if (selection_3d[s].type == item.type && selection_3d[s].index == osector->getIndex()) {
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
	else if (item.type != SEL_THING) {
		vector<selection_3d_t> list;
		getAdjacentWalls3d(item, list);
		for (unsigned a = 0; a < list.size(); a++)
			selectItem3d(list[a], SELECT);
	}
}

void MapEditor::changeSectorLight3d(int amount) {
	// Get items to process
	vector<selection_3d_t> items;
	if (selection_3d.size() == 0 && hilight_3d.type != SEL_THING)
		items.push_back(hilight_3d);
	else {
		for (unsigned a = 0; a < selection_3d.size(); a++) {
			if (selection_3d[a].type != SEL_THING)
				items.push_back(selection_3d[a]);
		}
	}

	// Go through items
	for (unsigned a = 0; a < items.size(); a++) {
		// Wall
		if (items[a].type == SEL_SIDE_BOTTOM || items[a].type == SEL_SIDE_MIDDLE || items[a].type == SEL_SIDE_TOP) {
			// Get side
			MapSide* side = map.getSide(items[a].index);
			if (!side) continue;

			// Check for decrease when light = 255
			if (side->getSector()->getLight(0) == 255 && amount < -1)
				amount++;

			// Change sector light level
			side->getSector()->changeLight(amount);
		}

		// Flat
		if (items[a].type == SEL_FLOOR || items[a].type == SEL_CEILING) {
			// Get sector
			MapSector* s = map.getSector(items[a].index);

			// Change light level
			if (items[a].type == SEL_FLOOR && !link_3d_light)
				s->changeLight(amount, 1);
			else if (items[a].type == SEL_CEILING && !link_3d_light)
				s->changeLight(amount, 2);
			else {
				// Check for decrease when light = 255
				if (s->getLight(0) == 255 && amount < -1)
					amount++;

				s->changeLight(amount, 0);
			}
		}
	}

	// Editor message
	if (items.size() > 0) {
		if (amount > 0)
			addEditorMessage(S_FMT("Light increased by %d", amount));
		else
			addEditorMessage(S_FMT("Light decreased by %d", -amount));
	}
}

void MapEditor::changeWallOffset3d(int amount, bool x) {
	// Get items to process
	vector<selection_3d_t> items;
	if (selection_3d.size() == 0) {
		if (hilight_3d.type >= SEL_SIDE_TOP && hilight_3d.type <= SEL_SIDE_BOTTOM)
			items.push_back(hilight_3d);
	}
	else {
		for (unsigned a = 0; a < selection_3d.size(); a++) {
			if (selection_3d[a].type >= SEL_SIDE_TOP && selection_3d[a].type <= SEL_SIDE_BOTTOM)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.size() == 0)
		return;

	// Go through items
	vector<int> done;
	for (unsigned a = 0; a < items.size(); a++) {
		MapSide* side = map.getSide(items[a].index);

		// If offsets are linked, just change the whole side offset
		if (link_3d_offset) {
			// Check we haven't processed this side already
			bool d = false;
			for (unsigned b = 0; b < done.size(); b++) {
				if (done[b] == items[a].index) {
					d = true;
					break;
				}
			}
			if (d)
				continue;

			// Change the appropriate offset
			if (x) {
				int offset = side->intProperty("offsetx");
				side->setIntProperty("offsetx", offset + amount);
			}
			else {
				int offset = side->intProperty("offsety");
				side->setIntProperty("offsety", offset + amount);
			}

			// Add to done list
			done.push_back(items[a].index);
		}

		// Unlinked offsets
		else {
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
	}

	// Editor message
	if (items.size() > 0) {
		string axis = "X";
		if (!x) axis = "Y";

		if (amount > 0)
			addEditorMessage(S_FMT("%s offset increased by %d", CHR(axis), amount));
		else
			addEditorMessage(S_FMT("%s offset decreased by %d", CHR(axis), -amount));
	}
}

void MapEditor::changeSectorHeight3d(int amount) {
	// Get items to process
	vector<selection_3d_t> items;
	if (selection_3d.size() == 0 && hilight_3d.type != SEL_THING)
		items.push_back(hilight_3d);
	else {
		for (unsigned a = 0; a < selection_3d.size(); a++) {
			if (selection_3d[a].type != SEL_THING)
				items.push_back(selection_3d[a]);
		}
	}

	// Go through items
	vector<int> ceilings;
	for (unsigned a = 0; a < items.size(); a++) {
		// Wall (ceiling only for now)
		if (items[a].type == SEL_SIDE_BOTTOM || items[a].type == SEL_SIDE_MIDDLE || items[a].type == SEL_SIDE_TOP) {
			// Get sector
			MapSector* sector = map.getSide(items[a].index)->getSector();

			// Check this sector's ceiling hasn't already been changed
			bool done = false;
			int index = sector->getIndex();
			for (unsigned b = 0; b < ceilings.size(); b++) {
				if (ceilings[b] == index) {
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
			ceilings.push_back(index);
		}

		// Floor
		else if (items[a].type == SEL_FLOOR) {
			// Get sector
			MapSector* sector = map.getSector(items[a].index);

			// Change height
			int height = sector->intProperty("heightfloor");
			sector->setIntProperty("heightfloor", height + amount);
		}

		// Ceiling
		else if (items[a].type == SEL_CEILING) {
			// Get sector
			MapSector* sector = map.getSector(items[a].index);

			// Check this sector's ceiling hasn't already been changed
			bool done = false;
			int index = sector->getIndex();
			for (unsigned b = 0; b < ceilings.size(); b++) {
				if (ceilings[b] == index) {
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

	// Editor message
	if (items.size() > 0) {
		if (amount > 0)
			addEditorMessage(S_FMT("Height increased by %d", amount));
		else
			addEditorMessage(S_FMT("Height decreased by %d", -amount));
	}
}

void MapEditor::doAlignX3d(MapSide* side, int offset, string tex, vector<selection_3d_t>& walls_done) {
	// Check if this wall has already been processed
	for (unsigned a = 0; a < walls_done.size(); a++) {
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
	for (unsigned a = 0; a < vertex->nConnectedLines(); a++) {
		MapLine* l = vertex->connectedLine(a);
		
		// First side
		MapSide* s = l->s1();
		if (s) {
			// Check for matching texture
			if (s->stringProperty("texturetop") == tex || s->stringProperty("texturemiddle") == tex || s->stringProperty("texturebottom") == tex)
				doAlignX3d(s, offset + intlen, tex, walls_done);
		}

		// Second side
		s = l->s2();
		if (s) {
			// Check for matching texture
			if (s->stringProperty("texturetop") == tex || s->stringProperty("texturemiddle") == tex || s->stringProperty("texturebottom") == tex)
				doAlignX3d(s, offset + intlen, tex, walls_done);
		}
	}
}

void MapEditor::autoAlignX3d(selection_3d_t start) {
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

	// Do alignment
	doAlignX3d(side, side->intProperty("offsetx"), tex, walls_done);

	// Editor message
	addEditorMessage("Auto-aligned on X axis");
}

void MapEditor::resetWall3d() {
	// Get items to process
	vector<selection_3d_t> items;
	if (selection_3d.size() == 0) {
		if (hilight_3d.type == SEL_SIDE_TOP || hilight_3d.type == SEL_SIDE_BOTTOM || hilight_3d.type == SEL_SIDE_MIDDLE)
			items.push_back(hilight_3d);
	}
	else {
		for (unsigned a = 0; a < selection_3d.size(); a++) {
			if (selection_3d[a].type == SEL_SIDE_TOP || selection_3d[a].type == SEL_SIDE_BOTTOM || selection_3d[a].type == SEL_SIDE_MIDDLE)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.size() == 0)
		return;

	// Go through items
	for (unsigned a = 0; a < items.size(); a++) {
		MapSide* side = map.getSide(items[a].index);
		if (!side) continue;

		// Reset offsets
		if (link_3d_offset) {
			// If offsets are linked, reset base offsets
			side->setIntProperty("offsetx", 0);
			side->setIntProperty("offsety", 0);
		}
		else {
			// Otherwise, reset offsets for the current wall part
			if (items[a].type == SEL_SIDE_TOP) {
				side->setFloatProperty("offsetx_top", 0);
				side->setFloatProperty("offsety_top", 0);
			}
			else if (items[a].type == SEL_SIDE_MIDDLE) {
				side->setFloatProperty("offsetx_mid", 0);
				side->setFloatProperty("offsety_mid", 0);
			}
			else {
				side->setFloatProperty("offsetx_bottom", 0);
				side->setFloatProperty("offsety_bottom", 0);
			}
		}

		// Reset scaling
		if (theGameConfiguration->udmfNamespace() == "zdoom") {
			if (items[a].type == SEL_SIDE_TOP) {
				side->setFloatProperty("scalex_top", 1);
				side->setFloatProperty("scaley_top", 1);
			}
			else if (items[a].type == SEL_SIDE_MIDDLE) {
				side->setFloatProperty("scalex_mid", 1);
				side->setFloatProperty("scaley_mid", 1);
			}
			else {
				side->setFloatProperty("scalex_bottom", 1);
				side->setFloatProperty("scaley_bottom", 1);
			}
		}
	}

	// Editor message
	if (theGameConfiguration->udmfNamespace() == "zdoom")
		addEditorMessage("Offsets and scaling reset");
	else
		addEditorMessage("Offsets reset");
}

void MapEditor::toggleUnpegged3d(bool lower) {
	// Get items to process
	vector<selection_3d_t> items;
	if (selection_3d.size() == 0) {
		if (hilight_3d.type == SEL_SIDE_TOP || hilight_3d.type == SEL_SIDE_BOTTOM || hilight_3d.type == SEL_SIDE_MIDDLE)
			items.push_back(hilight_3d);
	}
	else {
		for (unsigned a = 0; a < selection_3d.size(); a++) {
			if (selection_3d[a].type == SEL_SIDE_TOP || selection_3d[a].type == SEL_SIDE_BOTTOM || selection_3d[a].type == SEL_SIDE_MIDDLE)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.size() == 0)
		return;

	// Go through items
	for (unsigned a = 0; a < items.size(); a++) {
		// Get line
		MapLine* line = map.getSide(items[a].index)->getParentLine();
		if (!line) continue;

		// Toggle flag
		if (lower) {
			bool unpegged = theGameConfiguration->lineBasicFlagSet("dontpegbottom", line, theMapEditor->currentMapDesc().format);
			theGameConfiguration->setLineBasicFlag("dontpegbottom", line, map.currentFormat(), !unpegged);
		}
		else {
			bool unpegged = theGameConfiguration->lineBasicFlagSet("dontpegtop", line, theMapEditor->currentMapDesc().format);
			theGameConfiguration->setLineBasicFlag("dontpegtop", line, map.currentFormat(), !unpegged);
		}
	}

	// Editor message
	if (lower)
		addEditorMessage("Lower Unpegged flag toggled");
	else
		addEditorMessage("Upper Unpegged flag toggled");
}

void MapEditor::copy3d(int type) {
	// Check hilight
	if (hilight_3d.index < 0)
		return;

	// Upper wall
	else if (hilight_3d.type == SEL_SIDE_TOP) {
		// Texture
		if (type == COPY_TEXTYPE)
			copy_texture = map.getSide(hilight_3d.index)->stringProperty("texturetop");
	}

	// Middle wall
	else if (hilight_3d.type == SEL_SIDE_MIDDLE) {
		// Texture
		if (type == COPY_TEXTYPE)
			copy_texture = map.getSide(hilight_3d.index)->stringProperty("texturemiddle");
	}

	// Lower wall
	else if (hilight_3d.type == SEL_SIDE_BOTTOM) {
		// Texture
		if (type == COPY_TEXTYPE)
			copy_texture = map.getSide(hilight_3d.index)->stringProperty("texturebottom");
	}

	// Floor
	else if (hilight_3d.type == SEL_FLOOR) {
		// Texture
		if (type == COPY_TEXTYPE)
			copy_texture = map.getSector(hilight_3d.index)->floorTexture();
	}

	// Ceiling
	else if (hilight_3d.type == SEL_CEILING) {
		// Texture
		if (type == COPY_TEXTYPE)
			copy_texture = map.getSector(hilight_3d.index)->ceilingTexture();
	}

	// Thing
	else if (hilight_3d.type == SEL_THING) {
		if (!copy_thing)
			copy_thing = new MapThing();

		copy_thing->copy(map.getThing(hilight_3d.index));
	}

	// Flash
	if (canvas)
		canvas->itemSelected3d(hilight_3d);

	// Editor message
	if (type == COPY_TEXTYPE) {
		if (hilight_3d.type == SEL_THING)
			addEditorMessage("Copied Thing Type");
		else
			addEditorMessage("Copied Texture");
	}
}

void MapEditor::paste3d(int type) {
	// Get items to paste to
	vector<selection_3d_t> items;
	if (selection_3d.size() == 0 && hilight_3d.index >= 0)
		items.push_back(hilight_3d);
	else if (selection_3d.size() > 0) {
		for (unsigned a = 0; a < selection_3d.size(); a++)
			items.push_back(selection_3d[a]);
	}
	else
		return;

	// Go through items
	for (unsigned a = 0; a < items.size(); a++) {
		// Upper wall
		if (items[a].type == SEL_SIDE_TOP) {
			// Texture
			if (type == COPY_TEXTYPE)
				map.getSide(items[a].index)->setStringProperty("texturetop", copy_texture);
		}

		// Middle wall
		if (items[a].type == SEL_SIDE_MIDDLE) {
			// Texture
			if (type == COPY_TEXTYPE)
				map.getSide(items[a].index)->setStringProperty("texturemiddle", copy_texture);
		}

		// Lower wall
		if (items[a].type == SEL_SIDE_BOTTOM) {
			// Texture
			if (type == COPY_TEXTYPE)
				map.getSide(items[a].index)->setStringProperty("texturebottom", copy_texture);
		}

		// Floor
		if (items[a].type == SEL_FLOOR) {
			// Texture
			if (type == COPY_TEXTYPE)
				map.getSector(items[a].index)->setStringProperty("texturefloor", copy_texture);
		}

		// Ceiling
		if (items[a].type == SEL_CEILING) {
			// Texture
			if (type == COPY_TEXTYPE)
				map.getSector(items[a].index)->setStringProperty("textureceiling", copy_texture);
		}

		// Thing
		if (items[a].type == SEL_THING) {
			// Type
			if (type == COPY_TEXTYPE)
				map.getThing(items[a].index)->setIntProperty("type", copy_thing->getType());
		}
	}

	// Editor message
	if (type == COPY_TEXTYPE) {
		if (hilight_3d.type == SEL_THING)
			addEditorMessage("Pasted Thing Type");
		else
			addEditorMessage("Pasted Texture");
	}
}

string MapEditor::getEditorMessage(int index) {
	// Check index
	if (index < 0 || index >= (int)editor_messages.size())
		return "";

	return editor_messages[index].message;
}

long MapEditor::getEditorMessageTime(int index) {
	// Check index
	if (index < 0 || index >= (int)editor_messages.size())
		return -1;

	return theApp->runTimer() - editor_messages[index].act_time;
}

void MapEditor::addEditorMessage(string message) {
	// Remove oldest message if there are too many active
	if (editor_messages.size() >= 4)
		editor_messages.erase(editor_messages.begin());

	// Add message to list
	editor_msg_t msg;
	msg.message = message;
	msg.act_time = theApp->runTimer();
	editor_messages.push_back(msg);
}

unsigned MapEditor::numEditorMessages() {
	return editor_messages.size();
}

string MapEditor::getModeString() {
	switch (edit_mode) {
	case MODE_VERTICES: return "Vertices";
	case MODE_LINES: return "Lines";
	case MODE_SECTORS: return "Sectors";
	case MODE_THINGS: return "Things";
	default: return "Items";
	};
}

bool MapEditor::handleKeyBind(string key, fpoint2_t position) {
	// --- General keybinds ---

	bool handled = true;
	if (edit_mode != MODE_3D) {
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
		else if (key == "me2d_clear_selection") {
			clearSelection();
			addEditorMessage("Selection cleared");
		}

		// Lock/unlock hilight
		else if (key == "me2d_lock_hilight") {
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
	if (key.StartsWith("me2d_sector") && edit_mode == MODE_SECTORS) {
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
	else if (key.StartsWith("me3d_") && edit_mode == MODE_3D) {
		// Check for extended udmf properties
		bool ext = false;
		if (theMapEditor->currentMapDesc().format == MAP_UDMF &&
			S_CMPNOCASE(theGameConfiguration->udmfNamespace(), "zdoom"))
			ext = true;

		// Clear selection
		if (key == "me3d_clear_selection") {
			clearSelection();
			addEditorMessage("Selection cleared");
		}

		// Toggle linked light levels
		else if (key == "me3d_light_toggle_link") {
			if (!ext)
				addEditorMessage("Unlinked light levels not supported in this game configuration");
			else {
				link_3d_light = !link_3d_light;
				if (link_3d_light)
					addEditorMessage("Flat light levels linked");
				else
					addEditorMessage("Flat light levels unlinked");
			}
		}

		// Toggle linked offsets
		else if (key == "me3d_wall_toggle_link_ofs") {
			if (!ext)
				addEditorMessage("Unlinked wall offsets not supported in this game configuration");
			else {
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

		// Wall offset changes
		else if	(key == "me3d_wall_xoff_up8")	changeWallOffset3d(8, true);
		else if	(key == "me3d_wall_xoff_up")	changeWallOffset3d(1, true);
		else if	(key == "me3d_wall_xoff_down8")	changeWallOffset3d(-8, true);
		else if	(key == "me3d_wall_xoff_down")	changeWallOffset3d(-1, true);
		else if	(key == "me3d_wall_yoff_up8")	changeWallOffset3d(8, false);
		else if	(key == "me3d_wall_yoff_up")	changeWallOffset3d(1, false);
		else if	(key == "me3d_wall_yoff_down8")	changeWallOffset3d(-8, false);
		else if	(key == "me3d_wall_yoff_down")	changeWallOffset3d(-1, false);

		// Height changes
		else if	(key == "me3d_flat_height_up8")		changeSectorHeight3d(8);
		else if	(key == "me3d_flat_height_up")		changeSectorHeight3d(1);
		else if	(key == "me3d_flat_height_down8")	changeSectorHeight3d(-8);
		else if	(key == "me3d_flat_height_down")	changeSectorHeight3d(-1);

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

		else
			return false;
	}

	return handled;
}

void MapEditor::updateDisplay() {
	// Update map object properties panel
	vector<MapObject*> selection;
	getSelectedObjects(selection);
	theMapEditor->propsPanel()->openObjects(selection);

	// Update canvas info overlay
	if (canvas) {
		canvas->updateInfoOverlay();
		canvas->Refresh();
	}
}




CONSOLE_COMMAND(m_show_item, 1) {
	int index = atoi(CHR(args[0]));
	theMapEditor->mapEditor().showItem(index);
}





// testing stuff

CONSOLE_COMMAND(m_test_sector, 0) {
	sf::Clock clock;
	SLADEMap& map = theMapEditor->mapEditor().getMap();
	for (unsigned a = 0; a < map.nThings(); a++)
		map.sectorAt(map.getThing(a)->xPos(), map.getThing(a)->yPos());
#if SFML_VERSION_MAJOR < 2	// SFML 1.6: uppercase G, returns time in seconds as a float
	long ms = clock.GetElapsedTime() * 1000;
#else						// SFML 2.0: lowercase G, returns a Time object
	long ms = clock.getElapsedTime().asMilliseconds();
#endif
	wxLogMessage("Took %dms", ms);
}

/*
CONSOLE_COMMAND(m_vertex_attached, 1) {
	MapVertex* vertex = theMapEditor->mapEditor().getMap().getVertex(atoi(CHR(args[0])));
	if (vertex) {
		wxLogMessage("Attached lines:");
		for (unsigned a = 0; a < vertex->nConnectedLines(); a++)
			wxLogMessage("Line #%d", vertex->connectedLine(a)->getIndex());
	}
}
*/

/*
CONSOLE_COMMAND(n_polys ,0) {
	SLADEMap& map = theMapEditor->mapEditor().getMap();
	int npoly = 0;
	for (unsigned a = 0; a < map.nSectors(); a++)
		npoly += map.getSector(a)->getPolygon()->nSubPolys();

	theConsole->logMessage(S_FMT("%d polygons total", npoly));
}
*/

/*
CONSOLE_COMMAND(m_test_save, 1) {
	vector<ArchiveEntry*> entries;
	theMapEditor->mapEditor().getMap().writeDoomMap(entries);
	WadArchive temp;
	temp.addNewEntry("MAP01");
	for (unsigned a = 0; a < entries.size(); a++)
		temp.addEntry(entries[a]);
	temp.save(args[0]);
	temp.close();
}
*/
