
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveListView.cpp
// Description: List view for archive files, split to filename and path columns
//              and with appropriate icons for different archive types
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
#include "ArchiveListView.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveFormat.h"
#include "Graphics/Icons.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// ArchiveListView Class Functions
//
// -----------------------------------------------------------------------------

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
	InsertColumn(0, wxS("Filename"));
	InsertColumn(1, wxS("Path"));
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

// -----------------------------------------------------------------------------
// Appends a new item to the list with the given [path]
// -----------------------------------------------------------------------------
void ArchiveListView::append(string_view path)
{
	insert(GetItemCount(), path);
}

// -----------------------------------------------------------------------------
// Appends a new item to the list with the given [archive]'s filename
// -----------------------------------------------------------------------------
void ArchiveListView::append(const Archive* archive)
{
	insert(GetItemCount(), archive);
}

// -----------------------------------------------------------------------------
// Inserts a new item at [index] with the given [path]
// -----------------------------------------------------------------------------
void ArchiveListView::insert(int index, string_view path)
{
	InsertItem(index, wxEmptyString);
	setItem(index, path);
}

// -----------------------------------------------------------------------------
// Inserts a new item at [index] with the given [archive]'s filename
// -----------------------------------------------------------------------------
void ArchiveListView::insert(int index, const Archive* archive)
{
	InsertItem(index, wxEmptyString);
	setItem(index, archive);
}

// -----------------------------------------------------------------------------
// Sets the item at [index] to the given [path]
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Sets the item at [index] to the given [archive]'s filename and coloured by
// its status
// -----------------------------------------------------------------------------
void ArchiveListView::setItem(int index, const Archive* archive)
{
	if (!archive)
	{
		SetItem(index, 0, wxS("INVALID"));
		SetItem(index, 1, wxS("INVALID"));
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
