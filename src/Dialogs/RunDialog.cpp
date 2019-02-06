
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    RunDialog.cpp
// Description: Allows selection of a game executable and configuration to run
//              an archive (map optional) and selected resource archives
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
#include "RunDialog.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
#include "General/Executables.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "UI/Controls/ResourceArchiveChooser.h"
#include "Utility/SFileDialog.h"

#ifdef __WXOSX_MAC__
#include <CoreFoundation/CoreFoundation.h>
#include <wx/osx/core/cfstring.h>
#endif // __WXOSX_MAC__


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, run_last_exe, "", CVar::Flag::Save)
CVAR(Int, run_last_config, 0, CVar::Flag::Save)
CVAR(String, run_last_extra, "", CVar::Flag::Save)
CVAR(Bool, run_start_3d, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Helper function to get the actual path of game executable [exe], with special
// handling for macOS .apps
// -----------------------------------------------------------------------------
wxString getExecutablePath(const Executables::GameExe* const exe)
{
	const wxString& exe_path = exe->path;

#ifdef __WXOSX_MAC__
	if (exe_path.EndsWith(".app"))
	{
#define CF_CHECK_NULL(VAR) \
	if (NULL == VAR)       \
		return exe_path;

		const wxCFStringRef cf_path(
			CFStringCreateWithCString(kCFAllocatorDefault, exe_path.utf8_str(), kCFStringEncodingUTF8));
		CF_CHECK_NULL(cf_path);

		typedef wxCFRef<CFURLRef> wxCFURLRef;

		const wxCFURLRef cf_path_url(
			CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cf_path, kCFURLPOSIXPathStyle, true));
		CF_CHECK_NULL(cf_path_url);

		const wxCFRef<CFBundleRef> cf_bundle(CFBundleCreate(0, cf_path_url));
		CF_CHECK_NULL(cf_bundle);

		const wxCFURLRef cf_relative_url(CFBundleCopyExecutableURL(cf_bundle));
		CF_CHECK_NULL(cf_relative_url);

		const wxCFURLRef cf_absolute_url(CFURLCopyAbsoluteURL(cf_relative_url));
		CF_CHECK_NULL(cf_absolute_url);

		const wxCFStringRef cf_exe_path(CFURLCopyFileSystemPath(cf_absolute_url, kCFURLPOSIXPathStyle));
		return wxCFStringRef::AsStringWithNormalizationFormC(cf_exe_path);

#undef CF_CHECK_NULL
	}
#endif // __WXOSX_MAC__

	return exe_path;
}
} // namespace


// -----------------------------------------------------------------------------
// RunConfigDialog Class
//
// Simple dialog for creating a run configuration (name and parameters)
// -----------------------------------------------------------------------------
class RunConfigDialog : public wxDialog
{
public:
	RunConfigDialog(
		wxWindow*       parent,
		const wxString& title,
		const wxString& name,
		const wxString& params,
		bool            custom = true) :
		wxDialog(parent, -1, title)
	{
		// Setup sizer
		auto sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		auto gb_sizer = new wxGridBagSizer(UI::padLarge(), UI::pad());
		sizer->Add(gb_sizer, 1, wxEXPAND | wxALL, UI::padLarge());

		// Config name
		gb_sizer->Add(
			new wxStaticText(this, -1, "Config Name:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		text_name_ = new wxTextCtrl(this, -1, name);
		text_name_->Enable(custom);
		gb_sizer->Add(text_name_, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

		// Config params
		gb_sizer->Add(
			new wxStaticText(this, -1, "Parameters:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		text_params_ = new wxTextCtrl(this, -1, params);
		gb_sizer->Add(text_params_, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

		wxStaticText* label_help = new wxStaticText(this, -1, "");
		gb_sizer->Add(label_help, wxGBPosition(2, 0), wxGBSpan(1, 2), wxEXPAND);

		gb_sizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), wxGBPosition(3, 0), wxGBSpan(1, 2), wxALIGN_RIGHT);
		gb_sizer->AddGrowableCol(1);
		gb_sizer->AddGrowableRow(2);

		label_help->SetLabel(
			"%i - Base resource archive\n%r - Resource archive(s)\n%a - Current archive\n%mn - Map name\n"
			"%mw - Map number (eg. E1M1 = 1 1, MAP02 = 02)");
		label_help->Wrap(UI::scalePx(300));
		text_params_->SetInsertionPoint(0);
	}
	~RunConfigDialog() = default;

	wxString name() const { return text_name_->GetValue(); }
	wxString params() const { return text_params_->GetValue(); }

private:
	wxTextCtrl* text_name_;
	wxTextCtrl* text_params_;
};


// -----------------------------------------------------------------------------
//
// RunDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// RunDialog class constructor
// -----------------------------------------------------------------------------
RunDialog::RunDialog(wxWindow* parent, Archive* archive, bool show_start_3d_cb) :
	SDialog(parent, "Run", "run", 500, 400)
{
	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	sizer->Add(gb_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, UI::padLarge());

	// Game Executable
	gb_sizer->Add(
		new wxStaticText(this, -1, "Game Executable:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	choice_game_exes_ = new wxChoice(this, -1);
	gb_sizer->Add(choice_game_exes_, wxGBPosition(0, 1), wxGBSpan(1, 2), wxEXPAND);
	btn_add_game_ = new wxBitmapButton(this, -1, Icons::getIcon(Icons::General, "plus"));
	gb_sizer->Add(btn_add_game_, wxGBPosition(0, 3));
	btn_remove_game_ = new wxBitmapButton(this, -1, Icons::getIcon(Icons::General, "minus"));
	gb_sizer->Add(btn_remove_game_, wxGBPosition(0, 4));

	// Executable path
	gb_sizer->Add(new wxStaticText(this, -1, "Path:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	text_exe_path_ = new wxTextCtrl(this, -1, "");
	text_exe_path_->Enable(false);
	gb_sizer->Add(text_exe_path_, wxGBPosition(1, 1), wxGBSpan(1, 3), wxEXPAND);
	btn_browse_exe_ = new wxBitmapButton(this, -1, Icons::getIcon(Icons::General, "open"));
	btn_browse_exe_->SetToolTip("Browse...");
	gb_sizer->Add(btn_browse_exe_, wxGBPosition(1, 4));

	// Configuration
	gb_sizer->Add(
		new wxStaticText(this, -1, "Run Configuration:"), wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	choice_config_ = new wxChoice(this, -1);
	gb_sizer->Add(choice_config_, wxGBPosition(2, 1), wxDefaultSpan, wxEXPAND);
	btn_edit_config_ = new wxBitmapButton(this, -1, Icons::getIcon(Icons::General, "settings"));
	btn_edit_config_->SetToolTip("Edit command line");
	gb_sizer->Add(btn_edit_config_, wxGBPosition(2, 2));
	btn_add_config_ = new wxBitmapButton(this, -1, Icons::getIcon(Icons::General, "plus"));
	gb_sizer->Add(btn_add_config_, wxGBPosition(2, 3));
	btn_remove_config_ = new wxBitmapButton(this, -1, Icons::getIcon(Icons::General, "minus"));
	btn_remove_config_->Enable(false);
	gb_sizer->Add(btn_remove_config_, wxGBPosition(2, 4));

	// Extra parameters
	gb_sizer->Add(
		new wxStaticText(this, -1, "Extra Parameters:"), wxGBPosition(3, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	text_extra_params_ = new wxTextCtrl(this, -1, run_last_extra);
	gb_sizer->Add(text_extra_params_, wxGBPosition(3, 1), wxGBSpan(1, 4), wxEXPAND);

	// Resources
	auto frame      = new wxStaticBox(this, -1, "Resources");
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->AddSpacer(UI::padLarge());
	sizer->Add(framesizer, 1, wxEXPAND | wxLEFT | wxRIGHT, UI::padLarge());
	rac_resources_ = new ResourceArchiveChooser(this, archive);
	framesizer->Add(rac_resources_, 1, wxEXPAND | wxALL, UI::pad());

	// Start from 3d mode camera
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->AddSpacer(UI::padLarge());
	sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::padLarge());
	cb_start__3_d_ = new wxCheckBox(this, -1, "Start from 3D mode camera position");
	cb_start__3_d_->SetValue(run_start_3d);
	if (show_start_3d_cb)
		hbox->Add(cb_start__3_d_, 1, wxALIGN_CENTER_VERTICAL);
	else
	{
		hbox->AddStretchSpacer();
		cb_start__3_d_->Show(false);
	}

	// Dialog buttons
	btn_run_ = new wxButton(this, wxID_OK, "Run");
	btn_run_->SetDefault();
	hbox->Add(btn_run_, 0, wxEXPAND | wxRIGHT, UI::pad());

	btn_cancel_ = new wxButton(this, wxID_CANCEL, "Cancel");
	hbox->Add(btn_cancel_, 0, wxEXPAND);

	// Populate game executables dropdown
	int last_index = -1;
	for (unsigned a = 0; a < Executables::nGameExes(); a++)
	{
		auto exe = Executables::gameExe(a);
		choice_game_exes_->AppendString(exe->name);

		if (exe->id == run_last_exe)
			last_index = choice_game_exes_->GetCount() - 1;
	}
	if ((int)choice_game_exes_->GetCount() > last_index)
	{
		choice_game_exes_->Select(last_index);
		openGameExe(last_index);
		choice_config_->Select(run_last_config);
	}

	// Bind Events
	btn_add_game_->Bind(wxEVT_BUTTON, &RunDialog::onBtnAddGame, this);
	btn_remove_game_->Bind(wxEVT_BUTTON, &RunDialog::onBtnRemoveGame, this);
	btn_browse_exe_->Bind(wxEVT_BUTTON, &RunDialog::onBtnBrowseExe, this);
	btn_edit_config_->Bind(wxEVT_BUTTON, &RunDialog::onBtnEditConfig, this);
	btn_add_config_->Bind(wxEVT_BUTTON, &RunDialog::onBtnAddConfig, this);
	btn_remove_config_->Bind(wxEVT_BUTTON, &RunDialog::onBtnRemoveConfig, this);
	btn_run_->Bind(wxEVT_BUTTON, &RunDialog::onBtnRun, this);
	btn_cancel_->Bind(wxEVT_BUTTON, &RunDialog::onBtnCancel, this);
	choice_game_exes_->Bind(wxEVT_CHOICE, &RunDialog::onChoiceGameExe, this);
	choice_config_->Bind(wxEVT_CHOICE, &RunDialog::onChoiceConfig, this);

	gb_sizer->AddGrowableCol(1, 1);
	wxTopLevelWindowBase::SetMinSize(wxSize(UI::scalePx(500), UI::scalePx(400)));
	wxWindowBase::Layout();
	CenterOnParent();
	btn_run_->SetFocusFromKbd();
}

// -----------------------------------------------------------------------------
// RunDialog class destructor
// -----------------------------------------------------------------------------
RunDialog::~RunDialog()
{
	run_start_3d = cb_start__3_d_->GetValue();
}

// -----------------------------------------------------------------------------
// Loads run configurations and sets up controls for game exe [index]
// -----------------------------------------------------------------------------
void RunDialog::openGameExe(unsigned index) const
{
	// Clear
	choice_config_->Clear();
	text_exe_path_->SetValue("");

	// Populate configs
	auto exe = Executables::gameExe(index);
	if (exe)
	{
		for (auto& config : exe->configs)
			choice_config_->AppendString(config.first);

		text_exe_path_->SetValue(exe->path);
		btn_remove_game_->Enable(exe->custom);
		if (choice_config_->GetCount() == 0)
			btn_edit_config_->Enable(false);
		else
		{
			choice_config_->SetSelection(0);
			btn_edit_config_->Enable();
			btn_remove_config_->Enable(exe->configs_custom[0]);
		}
	}
}

// -----------------------------------------------------------------------------
// Returns a command line based on the currently selected run configuration and
// resources
// -----------------------------------------------------------------------------
wxString RunDialog::selectedCommandLine(Archive* archive, const wxString& map_name, const wxString& map_file) const
{
	auto exe = Executables::gameExe(choice_game_exes_->GetSelection());
	if (exe)
	{
		// Get exe path
		const wxString exe_path = getExecutablePath(exe);

		if (exe_path.IsEmpty())
			return "";

		wxString path = wxString::Format("\"%s\"", exe_path);

		unsigned cfg = choice_config_->GetSelection();
		if (cfg < exe->configs.size())
		{
			path += " ";
			path += exe->configs[cfg].second;
		}

		// IWAD
		auto bra = App::archiveManager().baseResourceArchive();
		path.Replace("%i", wxString::Format("\"%s\"", bra ? bra->filename() : ""));

		// Resources
		path.Replace("%r", selectedResourceList());

		// Archive (+ temp map if specified)
		if (map_file.IsEmpty() && archive)
			path.Replace("%a", wxString::Format("\"%s\"", archive->filename()));
		else
		{
			if (archive)
				path.Replace("%a", wxString::Format("\"%s\" \"%s\"", archive->filename(), map_file));
			else
				path.Replace("%a", wxString::Format("\"%s\"", map_file));
		}

		// Running an archive yields no map name, so don't try to warp
		if (map_name.IsEmpty())
		{
			path.Replace("-warp ", wxEmptyString);
			path.Replace("+map ", wxEmptyString);
			path.Replace("%mn", wxEmptyString);
			path.Replace("%mw", wxEmptyString);
		}
		// Map name
		else
		{
			path.Replace("%mn", map_name);

			// Map warp
			if (path.Contains("%mw"))
			{
				wxString mn = map_name.Lower();

				// MAPxx
				wxString mapnum;
				if (mn.StartsWith("map", &mapnum))
					path.Replace("%mw", mapnum);

				// ExMx
				else if (map_name.length() == 4 && mn[0] == 'e' && mn[2] == 'm')
					path.Replace("%mw", wxString::Format("%c %c", mn[1], mn[3]));
			}
		}

		// Extra parameters
		if (!text_extra_params_->GetValue().IsEmpty())
		{
			path += " ";
			path += text_extra_params_->GetValue();
		}

		Log::info(2, wxString::Format("Run command: %s", path));
		return path;
	}

	return "";
}

// -----------------------------------------------------------------------------
// Returns a space-separated list of selected resource archive filenames
// -----------------------------------------------------------------------------
wxString RunDialog::selectedResourceList() const
{
	return rac_resources_->selectedResourceList();
}

// -----------------------------------------------------------------------------
// Returns the directory of the currently selected executable
// -----------------------------------------------------------------------------
wxString RunDialog::selectedExeDir() const
{
	auto exe = Executables::gameExe(choice_game_exes_->GetSelection());
	if (exe)
	{
		wxFileName fn(exe->path);
		return fn.GetPath(wxPATH_GET_VOLUME);
	}

	return "";
}

// -----------------------------------------------------------------------------
// Returns the id of the currently selected game executable
// -----------------------------------------------------------------------------
wxString RunDialog::selectedExeId() const
{
	auto exe = Executables::gameExe(choice_game_exes_->GetSelection());
	if (exe)
		return exe->id;
	else
		return "";
}

// -----------------------------------------------------------------------------
// Returns true if 'Start from 3D mode camera position' checkbox is checked
// -----------------------------------------------------------------------------
bool RunDialog::start3dModeChecked() const
{
	return cb_start__3_d_->GetValue();
}


// -----------------------------------------------------------------------------
//
// RunDialog Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the add game button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnAddGame(wxCommandEvent& e)
{
	wxString name = wxGetTextFromUser("Enter a name for the game executable");
	Executables::addGameExe(name);
	choice_game_exes_->AppendString(name);
	choice_game_exes_->Select(choice_game_exes_->GetCount() - 1);
	openGameExe(Executables::nGameExes() - 1);
}

// -----------------------------------------------------------------------------
// Called when the browse button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnBrowseExe(wxCommandEvent& e)
{
	auto exe = Executables::gameExe(choice_game_exes_->GetSelection());

	if (exe)
	{
		SFileDialog::FDInfo info;
#ifdef WIN32
		if (SFileDialog::openFile(
				info, "Browse for game executable", "Executable files (*.exe)|*.exe;*.bat", this, exe->exe_name))
#else
		if (SFileDialog::openFile(
				info, "Browse for game executable", wxFileSelectorDefaultWildcardStr, this, exe->exe_name))
#endif
		{
			text_exe_path_->SetValue(info.filenames[0]);
			exe->path = info.filenames[0];
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the add config button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnAddConfig(wxCommandEvent& e)
{
	if (choice_game_exes_->GetSelection() < 0)
		return;

	auto     exe         = Executables::gameExe(choice_game_exes_->GetSelection());
	wxString init_params = "";
	if (choice_config_->GetSelection() >= 0)
		init_params = exe->configs[choice_config_->GetSelection()].second;

	RunConfigDialog dlg(this, wxString::Format("Add Run Config for %s", exe->name), "", init_params);
	if (dlg.ShowModal() == wxID_OK)
	{
		wxString name = dlg.name();

		if (name.IsEmpty())
			name = wxString::Format("Config %d", choice_config_->GetCount() + 1);

		Executables::addGameExeConfig(choice_game_exes_->GetSelection(), name, dlg.params());
		choice_config_->AppendString(name);
		choice_config_->Select(choice_config_->GetCount() - 1);
	}
}

// -----------------------------------------------------------------------------
// Called when the edit config button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnEditConfig(wxCommandEvent& e)
{
	if (choice_game_exes_->GetSelection() < 0 || choice_config_->GetSelection() < 0)
		return;

	auto     exe    = Executables::gameExe(choice_game_exes_->GetSelection());
	int      index  = choice_config_->GetSelection();
	wxString name   = exe->configs[index].first;
	wxString params = exe->configs[index].second;
	bool     custom = exe->configs_custom[index];

	RunConfigDialog dlg(this, "Edit Run Config", name, params, custom);
	if (dlg.ShowModal() == wxID_OK)
	{
		wxString name              = dlg.name().IsEmpty() ? exe->configs[index].first : dlg.name();
		exe->configs[index].first  = name;
		exe->configs[index].second = dlg.params();
		choice_config_->SetString(index, name);
	}
}

// -----------------------------------------------------------------------------
// Called when the run button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnRun(wxCommandEvent& e)
{
	if (text_exe_path_->GetValue().empty()
		|| (!wxFileExists(text_exe_path_->GetValue())
#ifdef __WXOSX_MAC__
			&& !(text_exe_path_->GetValue().EndsWith(".app"))
#endif
				))
	{
		wxMessageBox("Invalid executable path", "Error", wxICON_ERROR);
		return;
	}

	// Update cvars
	run_last_extra  = text_extra_params_->GetValue();
	run_last_config = choice_config_->GetSelection();
	run_last_exe    = selectedExeId();

	EndModal(wxID_OK);
}

// -----------------------------------------------------------------------------
// Called when the cancel button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnCancel(wxCommandEvent& e)
{
	// Update cvars
	run_last_extra  = text_extra_params_->GetValue();
	run_last_config = choice_config_->GetSelection();
	run_last_exe    = selectedExeId();

	EndModal(wxID_CANCEL);
}

// -----------------------------------------------------------------------------
// Called when the game executable dropdown selection changes
// -----------------------------------------------------------------------------
void RunDialog::onChoiceGameExe(wxCommandEvent& e)
{
	openGameExe(e.GetSelection());
	run_last_exe = selectedExeId();
}

// -----------------------------------------------------------------------------
// Called when the run configuration dropdown selection changes
// -----------------------------------------------------------------------------
void RunDialog::onChoiceConfig(wxCommandEvent& e)
{
	run_last_config = choice_config_->GetSelection();
	btn_edit_config_->Enable(true);
	auto exe = Executables::gameExe(choice_game_exes_->GetSelection());
	btn_remove_config_->Enable(exe->configs_custom[choice_config_->GetSelection()]);
}

// -----------------------------------------------------------------------------
// Called when the remove game button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnRemoveGame(wxCommandEvent& e)
{
	if (Executables::removeGameExe(choice_game_exes_->GetSelection()))
	{
		choice_game_exes_->Clear();
		for (unsigned a = 0; a < Executables::nGameExes(); a++)
			choice_game_exes_->AppendString(Executables::gameExe(a)->name);

		if (choice_game_exes_->GetCount() > 0)
		{
			choice_game_exes_->Select(0);
			openGameExe(0);
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the remove config button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnRemoveConfig(wxCommandEvent& e)
{
	if (Executables::removeGameExeConfig(choice_game_exes_->GetSelection(), choice_config_->GetSelection()))
		openGameExe(choice_game_exes_->GetSelection());
}
