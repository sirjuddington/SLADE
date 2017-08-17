#pragma once

#include "Utility/Tree.h"
#include "ArchiveEntry.h"

class ArchiveTreeNode : public STreeNode
{
	friend class Archive;
public:
	ArchiveTreeNode(ArchiveTreeNode* parent = nullptr, Archive* archive = nullptr);
	~ArchiveTreeNode();

	// Accessors
	Archive*							archive();
	const vector<ArchiveEntry::SPtr>&	entries() const { return entries_; }
	ArchiveEntry*						dirEntry() const { return dir_entry_.get(); }

	// STreeNode
	string	getName() override;
	void 	addChild(STreeNode* child) override;
	void	setName(string name) override { dir_entry_->name = name; }
	
	// Entry Access
	ArchiveEntry*		entryAt(unsigned index);
	ArchiveEntry::SPtr	sharedEntryAt(unsigned index);
	ArchiveEntry*		entry(string name, bool cut_ext = false);
	ArchiveEntry::SPtr	sharedEntry(string name, bool cut_ext = false);
	ArchiveEntry::SPtr	sharedEntry(ArchiveEntry* entry);
	unsigned			numEntries(bool inc_subdirs = false);
	int					entryIndex(ArchiveEntry* entry, size_t startfrom = 0);

	vector<ArchiveEntry::SPtr>	allEntries();

	// Entry Operations
	void	linkEntries(ArchiveEntry* first, ArchiveEntry* second);
	bool	addEntry(ArchiveEntry* entry, unsigned index = 0xFFFFFFFF);
	bool	addEntry(ArchiveEntry::SPtr& entry, unsigned index = 0xFFFFFFFF);
	bool	removeEntry(unsigned index);
	bool	swapEntries(unsigned index1, unsigned index2);

	// Other
	void				clear();
	ArchiveTreeNode*	clone();
	bool				merge(ArchiveTreeNode* node, unsigned position = 0xFFFFFFFF, int state = 2);
	bool				exportTo(string path);
	void				allowDuplicateNames(bool allow) { allow_duplicate_names_ = allow; }

	typedef	std::unique_ptr<ArchiveTreeNode> Ptr;

protected:
	STreeNode* createChild(string name) override
	{
		ArchiveTreeNode* node = new ArchiveTreeNode();
		node->dir_entry_->name = name;
		node->archive_ = archive_;
		node->allow_duplicate_names_ = allow_duplicate_names_;
		return node;
	}

private:
	Archive*					archive_;
	ArchiveEntry::SPtr			dir_entry_;
	vector<ArchiveEntry::SPtr>	entries_;
	bool						allow_duplicate_names_ = true;

	void	ensureUniqueName(ArchiveEntry* entry);
};
