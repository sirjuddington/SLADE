
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    BaseResourceArchivesPanel.cpp
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
#include "BaseResourceArchivesPanel.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "General/UI.h"
#include "UI/Controls/FileLocationPanel.h"
#include "UI/WxUtils.h"
#include "Utility/Parser.h"

using namespace slade;


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
bool QueryPathKey(wxRegKey::StdKey hkey, const wxString& path, const wxString& variable, wxString& value)
{
	wxRegKey key(hkey, path);
	key.QueryValue(variable, value);
	key.Close();

	return (value.length() > 0);
}
#endif
} // namespace


// -----------------------------------------------------------------------------
//
// BaseResourceArchivesPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// BaseResourceArchivesPanel class constructor
// -----------------------------------------------------------------------------
BaseResourceArchivesPanel::BaseResourceArchivesPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create controls
	list_base_archive_paths_ = new wxListBox(this, -1);
	btn_add_                 = new wxButton(this, -1, "Add Archive");
	btn_remove_              = new wxButton(this, -1, "Remove Archive");
	btn_detect_              = new wxButton(this, -1, "Detect Archives");
	flp_zdoom_pk3_ = new FileLocationPanel(this, zdoom_pk3_path, false, "Browse ZDoom PK3", "Pk3 Files (*.pk3)|*.pk3");

	setupLayout();

	// Bind events
	btn_add_->Bind(wxEVT_BUTTON, &BaseResourceArchivesPanel::onBtnAdd, this);
	btn_remove_->Bind(wxEVT_BUTTON, &BaseResourceArchivesPanel::onBtnRemove, this);
	btn_detect_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { autodetect(); });

	// Init layout
	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Lays out the controls on the panel
// -----------------------------------------------------------------------------
void BaseResourceArchivesPanel::setupLayout()
{
	auto sizer = new wxGridBagSizer(ui::pad(), ui::pad());
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
int BaseResourceArchivesPanel::selectedPathIndex() const
{
	return list_base_archive_paths_->GetSelection();
}

// -----------------------------------------------------------------------------
// Automatically seek IWADs to populate the list
// -----------------------------------------------------------------------------
void BaseResourceArchivesPanel::autodetect() const
{
	// List of known IWADs and common aliases
	auto iwadlist = app::archiveManager().programResourceArchive()->entryAtPath("config/iwads.cfg");
	if (!iwadlist)
		return;
	Parser p;
	p.parseText(iwadlist->data(), "slade.pk3:config/iwads.cfg");


	// Find IWADs from DOOMWADDIR and DOOMWADPATH
	// See http://doomwiki.org/wiki/Environment_variables
	wxString doomwaddir, doomwadpath, envvar;
	envvar = "DOOMWADDIR";
	wxGetEnv(envvar, &doomwaddir);
	envvar = "DOOMWADPATH";
	wxGetEnv(envvar, &doomwadpath);

	if (doomwaddir.length() || doomwadpath.length())
	{
#ifdef WIN32
		char separator = ';';
		doomwadpath.Replace("\\", "/", true);
		doomwaddir.Replace("\\", "/", true);
#else
		char separator = ':';
#endif

		auto paths = wxSplit(doomwadpath, separator);
		paths.Add(doomwaddir);
		wxArrayString iwadnames;
		auto          list = p.parseTreeRoot()->childPTN("iwads");
		for (size_t i = 0; i < list->nChildren(); ++i)
			iwadnames.Add(list->child(i)->name());

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
					if (list_base_archive_paths_->FindString(iwad) == wxNOT_FOUND)
					{
						app::archiveManager().addBaseResourcePath(iwad.ToStdString());
						list_base_archive_paths_->Append(iwad);
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
	if (QueryPathKey(wxRegKey::HKLM, gogregistrypath, "DefaultPackPath", path))
	{
		auto list = p.parseTreeRoot()->childPTN("gog");
		for (size_t i = 0; i < list->nChildren(); ++i)
		{
			auto child = list->childPTN(i);
			gamepath   = gogregistrypath + (child->childPTN("id"))->stringValue();
			if (QueryPathKey(wxRegKey::HKLM, gamepath, "Path", path))
				paths.Add(path + (child->childPTN("path"))->stringValue());
		}
	}
#endif


	// Now query Steam paths -- Windows only for now as well
#ifdef __WXMSW__
	if (QueryPathKey(wxRegKey::HKCU, "Software\\Valve\\Steam", "SteamPath", gamepath)
		|| QueryPathKey(wxRegKey::HKLM, "Software\\Valve\\Steam", "InstallPath", gamepath))
	{
		gamepath += "/SteamApps/common/";
		auto list = p.parseTreeRoot()->childPTN("steam");
		for (size_t i = 0; i < list->nChildren(); ++i)
			paths.Add(gamepath + (list->childPTN(i))->stringValue());
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
		iwad.Replace("\\", "/", true);
		if (wxFileExists(iwad))
		{
			// Verify existence before adding it to the list
			if (list_base_archive_paths_->FindString(iwad) == wxNOT_FOUND)
			{
				app::archiveManager().addBaseResourcePath(iwad.ToStdString());
				list_base_archive_paths_->Append(iwad);
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void BaseResourceArchivesPanel::init()
{
	// Init paths list
	list_base_archive_paths_->Clear();
	for (size_t a = 0; a < app::archiveManager().numBaseResourcePaths(); a++)
		list_base_archive_paths_->Append(app::archiveManager().getBaseResourcePath(a));

	// Select the currently open base archive if any
	if (base_resource >= 0)
		list_base_archive_paths_->Select(base_resource);

	flp_zdoom_pk3_->setLocation(zdoom_pk3_path);
}

// -----------------------------------------------------------------------------
// Applies preferences from the panel controls
// -----------------------------------------------------------------------------
void BaseResourceArchivesPanel::applyPreferences()
{
	app::archiveManager().openBaseResource(selectedPathIndex());
	zdoom_pk3_path = wxutil::strToView(flp_zdoom_pk3_->location());
}


// -----------------------------------------------------------------------------
//
// BaseResourceArchivesPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Add Archive' button is clicked
// -----------------------------------------------------------------------------
void BaseResourceArchivesPanel::onBtnAdd(wxCommandEvent& e)
{
	// Create extensions string
	wxString extensions = app::archiveManager().getArchiveExtensionsString();

	// Open a file browser dialog that allows multiple selection
	wxFileDialog dialog_open(
		this,
		"Choose file(s) to open",
		dir_last,
		wxEmptyString,
		extensions,
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
			if (app::archiveManager().addBaseResourcePath(file.ToStdString()))
				list_base_archive_paths_->Append(file);
		}

		// Save 'dir_last'
		dir_last = wxutil::strToView(dialog_open.GetDirectory());
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Remove Archive' button is clicked
// -----------------------------------------------------------------------------
void BaseResourceArchivesPanel::onBtnRemove(wxCommandEvent& e)
{
	// Get the selected item index and remove it
	int index = list_base_archive_paths_->GetSelection();
	list_base_archive_paths_->Delete(index);

	// Also remove it from the archive manager
	app::archiveManager().removeBaseResourcePath(index);
}
