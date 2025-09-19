
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    BaseResourceArchiveSettingsPanel.cpp
// Description: Panel containing controls to select from and modify saved paths
//              to base resource archives
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
#include "BaseResourceArchiveSettingsPanel.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "UI/Controls/FileLocationPanel.h"
#include "UI/Lists/ArchiveListView.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"
#include "Utility/JsonUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, base_resource)
EXTERN_CVAR(String, dir_last)
EXTERN_CVAR(String, zdoom_pk3_path)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
#ifdef __WXMSW__
// -----------------------------------------------------------------------------
// Inspired by ZDoom. Automatizes some repetitive steps. Puts the value of a
// registry key into the reference, and returns true if the value actually
// exists and isn't an empty string.
// -----------------------------------------------------------------------------
bool queryPathKey(wxRegKey::StdKey hkey, const wxString& path, const wxString& variable, wxString& value)
{
	wxRegKey key(hkey, path);
	key.QueryValue(variable, value);
	key.Close();

	return (!value.empty());
}
#endif
} // namespace


// -----------------------------------------------------------------------------
//
// BaseResourceArchiveSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// BaseResourceArchiveSettingsPanel class constructor
// -----------------------------------------------------------------------------
BaseResourceArchiveSettingsPanel::BaseResourceArchiveSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	// Create controls
	list_base_archive_paths_ = new ArchiveListView(this);
	btn_add_                 = new wxButton(this, -1, wxS("Add Archive"));
	btn_remove_              = new wxButton(this, -1, wxS("Remove Archive"));
	btn_detect_              = new wxButton(this, -1, wxS("Detect Archives"));
	flp_zdoom_pk3_ = new FileLocationPanel(this, zdoom_pk3_path, false, "Browse ZDoom PK3", "Pk3 Files (*.pk3)|*.pk3");

	list_base_archive_paths_->setColumnSpacing(FromDIP(8));

	setupLayout();

	// Bind events
	btn_add_->Bind(wxEVT_BUTTON, &BaseResourceArchiveSettingsPanel::onBtnAdd, this);
	btn_remove_->Bind(wxEVT_BUTTON, &BaseResourceArchiveSettingsPanel::onBtnRemove, this);
	btn_detect_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { autodetect(); });

	// Init layout
	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Lays out the controls on the panel
// -----------------------------------------------------------------------------
void BaseResourceArchiveSettingsPanel::setupLayout()
{
	auto sizer = new wxGridBagSizer(ui::pad(this), ui::pad(this));
	SetSizer(sizer);

	// Paths list + buttons
	sizer->Add(list_base_archive_paths_, { 0, 0 }, { 4, 1 }, wxEXPAND);
	sizer->Add(btn_add_, { 0, 1 }, { 1, 1 }, wxEXPAND);
	sizer->Add(btn_remove_, { 1, 1 }, { 1, 1 }, wxEXPAND);
	sizer->Add(btn_detect_, { 2, 1 }, { 1, 1 }, wxEXPAND);

	// ZDoom.pk3 path
	sizer->Add(wxutil::createLabelHBox(this, "ZDoom PK3 Path:", flp_zdoom_pk3_), { 4, 0 }, { 1, 2 }, wxEXPAND);

	sizer->AddGrowableRow(3, 1);
	sizer->AddGrowableCol(0, 1);
}

// -----------------------------------------------------------------------------
// Returns the currently selected base resource path
// -----------------------------------------------------------------------------
int BaseResourceArchiveSettingsPanel::selectedPathIndex() const
{
	auto selected = list_base_archive_paths_->selectedItems();
	return selected.empty() ? -1 : selected[0];
}

// -----------------------------------------------------------------------------
// Automatically seek IWADs to populate the list
// -----------------------------------------------------------------------------
void BaseResourceArchiveSettingsPanel::autodetect() const
{
	vector<string> found_paths;

	// List of known IWADs and common aliases
	auto iwadlist = app::archiveManager().programResourceArchive()->entryAtPath("config/iwads.json");
	if (!iwadlist)
		return;
	auto j = jsonutil::parse(iwadlist->data());
	if (j.is_discarded())
		return;


	// Find IWADs from DOOMWADDIR and DOOMWADPATH
	// See http://doomwiki.org/wiki/Environment_variables
	wxString doomwaddir, doomwadpath, envvar;
	envvar = wxS("DOOMWADDIR");
	wxGetEnv(envvar, &doomwaddir);
	envvar = wxS("DOOMWADPATH");
	wxGetEnv(envvar, &doomwadpath);

	if (!doomwaddir.empty() || !doomwadpath.empty())
	{
#ifdef WIN32
		char separator = ';';
		doomwadpath.Replace(wxS("\\"), wxS("/"), true);
		doomwaddir.Replace(wxS("\\"), wxS("/"), true);
#else
		char separator = ':';
#endif

		auto paths = wxSplit(doomwadpath, separator);
		paths.Add(doomwaddir);
		wxArrayString iwadnames;
		for (auto& path : j["iwads"])
			iwadnames.Add(wxString::FromUTF8(path.get<string>()));

		// Look for every known IWAD in every known IWAD directory
		for (auto folder : paths)
		{
			if (folder.Last() != '/')
				folder += '/';
			for (const auto& iwadname : iwadnames)
			{
				wxString iwad = folder + iwadname;
#ifndef WIN32
				// Try a couple variants before throwing the towel about a name
				if (!wxFileExists(iwad))
					iwad = folder + iwadname.Capitalize();
				if (!wxFileExists(iwad))
					iwad = folder + iwadname.Upper();
#endif
				// If a valid combo is found, add it to the list unless already present
				if (wxFileExists(iwad))
				{
					// Verify existence before adding it to the list
					if (list_base_archive_paths_->findArchive(iwad) == wxNOT_FOUND)
					{
						app::archiveManager().addBaseResourcePath(iwad.utf8_string());
						list_base_archive_paths_->append(iwad.utf8_string());
					}
				}
			}
		}
	}

	// Let's take a look at the registry
	wxArrayString paths;
	wxString      path;
	wxString      gamepath;

	// Now query GOG.com paths -- Windows only for now
#ifdef __WXMSW__
#ifdef _WIN64
	string gogregistrypath = "Software\\Wow6432Node\\GOG.com";
#else
	// If a 32-bit ZDoom runs on a 64-bit Windows, this will be transparently and
	// automatically redirected to the Wow6432Node address instead, so this address
	// should be safe to use in all cases.
	wxString gogregistrypath = "Software\\GOG.com";
#endif
	if (queryPathKey(wxRegKey::HKLM, wxString::FromUTF8(gogregistrypath), wxS("DefaultPackPath"), path))
	{
		for (auto& [game, j_gog] : j["gog"].items())
		{
			gamepath = wxString::FromUTF8(j_gog["id"].get<string>());
			if (queryPathKey(wxRegKey::HKLM, gamepath, wxS("Path"), path))
				paths.Add(path + wxString::FromUTF8(j_gog["path"].get<string>()));
		}
	}
#endif


	// Now query Steam paths -- Windows only for now as well
#ifdef __WXMSW__
	if (queryPathKey(wxRegKey::HKCU, wxS("Software\\Valve\\Steam"), wxS("SteamPath"), gamepath)
		|| queryPathKey(wxRegKey::HKLM, wxS("Software\\Valve\\Steam"), wxS("InstallPath"), gamepath))
	{
		gamepath += wxS("/SteamApps/common/");
		for (auto& [game, path] : j["steam"].items())
			paths.Add(gamepath + wxString::FromUTF8(path.get<string>()));
	}
#else
	// TODO: Querying Steam registry on Linux and OSX. This involves parsing Steam's config.vdf file, which is found in
	// FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &folder) + "/Steam/config/config.vdf" on
	// OSX and in ~home/.local/share/Steam/config/config.vdf on Linux/BSD systems. There's also default install dirs in
	// appSupportPath + "/Steam/SteamApps/common" for OSX, ~home/.local/share/Steam/SteamApps/common for Linux/BSD.
#endif


	// Add GOG & Steam paths
	for (auto iwad : paths)
	{
		iwad.Replace(wxS("\\"), wxS("/"), true);
		if (wxFileExists(iwad))
		{
			// Verify existence before adding it to the list
			if (list_base_archive_paths_->findArchive(iwad) == wxNOT_FOUND)
			{
				app::archiveManager().addBaseResourcePath(iwad.utf8_string());
				list_base_archive_paths_->append(iwad.utf8_string());
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void BaseResourceArchiveSettingsPanel::loadSettings()
{
	// Init paths list
	list_base_archive_paths_->DeleteAllItems();
	for (size_t a = 0; a < app::archiveManager().numBaseResourcePaths(); a++)
		list_base_archive_paths_->append(app::archiveManager().getBaseResourcePath(a));

	// Select the currently open base archive if any
	if (base_resource >= 0)
		list_base_archive_paths_->selectItem(base_resource);

	flp_zdoom_pk3_->setLocation(zdoom_pk3_path);
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void BaseResourceArchiveSettingsPanel::applySettings()
{
	app::archiveManager().openBaseResource(selectedPathIndex());
	zdoom_pk3_path = flp_zdoom_pk3_->location();
}


// -----------------------------------------------------------------------------
//
// BaseResourceArchiveSettingsPanel Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the 'Add Archive' button is clicked
// -----------------------------------------------------------------------------
void BaseResourceArchiveSettingsPanel::onBtnAdd(wxCommandEvent& e)
{
	// Create extensions string
	auto extensions = app::archiveManager().getArchiveExtensionsString();

	// Open a file browser dialog that allows multiple selection
	wxFileDialog dialog_open(
		this,
		wxS("Choose file(s) to open"),
		dir_last,
		wxEmptyString,
		wxString::FromUTF8(extensions),
		wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST,
		wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_open.ShowModal() == wxID_OK)
	{
		// Get an array of selected filenames
		wxArrayString files;
		dialog_open.GetPaths(files);

		// Add each to the paths list
		for (const auto& file : files)
		{
			if (app::archiveManager().addBaseResourcePath(file.utf8_string()))
				list_base_archive_paths_->append(file.utf8_string());
		}

		// Save 'dir_last'
		dir_last = dialog_open.GetDirectory().utf8_string();
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Remove Archive' button is clicked
// -----------------------------------------------------------------------------
void BaseResourceArchiveSettingsPanel::onBtnRemove(wxCommandEvent& e)
{
	// Remove selected items
	for (auto item : list_base_archive_paths_->selectedItems())
	{
		list_base_archive_paths_->DeleteItem(item);

		// Also remove it from the archive manager
		app::archiveManager().removeBaseResourcePath(item);
	}
}
