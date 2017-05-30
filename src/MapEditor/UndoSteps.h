#pragma once

#include "General/UndoRedo.h"

class MapObject;
struct mobj_backup_t;

namespace MapEditor
{
 	// UndoStep for when a MapObject has properties changed
	class PropertyChangeUS : public UndoStep
	{
	public:
		PropertyChangeUS(MapObject* object);
		~PropertyChangeUS();

		void doSwap(MapObject* obj);
		bool doUndo();
		bool doRedo();

	private:
		mobj_backup_t*	backup;
	};

 	// UndoStep for when a MapObject is either created or deleted
	class MapObjectCreateDeleteUS : public UndoStep
	{
	public:
		MapObjectCreateDeleteUS();
		~MapObjectCreateDeleteUS() {}

		bool isValid(vector<unsigned>& list) { return !(list.size() == 1 && list[0] == 0); }
		void swapLists();
		bool doUndo();
		bool doRedo();
		void checkChanges();
		bool isOk();

	private:
		vector<unsigned>	vertices;
		vector<unsigned>	lines;
		vector<unsigned>	sides;
		vector<unsigned>	sectors;
		vector<unsigned>	things;
	};

	// UndoStep for when multiple MapObjects have properties changed
	class MultiMapObjectPropertyChangeUS : public UndoStep
	{
	public:
		MultiMapObjectPropertyChangeUS();
		~MultiMapObjectPropertyChangeUS();

		void doSwap(MapObject* obj, unsigned index);
		bool doUndo();
		bool doRedo();
		bool isOk() { return !backups.empty(); }

	private:
		vector<mobj_backup_t*>	backups;
	};
}
