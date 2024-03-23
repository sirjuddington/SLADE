#pragma once

namespace slade
{
class UndoStep
{
public:
	UndoStep()          = default;
	virtual ~UndoStep() = default;

	virtual bool doUndo() { return true; }
	virtual bool doRedo() { return true; }
	virtual bool writeFile(MemChunk& mc) { return true; }
	virtual bool readFile(MemChunk& mc) { return true; }
	virtual bool isOk() { return true; }
};

class UndoLevel
{
public:
	UndoLevel(string_view name);
	~UndoLevel() = default;

	string name() const { return name_; }
	bool   doUndo() const;
	bool   doRedo() const;
	void   addStep(unique_ptr<UndoStep> step) { undo_steps_.push_back(std::move(step)); }
	string timeStamp(bool date, bool time) const;

	bool writeFile(string_view filename) const;
	bool readFile(string_view filename) const;
	void createMerged(const vector<unique_ptr<UndoLevel>>& levels);

private:
	string                       name_;
	vector<unique_ptr<UndoStep>> undo_steps_;
	wxDateTime                   timestamp_;
};

class SLADEMap;
class UndoManager
{
public:
	UndoManager(SLADEMap* map = nullptr) : map_{ map } {}
	~UndoManager() = default;

	SLADEMap*  map() const { return map_; }
	void       putAllLevels(vector<string>& list) const;
	int        currentIndex() const { return current_level_index_; }
	unsigned   nUndoLevels() const { return undo_levels_.size(); }
	UndoLevel* undoLevel(unsigned index) const { return undo_levels_[index].get(); }

	void   beginRecord(string_view name);
	void   endRecord(bool success);
	bool   currentlyRecording() const;
	bool   recordUndoStep(unique_ptr<UndoStep> step) const;
	string undo();
	string redo();
	void   setResetPoint() { reset_point_ = current_level_index_; }
	void   clearToResetPoint();

	void clear();
	bool createMergedLevel(UndoManager* manager, string_view name);

	// Signals
	struct Signals
	{
		sigslot::signal<> level_recorded;
		sigslot::signal<> undo;
		sigslot::signal<> redo;
	};
	Signals& signals() { return signals_; }

private:
	vector<unique_ptr<UndoLevel>> undo_levels_;
	unique_ptr<UndoLevel>         current_level_;
	int                           current_level_index_ = -1;
	int                           reset_point_         = -1;
	bool                          undo_running_        = false;
	SLADEMap*                     map_                 = nullptr;
	Signals                       signals_;
};

namespace undoredo
{
	bool         currentlyRecording();
	UndoManager* currentManager();
	SLADEMap*    currentMap();
} // namespace undoredo
} // namespace slade
