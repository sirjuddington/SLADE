
#ifndef __UNDO_REDO_H__
#define __UNDO_REDO_H__

#include "ListenerAnnouncer.h"
#include <wx/datetime.h>

class UndoStep
{
private:

public:
	UndoStep() {}
	virtual ~UndoStep() {}

	virtual bool	doUndo() { return true; }
	virtual bool	doRedo() { return true; }
	virtual bool	writeFile(MemChunk& mc) { return true; }
	virtual bool	readFile(MemChunk& mc) { return true; }
	virtual bool	isOk() { return true; }
};

class UndoLevel
{
private:
	string				name;
	vector<UndoStep*>	undo_steps;
	wxDateTime			timestamp;

public:
	UndoLevel(string name);
	~UndoLevel();

	string	getName() { return name; }
	bool	doUndo();
	bool	doRedo();
	void	addStep(UndoStep* step) { undo_steps.push_back(step); }
	string	getTimeStamp(bool date, bool time);

	bool	writeFile(string filename);
	bool	readFile(string filename);
};

class SLADEMap;
class UndoManager : public Announcer
{
private:
	vector<UndoLevel*>	undo_levels;
	UndoLevel*			current_level;
	int					current_level_index;
	bool				undo_running;
	SLADEMap*			map;

public:
	UndoManager(SLADEMap* map = NULL);
	~UndoManager();

	SLADEMap*	getMap() { return map; }
	void		getAllLevels(vector<string>& list);
	int			getCurrentIndex() { return current_level_index; }
	unsigned	nUndoLevels() { return undo_levels.size(); }
	UndoLevel*	undoLevel(unsigned index) { return undo_levels[index]; }

	void	beginRecord(string name);
	void	endRecord(bool success);
	bool	currentlyRecording();
	bool	recordUndoStep(UndoStep* step);
	string	undo();
	string	redo();

	void	clear();
};

namespace UndoRedo
{
	bool			currentlyRecording();
	UndoManager*	currentManager();
	SLADEMap*		currentMap();
}

#endif//__UNDO_REDO_H__
