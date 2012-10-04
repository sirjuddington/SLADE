
#ifndef __UNDO_REDO_H__
#define __UNDO_REDO_H__

#include "ListenerAnnouncer.h"

class UndoStep {
private:

public:
	UndoStep() {}
	virtual ~UndoStep() {}
	
	virtual bool	doUndo() { return true; }
	virtual bool	doRedo() { return true; }
	virtual bool	writeFile(MemChunk& mc) { return true; }
	virtual bool	readFile(MemChunk& mc) { return true; }
};

class UndoLevel {
private:
	string				name;
	vector<UndoStep*>	undo_steps;
	
public:
	UndoLevel(string name);
	~UndoLevel();

	string	getName() { return name; }
	bool	doUndo();
	bool	doRedo();
	void	addStep(UndoStep* step) { undo_steps.push_back(step); }
	
	bool	writeFile(string filename);
	bool	readFile(string filename);
};

class UndoManager : public Announcer {
private:
	vector<UndoLevel*>	undo_levels;
	UndoLevel*			current_level;
	int					current_level_index;
	bool				undo_running;

public:
	UndoManager();
	~UndoManager();

	void	getAllLevels(vector<string>& list);
	int		getCurrentIndex() { return current_level_index; }

	void	beginRecord(string name);
	void	endRecord(bool success);
	bool	currentlyRecording();
	bool	recordUndoStep(UndoStep* step);
	void	undo();
	void	redo();
};

namespace UndoRedo {
	bool			currentlyRecording();
	UndoManager*	currentManager();
}

#endif//__UNDO_REDO_H__
