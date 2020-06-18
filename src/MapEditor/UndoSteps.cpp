
#include "Main.h"
#include "UndoSteps.h"
#include "SLADEMap/SLADEMap.h"

using namespace slade;
using namespace mapeditor;

PropertyChangeUS::PropertyChangeUS(MapObject* object) : backup_{ new MapObject::Backup() }
{
	object->backupTo(backup_.get());
}

void PropertyChangeUS::doSwap(MapObject* obj)
{
	auto temp = std::make_unique<MapObject::Backup>();
	obj->backupTo(temp.get());
	obj->loadFromBackup(backup_.get());
	backup_.swap(temp);
}

bool PropertyChangeUS::doUndo()
{
	auto obj = undoredo::currentMap()->mapData().getObjectById(backup_->id);
	if (obj)
		doSwap(obj);

	return true;
}

bool PropertyChangeUS::doRedo()
{
	auto obj = undoredo::currentMap()->mapData().getObjectById(backup_->id);
	if (obj)
		doSwap(obj);

	return true;
}


MapObjectCreateDeleteUS::MapObjectCreateDeleteUS()
{
	auto map = undoredo::currentMap();
	map->mapData().putObjectIdList(MapObject::Type::Vertex, vertices_);
	map->mapData().putObjectIdList(MapObject::Type::Line, lines_);
	map->mapData().putObjectIdList(MapObject::Type::Side, sides_);
	map->mapData().putObjectIdList(MapObject::Type::Sector, sectors_);
	map->mapData().putObjectIdList(MapObject::Type::Thing, things_);
}

void MapObjectCreateDeleteUS::swapLists()
{
	// Backup
	vector<unsigned> vertices;
	vector<unsigned> lines;
	vector<unsigned> sides;
	vector<unsigned> sectors;
	vector<unsigned> things;
	auto             map = undoredo::currentMap();
	if (isValid(vertices_))
		map->mapData().putObjectIdList(MapObject::Type::Vertex, vertices);
	if (isValid(lines_))
		map->mapData().putObjectIdList(MapObject::Type::Line, lines);
	if (isValid(sides_))
		map->mapData().putObjectIdList(MapObject::Type::Side, sides);
	if (isValid(sectors_))
		map->mapData().putObjectIdList(MapObject::Type::Sector, sectors);
	if (isValid(things_))
		map->mapData().putObjectIdList(MapObject::Type::Thing, things);

	// Restore
	if (isValid(vertices_))
	{
		map->restoreObjectIdList(MapObject::Type::Vertex, vertices_);
		vertices_ = vertices;
		map->updateGeometryInfo(0);
	}
	if (isValid(lines_))
	{
		map->restoreObjectIdList(MapObject::Type::Line, lines_);
		lines_ = lines;
		map->updateGeometryInfo(0);
	}
	if (isValid(sides_))
	{
		map->restoreObjectIdList(MapObject::Type::Side, sides_);
		sides_ = sides;
	}
	if (isValid(sectors_))
	{
		map->restoreObjectIdList(MapObject::Type::Sector, sectors_);
		sectors_ = sectors;
	}
	if (isValid(things_))
	{
		map->restoreObjectIdList(MapObject::Type::Thing, things_);
		things_ = things;
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
	auto map = undoredo::currentMap();

	// Check vertices changed
	bool vertices_changed = false;
	if (map->nVertices() != vertices_.size())
		vertices_changed = true;
	else
		for (unsigned a = 0; a < map->nVertices(); a++)
			if (map->vertex(a)->objId() != vertices_[a])
			{
				vertices_changed = true;
				break;
			}
	if (!vertices_changed)
	{
		// No change, clear
		vertices_.clear();
		vertices_.push_back(0);
		log::info(3, "MapObjectCreateDeleteUS: No vertices added/deleted");
	}

	// Check lines changed
	bool lines_changed = false;
	if (map->nLines() != lines_.size())
		lines_changed = true;
	else
		for (unsigned a = 0; a < map->nLines(); a++)
			if (map->line(a)->objId() != lines_[a])
			{
				lines_changed = true;
				break;
			}
	if (!lines_changed)
	{
		// No change, clear
		lines_.clear();
		lines_.push_back(0);
		log::info(3, "MapObjectCreateDeleteUS: No lines added/deleted");
	}

	// Check sides changed
	bool sides_changed = false;
	if (map->nSides() != sides_.size())
		sides_changed = true;
	else
		for (unsigned a = 0; a < map->nSides(); a++)
			if (map->side(a)->objId() != sides_[a])
			{
				sides_changed = true;
				break;
			}
	if (!sides_changed)
	{
		// No change, clear
		sides_.clear();
		sides_.push_back(0);
		log::info(3, "MapObjectCreateDeleteUS: No sides added/deleted");
	}

	// Check sectors changed
	bool sectors_changed = false;
	if (map->nSectors() != sectors_.size())
		sectors_changed = true;
	else
		for (unsigned a = 0; a < map->nSectors(); a++)
			if (map->sector(a)->objId() != sectors_[a])
			{
				sectors_changed = true;
				break;
			}
	if (!sectors_changed)
	{
		// No change, clear
		sectors_.clear();
		sectors_.push_back(0);
		log::info(3, "MapObjectCreateDeleteUS: No sectors added/deleted");
	}

	// Check things changed
	bool things_changed = false;
	if (map->nThings() != things_.size())
		things_changed = true;
	else
		for (unsigned a = 0; a < map->nThings(); a++)
			if (map->thing(a)->objId() != things_[a])
			{
				things_changed = true;
				break;
			}
	if (!things_changed)
	{
		// No change, clear
		things_.clear();
		things_.push_back(0);
		log::info(3, "MapObjectCreateDeleteUS: No things added/deleted");
	}
}

bool MapObjectCreateDeleteUS::isOk()
{
	// Check for any changes at all
	return !(
		vertices_.size() == 1 && vertices_[0] == 0 && lines_.size() == 1 && lines_[0] == 0 && sides_.size() == 1
		&& sides_[0] == 0 && sectors_.size() == 1 && sectors_[0] == 0 && things_.size() == 1 && things_[0] == 0);
}



MultiMapObjectPropertyChangeUS::MultiMapObjectPropertyChangeUS()
{
	// Get backups of recently modified map objects
	auto objects = undoredo::currentMap()->mapData().allModifiedObjects(MapObject::propBackupTime());
	for (auto& object : objects)
	{
		auto bak = object->backup(true);
		if (bak)
		{
			backups_.emplace_back(bak);
			// log::info(1, "%s #%d modified", objects[a]->getTypeName(), objects[a]->getIndex());
		}
	}

	if (log::verbosity() >= 2)
	{
		string msg = "Modified ids: ";
		for (auto& backup : backups_)
			msg += fmt::format("{}, ", backup->id);
		log::info(msg);
	}
}

void MultiMapObjectPropertyChangeUS::doSwap(MapObject* obj, unsigned index)
{
	auto temp = std::make_unique<MapObject::Backup>();
	obj->backupTo(temp.get());
	obj->loadFromBackup(backups_[index].get());
	backups_[index].swap(temp);
}

bool MultiMapObjectPropertyChangeUS::doUndo()
{
	for (unsigned a = 0; a < backups_.size(); a++)
	{
		auto obj = undoredo::currentMap()->mapData().getObjectById(backups_[a]->id);
		if (obj)
			doSwap(obj, a);
	}

	return true;
}

bool MultiMapObjectPropertyChangeUS::doRedo()
{
	// log::info(2, "Restore %lu objects", backups.size());
	for (unsigned a = 0; a < backups_.size(); a++)
	{
		auto obj = undoredo::currentMap()->mapData().getObjectById(backups_[a]->id);
		if (obj)
			doSwap(obj, a);
	}

	return true;
}
