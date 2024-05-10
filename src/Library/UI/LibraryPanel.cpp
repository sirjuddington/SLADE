
#include "Main.h"
#include "LibraryPanel.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveFormat.h"
#include "Archive/ArchiveManager.h"
#include "Database/Context.h"
#include "Database/Database.h"
#include "General/Misc.h"
#include "General/SAction.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "Library/ArchiveFile.h"
#include "Library/Library.h"
#include "MainEditor/MainEditor.h"
#include "UI/Dialogs/RunDialog.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/State.h"
#include "UI/WxUtils.h"
#include "Utility/DateTime.h"
#include "Utility/StringUtils.h"
#include <SQLiteCpp/Column.h>

using namespace slade;
using namespace ui;
using namespace library;


namespace
{
icons::IconCache icon_cache;
}


namespace
{
template<typename T> int compare(const T& left, const T& right)
{
	if (left == right)
		return 0;

	return left < right ? -1 : 1;
}

void openArchive(const LibraryViewModel::LibraryListRow* row)
{
	// Check if it's an archive within another archive
	if (row->parent_id >= 0)
	{
		// Open parent archive
		ArchiveFileRow parent_row{ db::global(), row->parent_id };
		maineditor::openArchiveFile(parent_row.path);

		// Now open the archive's entry within the parent
		auto parent_archive = app::archiveManager().getArchive(parent_row.path);
		if (auto entry = parent_archive->entryAtPath(strutil::replace(row->path, parent_row.path, {})))
			maineditor::openEntry(entry);

		return;
	}

	maineditor::openArchiveFile(row->path);
}
} // namespace

LibraryViewModel::LibraryViewModel()
{
	loadRows();
	Cleared();

	// Connect to library signals ----------------------------------------------
	auto& signals = library::signals();

	// Archive updated
	signal_connections_ += signals.archive_file_updated.connect(
		[this](int64_t id)
		{
			bool changed = false;
			for (auto& row : rows_)
				if (row.id == id)
				{
					row = LibraryListRow{ db::global(), id };
					ItemChanged(wxDataViewItem{ &row });
					changed = true;
				}

			if (changed)
				Resort();
		});

	// Archive added
	signal_connections_ += signals.archive_file_inserted.connect(
		[this](int64_t id)
		{
			rows_.emplace_back(database::global(), id);
			ItemAdded({}, wxDataViewItem{ &rows_.back() });
			Resort();
		});

	// Archive deleted
	signal_connections_ += signals.archive_file_deleted.connect(
		[this](int64_t id)
		{
			for (auto i = 0; i < rows_.size(); ++i)
				if (rows_[i].id == id)
				{
					ItemDeleted({}, wxDataViewItem{ &rows_[i] });
					rows_.erase(rows_.begin() + i);
					Resort();
					break;
				}
		});
}

LibraryViewModel::LibraryListRow::LibraryListRow(database::Context& db, int64_t id)
{
	if (auto sql = db.cacheQuery("get_library_list", "SELECT * FROM archive_library_list WHERE id = ?"))
	{
		sql->bind(1, id);

		if (sql->executeStep())
		{
			this->id      = id;
			path          = sql->getColumn(1).getString();
			size          = sql->getColumn(2).getUInt();
			format_id     = sql->getColumn(3).getString();
			last_opened   = sql->getColumn(4).getInt64();
			last_modified = sql->getColumn(5).getInt64();
			parent_id     = sql->getColumn(6).getInt64();
			entry_count   = sql->getColumn(7).getUInt();
			map_count     = sql->getColumn(8).getUInt();
		}

		sql->reset();
	}
}

wxDataViewItem LibraryViewModel::itemForArchiveId(int64_t id) const
{
	for (auto& row : rows_)
		if (row.id == id)
			return wxDataViewItem{ &row };

	return {};
}

void LibraryViewModel::setFilter(string_view filter)
{
	filter_ = filter;

	Cleared();
}

wxString LibraryViewModel::GetColumnType(unsigned col) const
{
	switch (static_cast<Column>(col))
	{
	case Column::Name: return "wxDataViewIconText";
	default:           return "string";
	}
}

void LibraryViewModel::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned col) const
{
	auto* row = static_cast<LibraryListRow*>(item.GetID());
	if (!row)
		return;

	switch (static_cast<Column>(col))
	{
	case Column::Name:
	{
		// Determine icon
		string icon = "archive";
		if (row->format_id == "wad")
			icon = "wad";
		else if (row->format_id == "zip")
			icon = "zip";
		else if (row->format_id == "folder")
			icon = "folder";

		// Find icon in cache
		if (!icon_cache.isCached(icon))
		{
			// Not found, add to cache
			constexpr auto pad = Point2i{ 1, 1 };
			icon_cache.cacheIcon(icons::Type::Entry, icon, 16, pad);
		}

		auto fn = wxutil::strFromView(strutil::Path::fileNameOf(row->path));

		variant << wxDataViewIconText(fn, icon_cache.icons[icon]);

		break;
	}
	case Column::Path: variant = wxutil::strFromView(strutil::Path::pathOf(row->path, false)); break;
	case Column::Size: variant = misc::sizeAsString(row->size); break;
	case Column::Type:
	{
		auto fn_ext = strutil::Path::extensionOf(row->path);
		auto desc   = archive::formatInfoFromId(row->format_id);

		variant = desc.name;

		for (const auto& ext : desc.extensions)
			if (strutil::equalCI(fn_ext, ext.first))
				variant = ext.second;

		break;
	}
	case Column::LastOpened:
		if (row->last_opened == 0)
			variant = "Never";
		else
			variant = datetime::toString(row->last_opened, datetime::Format::Local);
		break;
	case Column::FileModified:
		if (row->last_modified == 0)
			variant = "Unknown";
		else
			variant = datetime::toString(row->last_modified, datetime::Format::Local);
		break;
	case Column::EntryCount: variant = wxString::Format("%d", row->entry_count); break;
	case Column::MapCount:   variant = row->map_count > 0 ? wxString ::Format("%d", row->map_count) : ""; break;
	default:                 break;
	}
}

bool LibraryViewModel::GetAttr(const wxDataViewItem& item, unsigned col, wxDataViewItemAttr& attr) const
{
	return wxDataViewModel::GetAttr(item, col, attr);
}

bool LibraryViewModel::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned col)
{
	return false;
}

wxDataViewItem LibraryViewModel::GetParent(const wxDataViewItem& item) const
{
	return {};
}

bool LibraryViewModel::IsContainer(const wxDataViewItem& item) const
{
	return !item.IsOk();
}

unsigned LibraryViewModel::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
{
	if (!item.IsOk())
	{
		// Root item
		for (auto& row : rows_)
			if (matchesFilter(row))
				children.Add(wxDataViewItem{ &row });

		return children.size();
	}

	return 0;
}

int LibraryViewModel::Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned column, bool ascending)
	const
{
	auto row1 = static_cast<LibraryListRow*>(item1.GetID());
	auto row2 = static_cast<LibraryListRow*>(item2.GetID());
	if (!row1 || !row2)
		return 0;

	auto col = static_cast<Column>(column);

	// Size column
	if (col == Column::Size)
		return ascending ? compare(row1->size, row2->size) : compare(row2->size, row1->size);

	// Last Opened column
	if (col == Column::LastOpened)
		return ascending ? compare(row1->last_opened, row2->last_opened) :
						   compare(row2->last_opened, row1->last_opened);

	// File Modified column
	if (col == Column::FileModified)
		return ascending ? compare(row1->last_modified, row2->last_modified) :
						   compare(row2->last_modified, row1->last_modified);

	// Entry Count column
	if (col == Column::EntryCount)
		return ascending ? compare(row1->entry_count, row2->entry_count) :
						   compare(row2->entry_count, row1->entry_count);

	// Map Count column
	if (col == Column::MapCount)
		return ascending ? compare(row1->map_count, row2->map_count) : compare(row2->map_count, row1->map_count);

	// Default compare
	return wxDataViewModel::Compare(item1, item2, column, ascending);
}

void LibraryViewModel::loadRows() const
{
	rows_.clear();

	if (auto sql = db::cacheQuery("library_list", "SELECT * FROM archive_library_list"))
	{
		LibraryListRow row;
		while (sql->executeStep())
		{
			row.id            = sql->getColumn(0).getInt64();
			row.path          = sql->getColumn(1).getString();
			row.size          = sql->getColumn(2).getUInt();
			row.format_id     = sql->getColumn(3).getString();
			row.last_opened   = sql->getColumn(4).getInt64();
			row.last_modified = sql->getColumn(5).getInt64();
			row.parent_id     = sql->getColumn(6).getInt64();
			row.entry_count   = sql->getColumn(7).getUInt();
			row.map_count     = sql->getColumn(8).getUInt();

			rows_.push_back(row);
		}

		sql->reset();
	}
}

bool LibraryViewModel::matchesFilter(const LibraryListRow& row) const
{
	// Check for filename match if needed
	if (!filter_.empty())
		return strutil::matchesCI(strutil::Path::fileNameOf(row.path), fmt::format("*{}*", filter_));

	return true;
}

LibraryPanel::LibraryPanel(wxWindow* parent) : wxPanel{ parent }
{
	setup();
}

void LibraryPanel::setup()
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Toolbar
	setupToolbar();
	sizer->Add(toolbar_, wxutil::sfWithBorder(0, wxLEFT | wxRIGHT | wxTOP).Expand());
	sizer->AddSpacer(px(Size::Pad));

	// Archive list
	list_archives_ = new SDataViewCtrl{ this, wxDV_MULTIPLE };
	sizer->Add(list_archives_, wxutil::sfWithBorder(1, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Init archive list
	model_library_ = new LibraryViewModel();
	list_archives_->AssociateModel(model_library_);
	model_library_->DecRef();
	setupListColumns();

	bindEvents();
}

void LibraryPanel::setupToolbar()
{
	toolbar_ = new SToolBar(this);

	// Actions
	toolbar_->addActionGroup("_Library", { "alib_open", "alib_run", "alib_remove" });

	// Filter
	auto tbg_filter = new SToolBarGroup(toolbar_, "Filter");
	text_filter_    = new wxTextCtrl(tbg_filter, -1);
	tbg_filter->addCustomControl(new wxStaticText(tbg_filter, -1, "Filter:"));
	tbg_filter->addCustomControl(text_filter_);
	toolbar_->addGroup(tbg_filter, true);
}

bool LibraryPanel::handleAction(string_view id)
{
	// Open
	if (id == "alib_open")
	{
		wxDataViewItemArray selection;
		list_archives_->GetSelections(selection);

		for (const auto& item : selection)
			if (auto row = model_library_->rowForItem(item))
				openArchive(row);

		return true;
	}

	// Remove
	if (id == "alib_remove")
	{
		wxDataViewItemArray selection;
		list_archives_->GetSelections(selection);

		vector<int64_t> to_remove;
		for (const auto& item : selection)
			if (auto row = model_library_->rowForItem(item))
				to_remove.push_back(row->id);

		for (auto archive_id : to_remove)
			library::removeArchiveFile(archive_id);

		return true;
	}

	// Run
	if (id == "alib_run")
	{
		wxDataViewItemArray selection;
		list_archives_->GetSelections(selection);

		string  path;
		int64_t id = -1;
		for (const auto& item : selection)
			if (auto row = model_library_->rowForItem(item))
			{
				path = row->path;
				id   = row->id;
				break;
			}

		RunDialog dlg{ this, id };
		if (dlg.ShowModal() == wxID_OK)
			dlg.run(RunDialog::Config{ path }, id);

		return true;
	}

	return false;
}

void LibraryPanel::bindEvents()
{
	// Open archive if activated
	list_archives_->Bind(
		wxEVT_DATAVIEW_ITEM_ACTIVATED,
		[this](wxDataViewEvent& e)
		{
			if (auto row = model_library_->rowForItem(e.GetItem()))
				openArchive(row);
		});

	// Context menu
	list_archives_->Bind(
		wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
		[this](wxDataViewEvent& e)
		{
			wxMenu context;
			SAction::fromId("alib_open")->addToMenu(&context);
			SAction::fromId("alib_run")->addToMenu(&context);
			SAction::fromId("alib_remove")->addToMenu(&context);
			list_archives_->PopupMenu(&context);
		});

	// Header right click
	list_archives_->Bind(
		wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK,
		[this](wxDataViewEvent& e)
		{
			using Column = LibraryViewModel::Column;

			// Popup context menu
			wxMenu context;
			context.Append(static_cast<int>(Column::__Count), "Reset Sorting");
			context.AppendSeparator();
			list_archives_->appendColumnToggleItem(context, static_cast<int>(Column::Path));
			list_archives_->appendColumnToggleItem(context, static_cast<int>(Column::Size));
			list_archives_->appendColumnToggleItem(context, static_cast<int>(Column::Type));
			list_archives_->appendColumnToggleItem(context, static_cast<int>(Column::LastOpened));
			list_archives_->appendColumnToggleItem(context, static_cast<int>(Column::FileModified));
			list_archives_->appendColumnToggleItem(context, static_cast<int>(Column::EntryCount));
			list_archives_->appendColumnToggleItem(context, static_cast<int>(Column::MapCount));
			list_archives_->PopupMenu(&context);
			e.Skip();
		});

	// Header context menu
	list_archives_->Bind(
		wxEVT_MENU,
		[this](wxCommandEvent& e)
		{
			using Column = LibraryViewModel::Column;

			string toggle_column;
			switch (static_cast<Column>(e.GetId()))
			{
			case Column::Name:         break;
			case Column::Path:         toggle_column = "LibraryPanelPathVisible"; break;
			case Column::Size:         toggle_column = "LibraryPanelSizeVisible"; break;
			case Column::Type:         toggle_column = "LibraryPanelTypeVisible"; break;
			case Column::LastOpened:   toggle_column = "LibraryPanelLastOpenedVisible"; break;
			case Column::FileModified: toggle_column = "LibraryPanelFileModifiedVisible"; break;
			case Column::EntryCount:   toggle_column = "LibraryPanelEntryCountVisible"; break;
			case Column::MapCount:     toggle_column = "LibraryPanelMapCountVisible"; break;
			case Column::__Count:      list_archives_->resetSorting(); break;
			default:                   e.Skip(); break;
			}

			if (!toggle_column.empty())
			{
				list_archives_->toggleColumnVisibility(e.GetId(), toggle_column);
				updateColumnWidths();
			}
		});

	// List column resized
	list_archives_->Bind(
		EVT_SDVC_COLUMN_RESIZED,
		[this](wxDataViewEvent& e)
		{
			using Column = LibraryViewModel::Column;
			auto col     = static_cast<Column>(e.GetColumn());
			auto width   = e.GetDataViewColumn()->GetWidth();

			switch (col)
			{
			case Column::Name:         saveStateInt("LibraryPanelFilenameWidth", width); break;
			case Column::Path:         saveStateInt("LibraryPanelPathWidth", width); break;
			case Column::Size:         saveStateInt("LibraryPanelSizeWidth", width); break;
			case Column::Type:         saveStateInt("LibraryPanelTypeWidth", width); break;
			case Column::LastOpened:   saveStateInt("LibraryPanelLastOpenedWidth", width); break;
			case Column::FileModified: saveStateInt("LibraryPanelFileModifiedWidth", width); break;
			case Column::EntryCount:   saveStateInt("LibraryPanelEntryCountWidth", width); break;
			case Column::MapCount:     saveStateInt("LibraryPanelMapCountWidth", width); break;
			default:                   break;
			}
		});

	// Filter changed
	text_filter_->Bind(
		wxEVT_TEXT,
		[this](wxCommandEvent& e) { model_library_->setFilter(wxutil::strToView(text_filter_->GetValue())); });
}

void LibraryPanel::setupListColumns() const
{
	using Column = LibraryViewModel::Column;

	// Search by filename column
	list_archives_->setSearchColumn(static_cast<int>(Column::Name));

	// Filename column is fixed
	list_archives_->AppendIconTextColumn(
		"Filename",
		static_cast<int>(Column::Name),
		wxDATAVIEW_CELL_INERT,
		getStateInt("LibraryPanelFilenameWidth"),
		wxALIGN_NOT,
		wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_RESIZABLE);

	// Add other columns
	appendTextColumn(Column::Path, "Path", "Path");
	appendTextColumn(Column::Size, "Size", "Size");
	appendTextColumn(Column::Type, "Type", "Type");
	appendTextColumn(Column::LastOpened, "Last Opened", "LastOpened");
	appendTextColumn(Column::FileModified, "File Modified", "FileModified");
	appendTextColumn(Column::EntryCount, "# Entries", "EntryCount");
	appendTextColumn(Column::MapCount, "# Maps", "MapCount");
}

void LibraryPanel::updateColumnWidths()
{
	using Column = LibraryViewModel::Column;

	Freeze();
	list_archives_->setColumnWidth(modelColumn(Column::Name), getStateInt("LibraryPanelFilenameWidth"));
	list_archives_->setColumnWidth(modelColumn(Column::Path), getStateInt("LibraryPanelPathWidth"));
	list_archives_->setColumnWidth(modelColumn(Column::Size), getStateInt("LibraryPanelSizeWidth"));
	list_archives_->setColumnWidth(modelColumn(Column::Type), getStateInt("LibraryPanelTypeWidth"));
	list_archives_->setColumnWidth(modelColumn(Column::LastOpened), getStateInt("LibraryPanelLastOpenedWidth"));
	list_archives_->setColumnWidth(modelColumn(Column::FileModified), getStateInt("LibraryPanelFileModifiedWidth"));
	list_archives_->setColumnWidth(modelColumn(Column::EntryCount), getStateInt("LibraryPanelEntryCountWidth"));
	list_archives_->setColumnWidth(modelColumn(Column::MapCount), getStateInt("LibraryPanelMapCountWidth"));
	Thaw();
}

void LibraryPanel::appendTextColumn(LibraryViewModel::Column column, string_view title, string_view id) const
{
	static auto colstyle_visible = wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_RESIZABLE;
	static auto colstyle_hidden  = colstyle_visible | wxDATAVIEW_COL_HIDDEN;

	list_archives_->AppendTextColumn(
		wxutil::strFromView(title),
		static_cast<int>(column),
		wxDATAVIEW_CELL_INERT,
		getStateInt(fmt::format("LibraryPanel{}Width", id)),
		wxALIGN_NOT,
		getStateBool(fmt::format("LibraryPanel{}Visible", id)) ? colstyle_visible : colstyle_hidden);
}
