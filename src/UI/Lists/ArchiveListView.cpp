
#include "Main.h"
#include "ArchiveListView.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveFormat.h"
#include "Graphics/Icons.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace ui;

// -----------------------------------------------------------------------------
// ArchiveListView class constructor
// -----------------------------------------------------------------------------
ArchiveListView::ArchiveListView(wxWindow* parent) : ListView(parent, -1)
{
	// Setup image list
	auto list = wxutil::createSmallImageList();
	wxutil::addImageListIcon(list, icons::Entry, "archive");
	wxutil::addImageListIcon(list, icons::Entry, "wad");
	wxutil::addImageListIcon(list, icons::Entry, "zip");
	wxutil::addImageListIcon(list, icons::Entry, "folder");
	ListView::SetImageList(list, wxIMAGE_LIST_SMALL);

	// Add columns
	InsertColumn(0, "Filename");
	InsertColumn(1, "Path");
}

// -----------------------------------------------------------------------------
// Returns the index of the list item with the given [path], or -1 if not found
// -----------------------------------------------------------------------------
int ArchiveListView::findArchive(const wxString& path) const
{
	for (int i = 0; i < GetItemCount(); i++)
	{
		auto item_path = GetItemText(i, 1).Append(GetItemText(i));
		if (item_path.CmpNoCase(path) == 0)
			return i;
	}

	return -1;
}
int ArchiveListView::findArchive(string_view path) const
{
	return findArchive(wxutil::strFromView(path));
}

void ArchiveListView::append(string_view path)
{
	insert(GetItemCount(), path);
}

void ArchiveListView::append(const Archive* archive)
{
	insert(GetItemCount(), archive);
}

void ArchiveListView::insert(int index, string_view path)
{
	InsertItem(index, wxEmptyString);
	setItem(index, path);
}

void ArchiveListView::insert(int index, const Archive* archive)
{
	InsertItem(index, wxEmptyString);
	setItem(index, archive);
}

void ArchiveListView::setItem(int index, string_view path)
{
	auto fn = strutil::Path(path);
	SetItem(index, 0, wxutil::strFromView(fn.fileName()));
	SetItem(index, 1, wxutil::strFromView(fn.path()));

	// Set item icon
	int  icon   = 0;
	auto format = archive::formatFromExtension(fn.extension());
	if (format == ArchiveFormat::Wad)
		icon = 1;
	else if (format == ArchiveFormat::Zip)
		icon = 2;
	else if (wxDirExists(wxutil::strFromView(path)))
		icon = 3;
	SetItemImage(index, icon);

	updateSize();
}

void ArchiveListView::setItem(int index, const Archive* archive)
{
	if (!archive)
	{
		SetItem(index, 0, "INVALID");
		SetItem(index, 1, "INVALID");
		SetItemImage(index, 0);
		return;
	}

	setItem(index, archive->filename());

	// Set item status colour
	if (archive->canSave())
	{
		if (archive->isModified())
			setItemStatus(index, ItemStatus::Modified);
		else
			setItemStatus(index, ItemStatus::Normal);
	}
	else
		setItemStatus(index, ItemStatus::New);
}
