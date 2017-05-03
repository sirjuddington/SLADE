
#include "Main.h"
#include "SLADEMap/SLADEMap.h"
#include "UndoSteps.h"

using namespace MapEditor;

PropertyChangeUS::PropertyChangeUS(MapObject* object)
{
	backup = new mobj_backup_t();
	object->backup(backup);
}

PropertyChangeUS::~PropertyChangeUS()
{
	delete backup;
}

void PropertyChangeUS::doSwap(MapObject* obj)
{
	mobj_backup_t* temp = new mobj_backup_t();
	obj->backup(temp);
	obj->loadFromBackup(backup);
	delete backup;
	backup = temp;
}

bool PropertyChangeUS::doUndo()
{
	MapObject* obj = UndoRedo::currentMap()->getObjectById(backup->id);
	if (obj) doSwap(obj);

	return true;
}

bool PropertyChangeUS::doRedo()
{
	MapObject* obj = UndoRedo::currentMap()->getObjectById(backup->id);
	if (obj) doSwap(obj);

	return true;
}


MapObjectCreateDeleteUS::MapObjectCreateDeleteUS()
{
	SLADEMap* map = UndoRedo::currentMap();
	map->getObjectIdList(MOBJ_VERTEX, vertices);
	map->getObjectIdList(MOBJ_LINE, lines);
	map->getObjectIdList(MOBJ_SIDE, sides);
	map->getObjectIdList(MOBJ_SECTOR, sectors);
	map->getObjectIdList(MOBJ_THING, things);
}

void MapObjectCreateDeleteUS::swapLists()
{
	// Backup
	vector<unsigned> vertices;
	vector<unsigned> lines;
	vector<unsigned> sides;
	vector<unsigned> sectors;
	vector<unsigned> things;
	SLADEMap* map = UndoRedo::currentMap();
	if (isValid(this->vertices))	map->getObjectIdList(MOBJ_VERTEX, vertices);
	if (isValid(this->lines))		map->getObjectIdList(MOBJ_LINE, lines);
	if (isValid(this->sides))		map->getObjectIdList(MOBJ_SIDE, sides);
	if (isValid(this->sectors))		map->getObjectIdList(MOBJ_SECTOR, sectors);
	if (isValid(this->things))		map->getObjectIdList(MOBJ_THING, things);

	// Restore
	if (isValid(this->vertices))
	{
		map->restoreObjectIdList(MOBJ_VERTEX, this->vertices);
		this->vertices = vertices;
		map->updateGeometryInfo(0);
	}
	if (isValid(this->lines))
	{
		map->restoreObjectIdList(MOBJ_LINE, this->lines);
		this->lines = lines;
		map->updateGeometryInfo(0);
	}
	if (isValid(this->sides))
	{
		map->restoreObjectIdList(MOBJ_SIDE, this->sides);
		this->sides = sides;
	}
	if (isValid(this->sectors))
	{
		map->restoreObjectIdList(MOBJ_SECTOR, this->sectors);
		this->sectors = sectors;
	}
	if (isValid(this->things))
	{
		map->restoreObjectIdList(MOBJ_THING, this->things);
		this->things = things;
	}
}

bool MapObjectCreateDeleteUS::doUndo()
{
	swapLists();
	return true;
}

bool MapObjectCreateDeleteUS::doRedo()
{
	swapLists();
	return true;
}

void MapObjectCreateDeleteUS::checkChanges()
{
	SLADEMap* map = UndoRedo::currentMap();

	// Check vertices changed
	bool vertices_changed = false;
	if (map->nVertices() != vertices.size())
		vertices_changed = true;
	else
		for (unsigned a = 0; a < map->nVertices(); a++)
			if (map->getVertex(a)->getId() != vertices[a])
			{
				vertices_changed = true;
				break;
			}
	if (!vertices_changed)
	{
		// No change, clear
		vertices.clear();
		vertices.push_back(0);
		LOG_MESSAGE(3, "MapObjectCreateDeleteUS: No vertices added/deleted");
	}

	// Check lines changed
	bool lines_changed = false;
	if (map->nLines() != lines.size())
		lines_changed = true;
	else
		for (unsigned a = 0; a < map->nLines(); a++)
			if (map->getLine(a)->getId() != lines[a])
			{
				lines_changed = true;
				break;
			}
	if (!lines_changed)
	{
		// No change, clear
		lines.clear();
		lines.push_back(0);
		LOG_MESSAGE(3, "MapObjectCreateDeleteUS: No lines added/deleted");
	}

	// Check sides changed
	bool sides_changed = false;
	if (map->nSides() != sides.size())
		sides_changed = true;
	else
		for (unsigned a = 0; a < map->nSides(); a++)
			if (map->getSide(a)->getId() != sides[a])
			{
				sides_changed = true;
				break;
			}
	if (!sides_changed)
	{
		// No change, clear
		sides.clear();
		sides.push_back(0);
		LOG_MESSAGE(3, "MapObjectCreateDeleteUS: No sides added/deleted");
	}

	// Check sectors changed
	bool sectors_changed = false;
	if (map->nSectors() != sectors.size())
		sectors_changed = true;
	else
		for (unsigned a = 0; a < map->nSectors(); a++)
			if (map->getSector(a)->getId() != sectors[a])
			{
				sectors_changed = true;
				break;
			}
	if (!sectors_changed)
	{
		// No change, clear
		sectors.clear();
		sectors.push_back(0);
		LOG_MESSAGE(3, "MapObjectCreateDeleteUS: No sectors added/deleted");
	}

	// Check things changed
	bool things_changed = false;
	if (map->nThings() != things.size())
		things_changed = true;
	else
		for (unsigned a = 0; a < map->nThings(); a++)
			if (map->getThing(a)->getId() != things[a])
			{
				things_changed = true;
				break;
			}
	if (!things_changed)
	{
		// No change, clear
		things.clear();
		things.push_back(0);
		LOG_MESSAGE(3, "MapObjectCreateDeleteUS: No things added/deleted");
	}
}

bool MapObjectCreateDeleteUS::isOk()
{
	// Check for any changes at all
	if (vertices.size() == 1 && vertices[0] == 0 &&
		lines.size() == 1 && lines[0] == 0 &&
		sides.size() == 1 && sides[0] == 0 &&
		sectors.size() == 1 && sectors[0] == 0 &&
		things.size() == 1 && things[0] == 0)
		return false;

	return true;
}



MultiMapObjectPropertyChangeUS::MultiMapObjectPropertyChangeUS()
{
	// Get backups of recently modified map objects
	vector<MapObject*> objects = UndoRedo::currentMap()->getAllModifiedObjects(MapObject::propBackupTime());
	for (unsigned a = 0; a < objects.size(); a++)
	{
		mobj_backup_t* bak = objects[a]->getBackup(true);
		if (bak)
		{
			backups.push_back(bak);
			//LOG_MESSAGE(1, "%s #%d modified", objects[a]->getTypeName(), objects[a]->getIndex());
		}
	}

	if (Log::verbosity() >= 2)
	{
		string msg = "Modified ids: ";
		for (unsigned a = 0; a < backups.size(); a++)
			msg += S_FMT("%d, ", backups[a]->id);
		Log::info(msg);
	}
}

MultiMapObjectPropertyChangeUS::~MultiMapObjectPropertyChangeUS()
{
	for (unsigned a = 0; a < backups.size(); a++)
		delete backups[a];
}

void MultiMapObjectPropertyChangeUS::doSwap(MapObject* obj, unsigned index)
{
	mobj_backup_t* temp = new mobj_backup_t();
	obj->backup(temp);
	obj->loadFromBackup(backups[index]);
	delete backups[index];
	backups[index] = temp;
}

bool MultiMapObjectPropertyChangeUS::doUndo()
{
	for (unsigned a = 0; a < backups.size(); a++)
	{
		MapObject* obj = UndoRedo::currentMap()->getObjectById(backups[a]->id);
		if (obj) doSwap(obj, a);
	}

	return true;
}

bool MultiMapObjectPropertyChangeUS::doRedo()
{
	//LOG_MESSAGE(2, "Restore %lu objects", backups.size());
	for (unsigned a = 0; a < backups.size(); a++)
	{
		MapObject* obj = UndoRedo::currentMap()->getObjectById(backups[a]->id);
		if (obj) doSwap(obj, a);
	}

	return true;
}
