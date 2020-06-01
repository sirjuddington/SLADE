#include "Main.h"
#include "ArchiveEntryTree.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Graphics/Icons.h"
#include "UI/WxUtils.h"
#include <wx/headerctrl.h>

using namespace slade;
using namespace ui;

CVAR(Int, elist_colsize_name, 150, CVar::Save)
CVAR(Int, elist_colsize_size, 80, CVar::Save)
CVAR(Int, elist_colsize_type, 150, CVar::Save)
CVAR(Int, elist_colsize_index, 50, CVar::Save)

EXTERN_CVAR(Int, elist_icon_size)
EXTERN_CVAR(Int, elist_icon_padding)

void ArchiveViewModel::openArchive(shared_ptr<Archive> archive)
{
	archive_ = archive;

	// Add root items
	auto                dir = archive->rootDir();
	wxDataViewItemArray items;
	for (const auto& subdir : dir->subdirs())
		items.push_back(wxDataViewItem{ subdir->dirEntry() });
	for (const auto& entry : dir->entries())
		items.push_back(wxDataViewItem{ entry.get() });
	ItemsAdded({}, items);

	// Notify when entry added
	connections_ += archive->signals().entry_added.connect([this](Archive& archive, ArchiveEntry& entry) {
		ItemAdded(createItemForDirectory(entry.parentDir()), wxDataViewItem(&entry));
	});

	// Notify when entry removed
	connections_ += archive->signals().entry_removed.connect(
		[this](Archive& archive, ArchiveDir& dir, ArchiveEntry& entry) {
			ItemDeleted(createItemForDirectory(&dir), wxDataViewItem(&entry));
		});

	// Notify when dir added
	connections_ += archive->signals().dir_added.connect([this](Archive& archive, ArchiveDir& dir) {
		ItemAdded(createItemForDirectory(dir.parent().get()), wxDataViewItem(dir.dirEntry()));
	});

	// Notify when dir removed
	connections_ += archive->signals().dir_removed.connect(
		[this](Archive& archive, ArchiveDir& parent, ArchiveDir& dir) {
			ItemDeleted(createItemForDirectory(&parent), wxDataViewItem(dir.dirEntry()));
		});
}

unsigned int ArchiveViewModel::GetColumnCount() const
{
	return 3;
}

wxString ArchiveViewModel::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0: return "wxDataViewIconText";
	case 1: return "string";
	case 2: return "string";
	default: return "string";
	}
}

void ArchiveViewModel::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const
{
	// Check the item contains an entry
	auto* entry = static_cast<ArchiveEntry*>(item.GetID());
	if (!entry)
		return;

	// Name column
	if (col == 0)
	{
		auto icon_bmp = elist_icon_padding > 0 ?
							icons::getPaddedIcon(
								icons::Type::Entry, entry->type()->icon(), elist_icon_size, elist_icon_padding) :
							icons::getIcon(icons::Type::Entry, entry->type()->icon(), elist_icon_size);

		wxIcon icon;
		icon.CopyFromBitmap(icon_bmp);
		variant << wxDataViewIconText(entry->name(), icon);
	}

	// Size column
	else if (col == 1)
		variant = entry->sizeString();

	// Type column
	else if (col == 2)
		variant = entry->typeString();

	// Invalid
	else
		variant = "Invalid Column";
}

bool ArchiveViewModel::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col)
{
	// TODO: This (in-place rename)
	return false;
}

wxDataViewItem ArchiveViewModel::GetParent(const wxDataViewItem& item) const
{
	if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
		if (auto* pdir = entry->parentDir())
		{
			// The root dir of the archive is the 'invalid' item (ie. hidden root node)
			if (pdir == archive_.lock()->rootDir().get())
				return {};
			else
				return wxDataViewItem{ pdir->dirEntry() };
		}

	return {};
}

bool ArchiveViewModel::IsContainer(const wxDataViewItem& item) const
{
	if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
		if (entry->type() != EntryType::folderType())
			return false;

	return true;
}

unsigned int ArchiveViewModel::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
{
	auto* archive = archive_.lock().get();
	if (!archive)
		return 0;

	// Check if the item is a directory
	ArchiveDir* dir;
	if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
	{
		if (entry->type() == EntryType::folderType())
			dir = archive->dirAtPath(entry->path(true));
		else
			return 0; // Non-directory entry, no children
	}
	else
		dir = archive->rootDir().get(); // 'Invalid' item is the archive root dir

	// Get items for directory subdirs + entries
	for (const auto& subdir : dir->subdirs())
		children.push_back(wxDataViewItem{ subdir->dirEntry() });

	for (const auto& entry : dir->entries())
		children.push_back(wxDataViewItem{ entry.get() });

	return dir->numSubdirs() + dir->numEntries();
}

bool ArchiveViewModel::IsListModel() const
{
	// Show as a list (no spacing for expanders) if the archive doesn't support directories
	if (auto* archive = archive_.lock().get())
		return !archive->formatDesc().supports_dirs;

	return false;
}

wxDataViewItem slade::ui::ArchiveViewModel::createItemForDirectory(const ArchiveDir* dir)
{
	if (auto archive = archive_.lock())
	{
		if (dir == archive->rootDir().get())
			return {};
		else
			return wxDataViewItem{ dir->dirEntry() };
	}

	return {};
}







ArchiveEntryTree::ArchiveEntryTree(wxWindow* parent, shared_ptr<Archive> archive) :
	wxDataViewCtrl(parent, -1, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE), archive_{ archive }
{
	// Init settings
	SetRowHeight(ui::scalePx(elist_icon_size + (elist_icon_padding * 2) + 2));

	// Create & associate model
	model_ = new ArchiveViewModel();
	model_->openArchive(archive);
	AssociateModel(model_);
	model_->DecRef();

	// Add Columns
	col_name_ = AppendIconTextColumn("Name", 0, wxDATAVIEW_CELL_EDITABLE, elist_colsize_name);
	col_size_ = AppendTextColumn("Size", 1, wxDATAVIEW_CELL_INERT, elist_colsize_size);
	col_type_ = AppendTextColumn("Type", 2, wxDATAVIEW_CELL_INERT, elist_colsize_type);
	GetColumn(GetColumnCount() - 1)->SetWidth(0); // temp

	// --- Bind Events ---

	// Expand/Contract folders if activated
	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [this](wxDataViewEvent& e) {
		if (IsExpanded(e.GetItem()))
			Collapse(e.GetItem());
		else
			Expand(e.GetItem());
	});

	// Update column width cvars when we can
	Bind(wxEVT_IDLE, [this](wxIdleEvent& e) {
		elist_colsize_name = col_name_->GetWidth();
		elist_colsize_size = col_size_->GetWidth();
		elist_colsize_type = col_type_->GetWidth();
	});
}

ArchiveDir* ArchiveEntryTree::dirForItem(const wxDataViewItem& item) const
{
	if (auto archive = archive_.lock())
	{
		auto* entry = static_cast<ArchiveEntry*>(item.GetID());
		return ArchiveDir::findDirByDirEntry(archive_.lock()->rootDir(), *entry).get();
	}

	return nullptr;
}

vector<ArchiveEntry*> ArchiveEntryTree::selectedEntries(bool include_dirs) const
{
	if (GetSelectedItemsCount() == 0)
		return {};

	vector<ArchiveEntry*> entries;

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Add (non-folder) entries from selected items
	for (const auto& item : selection)
		if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
			if (include_dirs || entry->type() != EntryType::folderType())
				entries.push_back(entry);

	return entries;
}

ArchiveEntry* ArchiveEntryTree::firstSelectedEntry(bool include_dirs) const
{
	if (GetSelectedItemsCount() == 0)
		return nullptr;

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Find first (non-folder) entry in selected items
	for (const auto& item : selection)
		if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
			if (include_dirs || entry->type() != EntryType::folderType())
				return entry;

	return nullptr;
}

ArchiveEntry* ArchiveEntryTree::lastSelectedEntry(bool include_dirs) const
{
	if (GetSelectedItemsCount() == 0)
		return nullptr;

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Find last (non-folder) entry in selected items
	for (int i = selection.size() - 1; i >= 0; --i)
		if (auto* entry = static_cast<ArchiveEntry*>(selection[i].GetID()))
			if (include_dirs || entry->type() != EntryType::folderType())
				return entry;

	return nullptr;
}

vector<ArchiveDir*> ArchiveEntryTree::selectedDirectories() const
{
	if (GetSelectedItemsCount() == 0)
		return {};

	auto* archive = archive_.lock().get();
	if (!archive || !archive->formatDesc().supports_dirs)
		return {};

	vector<ArchiveDir*> dirs;

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Add dirs from selected items
	auto dir_root = archive->rootDir();
	for (const auto& item : selection)
		if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
			if (entry->type() == EntryType::folderType())
				if (auto dir = ArchiveDir::findDirByDirEntry(dir_root, *entry))
					dirs.push_back(dir.get());

	return dirs;
}

ArchiveDir* ArchiveEntryTree::firstSelectedDirectory() const
{
	if (GetSelectedItemsCount() == 0)
		return {};

	auto* archive = archive_.lock().get();
	if (!archive || !archive->formatDesc().supports_dirs)
		return {};

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Find first directory in selected items
	auto dir_root = archive->rootDir();
	for (const auto& item : selection)
		if (auto* entry = static_cast<ArchiveEntry*>(item.GetID()))
			if (entry->type() == EntryType::folderType())
				if (auto dir = ArchiveDir::findDirByDirEntry(dir_root, *entry))
					return dir.get();

	return nullptr;
}

ArchiveDir* ArchiveEntryTree::lastSelectedDirectory() const
{
	if (GetSelectedItemsCount() == 0)
		return {};

	auto* archive = archive_.lock().get();
	if (!archive || !archive->formatDesc().supports_dirs)
		return {};

	// Get selected tree items
	wxDataViewItemArray selection;
	GetSelections(selection);

	// Find first directory in selected items
	auto dir_root = archive->rootDir();
	for (int i = selection.size() - 1; i >= 0; --i)
		if (auto* entry = static_cast<ArchiveEntry*>(selection[i].GetID()))
			if (entry->type() == EntryType::folderType())
				if (auto dir = ArchiveDir::findDirByDirEntry(dir_root, *entry))
					return dir.get();

	return nullptr;
}
