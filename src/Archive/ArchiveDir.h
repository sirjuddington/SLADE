#pragma once

#include "ArchiveEntry.h"

class ArchiveDir
{
	friend class Archive;

public:
	ArchiveDir(string_view name, const shared_ptr<ArchiveDir>& parent = nullptr, Archive* archive = nullptr);
	~ArchiveDir() = default;

	// Accessors
	Archive*                                archive() const { return archive_; }
	const vector<shared_ptr<ArchiveEntry>>& entries() const { return entries_; }
	const vector<shared_ptr<ArchiveDir>>&   subdirs() const { return subdirs_; }
	ArchiveEntry*                           dirEntry() const { return dir_entry_.get(); }
	const string&                           name() const;
	string                                  path(bool include_name = true) const;
	shared_ptr<ArchiveDir>                  parent() const { return parent_dir_.lock(); }

	// Mutators
	void setName(string_view new_name) const { dir_entry_->setName(new_name); }
	void setArchive(Archive* archive);

	// Entry Access
	ArchiveEntry*                    entryAt(unsigned index) const;
	shared_ptr<ArchiveEntry>         sharedEntryAt(unsigned index) const;
	ArchiveEntry*                    entry(string_view name, bool cut_ext = false) const;
	shared_ptr<ArchiveEntry>         sharedEntry(string_view name, bool cut_ext = false) const;
	shared_ptr<ArchiveEntry>         sharedEntry(ArchiveEntry* entry) const;
	unsigned                         numEntries(bool inc_subdirs = false) const;
	int                              entryIndex(ArchiveEntry* entry, size_t startfrom = 0) const;
	vector<shared_ptr<ArchiveEntry>> allEntries() const;

	// Entry Operations
	bool addEntry(shared_ptr<ArchiveEntry> entry, unsigned index = 0xFFFFFFFF);
	bool removeEntry(unsigned index);
	bool swapEntries(unsigned index1, unsigned index2);

	// Subdirs
	unsigned               numSubdirs() const { return subdirs_.size(); }
	shared_ptr<ArchiveDir> subdir(string_view name);
	shared_ptr<ArchiveDir> subdirAt(unsigned index);
	bool                   addSubdir(shared_ptr<ArchiveDir> subdir, unsigned index = -1);
	shared_ptr<ArchiveDir> removeSubdir(string_view name);
	shared_ptr<ArchiveDir> removeSubdirAt(unsigned index);

	// Other
	void                   clear();
	shared_ptr<ArchiveDir> clone(shared_ptr<ArchiveDir> parent = nullptr);
	bool                   exportTo(string_view path);
	void                   allowDuplicateNames(bool allow) { allow_duplicate_names_ = allow; }

	// Static utility functions
	static shared_ptr<ArchiveDir>   subdirAtPath(const shared_ptr<ArchiveDir>& root, string_view path);
	static ArchiveDir*              subdirAtPath(ArchiveDir* root, string_view path);
	static shared_ptr<ArchiveEntry> entryAtPath(const shared_ptr<ArchiveDir>& root, string_view path);
	static bool                     merge(
							shared_ptr<ArchiveDir>& target,
							ArchiveDir*             dir,
							unsigned                position = -1,
							ArchiveEntry::State     state    = ArchiveEntry::State::New);
	static shared_ptr<ArchiveDir> getOrCreateSubdir(shared_ptr<ArchiveDir>& root, string_view path);
	static void                   entryTreeAsList(
						  ArchiveDir*                       root,
						  vector<shared_ptr<ArchiveEntry>>& list,
						  bool                              include_dir_entry = false);
	static shared_ptr<ArchiveDir> getShared(ArchiveDir* dir);

private:
	Archive*                         archive_;
	weak_ptr<ArchiveDir>             parent_dir_;
	shared_ptr<ArchiveEntry>         dir_entry_;
	vector<shared_ptr<ArchiveEntry>> entries_;
	vector<shared_ptr<ArchiveDir>>   subdirs_;
	bool                             allow_duplicate_names_ = true;

	void ensureUniqueName(ArchiveEntry* entry);
};
