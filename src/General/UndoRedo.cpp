
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    UndoRedo.cpp
// Description: Classes for the Undo/Redo system
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
#include "General/UndoRedo.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
UndoManager* current_undo_manager = nullptr;
} // namespace


// -----------------------------------------------------------------------------
//
// UndoLevel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// UndoLevel class constructor
// -----------------------------------------------------------------------------
UndoLevel::UndoLevel(const string& name) : name_{ name }, timestamp_{ wxDateTime::Now() } {}

// -----------------------------------------------------------------------------
// UndoLevel class destructor
// -----------------------------------------------------------------------------
UndoLevel::~UndoLevel()
{
	for (auto& undo_step : undo_steps_)
		delete undo_step;
}

// -----------------------------------------------------------------------------
// Returns a string representation of the time at which the undo level was
// recorded
// -----------------------------------------------------------------------------
string UndoLevel::timeStamp(bool date, bool time) const
{
	if (date && !time)
		return timestamp_.FormatISODate();
	else if (!date && time)
		return timestamp_.FormatISOTime();
	else
		return timestamp_.FormatISOCombined();
}

// -----------------------------------------------------------------------------
// Performs all undo steps for this level
// -----------------------------------------------------------------------------
bool UndoLevel::doUndo()
{
	LOG_MESSAGE(3, "Performing undo \"%s\" (%lu steps)", name_, undo_steps_.size());
	bool ok = true;
	for (int a = (int)undo_steps_.size() - 1; a >= 0; a--)
	{
		if (!undo_steps_[a]->doUndo())
			ok = false;
	}

	return ok;
}

// -----------------------------------------------------------------------------
// Performs all redo steps for this level
// -----------------------------------------------------------------------------
bool UndoLevel::doRedo()
{
	LOG_MESSAGE(3, "Performing redo \"%s\" (%lu steps)", name_, undo_steps_.size());
	bool ok = true;
	for (auto& undo_step : undo_steps_)
	{
		if (!undo_step->doRedo())
			ok = false;
	}

	return ok;
}

// -----------------------------------------------------------------------------
// Reads the undo level from a file
// -----------------------------------------------------------------------------
bool UndoLevel::readFile(const string& filename) const
{
	return true;
}

// -----------------------------------------------------------------------------
// Writes the undo level to a file
// -----------------------------------------------------------------------------
bool UndoLevel::writeFile(const string& filename) const
{
	return true;
}

// -----------------------------------------------------------------------------
// Adds all undo steps from all undo levels in [levels]
// -----------------------------------------------------------------------------
void UndoLevel::createMerged(vector<UndoLevel*>& levels)
{
	for (auto& level : levels)
	{
		for (auto undo_step : level->undo_steps_)
			undo_steps_.push_back(undo_step);
		level->undo_steps_.clear();
	}
}


// -----------------------------------------------------------------------------
//
// UndoManager Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// UndoManager class destructor
// -----------------------------------------------------------------------------
UndoManager::~UndoManager()
{
	for (auto& undo_level : undo_levels_)
		delete undo_level;
}

// -----------------------------------------------------------------------------
// Begins 'recording' a new undo level
// -----------------------------------------------------------------------------
void UndoManager::beginRecord(const string& name)
{
	// Can't if currently in an undo/redo operation
	if (undo_running_)
		return;

	// Set this as the current undo manager
	current_undo_manager = this;

	// End current recording if any
	if (current_level_)
		endRecord(true);

	// Begin new UndoLevel
	// LOG_MESSAGE(1, "Recording undo level \"%s\"", name);
	current_level_ = new UndoLevel(name);
}

// -----------------------------------------------------------------------------
// Finishes 'recording' the current undo level and adds it
// -----------------------------------------------------------------------------
void UndoManager::endRecord(bool success)
{
	// Do nothing if not currently recording or in an undo/redo operation
	if (!current_level_ || undo_running_)
		return;

	// If failed, delete current undo level
	if (!success)
	{
		// LOG_MESSAGE(1, "Recording undo level \"%s\" failed", current_level->getName());
		delete current_level_;
		current_level_ = nullptr;
		return;
	}

	// Remove any undo levels after the current
	while ((int)undo_levels_.size() - 1 > current_level_index_)
	{
		// LOG_MESSAGE(1, "Removing undo level \"%s\"", undo_levels.back()->getName());
		delete undo_levels_.back();
		undo_levels_.pop_back();
	}

	// Add current level to levels
	// LOG_MESSAGE(1, "Recording undo level \"%s\" succeeded", current_level->getName());
	undo_levels_.push_back(current_level_);
	current_level_       = nullptr;
	current_level_index_ = undo_levels_.size() - 1;

	// Clear current undo manager
	current_undo_manager = nullptr;

	announce("level_recorded");
}

// -----------------------------------------------------------------------------
// Returns true if this manager is currently recording an undo level
// -----------------------------------------------------------------------------
bool UndoManager::currentlyRecording() const
{
	return (current_level_ != nullptr);
}

// -----------------------------------------------------------------------------
// Records the UndoStep [step] to the current undo level, if it is currently
// being recorded. Returns false if not currently recording
// -----------------------------------------------------------------------------
bool UndoManager::recordUndoStep(UndoStep* step) const
{
	// Do nothing if not recording or step not given
	if (!step)
		return false;
	else if (!current_level_)
	{
		delete step;
		return false;
	}

	// Add step to current undo level
	current_level_->addStep(step);

	return step->isOk();
}

// -----------------------------------------------------------------------------
// Performs an undo operation
// -----------------------------------------------------------------------------
string UndoManager::undo()
{
	// Can't while currently recording
	if (current_level_)
		return "";

	// Can't if no more levels to undo
	if (current_level_index_ < 0)
		return "";

	// Perform undo level
	undo_running_        = true;
	current_undo_manager = this;
	UndoLevel* level     = undo_levels_[current_level_index_];
	if (!level->doUndo())
		LOG_MESSAGE(3, "Undo operation \"%s\" failed", level->name());
	undo_running_        = false;
	current_undo_manager = nullptr;
	current_level_index_--;

	announce("undo");

	return level->name();
}

// -----------------------------------------------------------------------------
// Performs a redo operation
// -----------------------------------------------------------------------------
string UndoManager::redo()
{
	// Can't while currently recording
	if (current_level_)
		return "";

	// Can't if no more levels to redo
	if (current_level_index_ == (int)undo_levels_.size() - 1 || undo_levels_.empty())
		return "";

	// Perform redo level
	current_level_index_++;
	undo_running_        = true;
	current_undo_manager = this;
	UndoLevel* level     = undo_levels_[current_level_index_];
	level->doRedo();
	undo_running_        = false;
	current_undo_manager = nullptr;

	announce("redo");

	return level->name();
}

// -----------------------------------------------------------------------------
// Adds all undo level names to [list]
// -----------------------------------------------------------------------------
void UndoManager::putAllLevels(vector<string>& list)
{
	for (auto& undo_level : undo_levels_)
		list.push_back(undo_level->name());
}

// -----------------------------------------------------------------------------
// Clears all undo levels up to the last reset point
// -----------------------------------------------------------------------------
void UndoManager::clearToResetPoint()
{
	while (current_level_index_ > reset_point_)
	{
		undo_levels_.pop_back();
		current_level_index_--;
	}

	current_level_ = nullptr;
	undo_running_  = false;
}

// -----------------------------------------------------------------------------
// Clears all undo levels and resets variables
// -----------------------------------------------------------------------------
void UndoManager::clear()
{
	// Clean up undo levels
	for (auto& undo_level : undo_levels_)
		delete undo_level;

	// Reset
	undo_levels_.clear();
	current_level_       = nullptr;
	current_level_index_ = -1;
	undo_running_        = false;
}

// -----------------------------------------------------------------------------
// Creates an undo level from all levels in [manager], called [name]
// -----------------------------------------------------------------------------
bool UndoManager::createMergedLevel(UndoManager* manager, const string& name)
{
	// Do nothing if no levels to merge
	if (manager->undo_levels_.empty())
		return false;

	// Create merged undo level from manager
	auto merged = new UndoLevel(name);
	merged->createMerged(manager->undo_levels_);

	// Add undo level
	undo_levels_.push_back(merged);
	current_level_       = nullptr;
	current_level_index_ = undo_levels_.size() - 1;

	return true;
}


// -----------------------------------------------------------------------------
//
// UndoRedo Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if the current undo manager is currently recording an undo level
// -----------------------------------------------------------------------------
bool UndoRedo::currentlyRecording()
{
	if (current_undo_manager)
		return current_undo_manager->currentlyRecording();
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns the 'current' undo manager, this is usually the manager that is
// currently recording an undo level
// -----------------------------------------------------------------------------
UndoManager* UndoRedo::currentManager()
{
	return current_undo_manager;
}

// -----------------------------------------------------------------------------
// Returns the 'current' map, associated with the current undo manager
// -----------------------------------------------------------------------------
SLADEMap* UndoRedo::currentMap()
{
	if (current_undo_manager)
		return current_undo_manager->map();
	else
		return nullptr;
}
