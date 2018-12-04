#pragma once

#include "ArchiveEntry.h"
#include "Utility/Tree.h"

class ArchiveTreeNode : public STreeNode
{
	friend class Archive;

public:
	ArchiveTreeNode(ArchiveTreeNode* parent = nullptr, Archive* archive = nullptr);
	~ArchiveTreeNode() = default;

	// Accessors
	Archive*                          archive() const;
	const vector<ArchiveEntry::SPtr>& entries() const { return entries_; }
	ArchiveEntry*                     dirEntry() const { return dir_entry_.get(); }

	// STreeNode
	string name() override;
	void   addChild(STreeNode* child) override;
	void   setName(string name) override { dir_entry_->name_ = name; }

	// Entry Access
	ArchiveEntry*      entryAt(unsigned index);
	ArchiveEntry::SPtr sharedEntryAt(unsigned index);
	ArchiveEntry*      entry(const string& name, bool cut_ext = false);
	ArchiveEntry::SPtr sharedEntry(const string& name, bool cut_ext = false);
	ArchiveEntry::SPtr sharedEntry(ArchiveEntry* entry);
	unsigned           numEntries(bool inc_subdirs = false);
	int                entryIndex(ArchiveEntry* entry, size_t startfrom = 0);

	vector<ArchiveEntry::SPtr> allEntries();

	// Entry Operations
	bool addEntry(ArchiveEntry* entry, unsigned index = 0xFFFFFFFF);
	bool addEntry(ArchiveEntry::SPtr& entry, unsigned index = 0xFFFFFFFF);
	bool removeEntry(unsigned index);
	bool swapEntries(unsigned index1, unsigned index2);

	// Other
	void             clear();
	ArchiveTreeNode* clone();

	bool merge(
		ArchiveTreeNode*    node,
		unsigned            position = 0xFFFFFFFF,
		ArchiveEntry::State state    = ArchiveEntry::State::New);
	bool exportTo(const string& path);
	void allowDuplicateNames(bool allow) { allow_duplicate_names_ = allow; }

	typedef std::unique_ptr<ArchiveTreeNode> UPtr;

protected:
	STreeNode* createChild(string name) override
	{
		auto node                    = new ArchiveTreeNode();
		node->dir_entry_->name_      = name;
		node->archive_               = archive_;
		node->allow_duplicate_names_ = allow_duplicate_names_;
		return node;
	}

private:
	Archive*                   archive_;
	ArchiveEntry::SPtr         dir_entry_;
	vector<ArchiveEntry::SPtr> entries_;
	bool                       allow_duplicate_names_ = true;

	void        ensureUniqueName(ArchiveEntry* entry);
	static void linkEntries(ArchiveEntry* first, ArchiveEntry* second);
};
