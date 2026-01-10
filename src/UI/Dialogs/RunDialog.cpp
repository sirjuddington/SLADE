
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
#include "Database/Tables/ArchiveRunConfig.h"
#include "General/Executables.h"
#include "UI/Controls/ResourceArchiveChooser.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Layout.h"
#include "UI/State.h"
#include "UI/WxUtils.h"
#include "Utility/FileUtils.h"
#include "Utility/SFileDialog.h"
#include "Utility/StringUtils.h"

#ifdef __WXOSX_MAC__
#include <CoreFoundation/CoreFoundation.h>
#include <wx/osx/core/cfstring.h>
#endif // __WXOSX_MAC__

using namespace slade;


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
string getExecutablePath(const executables::GameExe* const exe)
{
#ifdef __WXOSX_MAC__
	if (strutil::endsWithCI(exe->path, ".app"))
	{
#define CF_CHECK_NULL(VAR) \
	if (NULL == VAR)       \
		return exe->path;

		const wxCFStringRef cf_path(
			CFStringCreateWithCString(kCFAllocatorDefault, exe->path.c_str(), kCFStringEncodingUTF8));
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
		return wxCFStringRef::AsStringWithNormalizationFormC(cf_exe_path).utf8_string();

#undef CF_CHECK_NULL
	}
#endif // __WXOSX_MAC__

	return exe->path;
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
		wxWindow*     parent,
		const string& title,
		const string& name,
		const string& params,
		bool          custom = true) :
		wxDialog(parent, -1, wxString::FromUTF8(title))
	{
		auto lh = ui::LayoutHelper(this);

		// Setup sizer
		auto sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		auto gb_sizer = new wxGridBagSizer(lh.padLarge(), lh.pad());
		sizer->Add(gb_sizer, 1, wxEXPAND | wxALL, lh.padLarge());

		// Config name
		gb_sizer->Add(
			new wxStaticText(this, -1, wxS("Config Name:")),
			wxGBPosition(0, 0),
			wxDefaultSpan,
			wxALIGN_CENTER_VERTICAL);
		text_name_ = new wxTextCtrl(this, -1, wxString::FromUTF8(name));
		text_name_->Enable(custom);
		gb_sizer->Add(text_name_, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

		// Config params
		gb_sizer->Add(
			new wxStaticText(this, -1, wxS("Parameters:")), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		text_params_ = new wxTextCtrl(this, -1, wxString::FromUTF8(params));
		gb_sizer->Add(text_params_, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

		wxStaticText* label_help = new wxStaticText(this, -1, wxEmptyString);
		gb_sizer->Add(label_help, wxGBPosition(2, 0), wxGBSpan(1, 2), wxEXPAND);

		gb_sizer->Add(wxutil::createDialogButtonBox(this), wxGBPosition(3, 0), wxGBSpan(1, 2), wxALIGN_RIGHT);
		gb_sizer->AddGrowableCol(1);
		gb_sizer->AddGrowableRow(2);

		label_help->SetLabel(
			wxS("%i - Base resource archive\n%r - Resource archive(s)\n%a - Current archive\n%mn - Map name\n"
				"%mw - Map number (eg. E1M1 = 1 1, MAP02 = 02)"));
		label_help->Wrap(FromDIP(300));
		text_params_->SetInsertionPoint(0);
	}
	~RunConfigDialog() override = default;

	string name() const { return text_name_->GetValue().utf8_string(); }
	string params() const { return text_params_->GetValue().utf8_string(); }

private:
	wxTextCtrl* text_name_;
	wxTextCtrl* text_params_;
};


// -----------------------------------------------------------------------------
// IwadSelectionPanel Class
//
// A control to select an IWAD path, with dropdown options showing configured
// base resource archive paths and a button to browse for one
// -----------------------------------------------------------------------------
class IwadSelectionPanel : public wxPanel
{
public:
	IwadSelectionPanel(wxWindow* parent, const string& iwad_path) : wxPanel(parent, -1)
	{
		auto lh = ui::LayoutHelper(this);

		auto sizer = new wxBoxSizer(wxHORIZONTAL);
		SetSizer(sizer);
		combo_iwad_path_ = new wxComboBox(this, -1, wxString::FromUTF8(iwad_path));
		sizer->Add(combo_iwad_path_, wxSizerFlags(1).Expand());

		// Add base resource paths to dropdown
		for (const auto& br_path : app::archiveManager().baseResourcePaths())
			combo_iwad_path_->AppendString(wxString::FromUTF8(br_path));

		btn_browse_ = new SIconButton(this, "open", "Browse IWAD path");
		sizer->Add(btn_browse_, lh.sfWithBorder(0, wxLEFT).Expand());

		btn_browse_->Bind(
			wxEVT_BUTTON,
			[&](wxCommandEvent& e)
			{
				filedialog::FDInfo inf;
				if (filedialog::openFile(
						inf, "Browse IWAD path", app::archiveManager().getArchiveExtensionsString(), this))
				{
					combo_iwad_path_->SetValue(wxString::FromUTF8(inf.filenames[0]));
				}
			});
	}

	string selectedIwadPath() const { return combo_iwad_path_->GetValue().utf8_string(); }

private:
	wxComboBox*  combo_iwad_path_ = nullptr;
	SIconButton* btn_browse_      = nullptr;
};


// -----------------------------------------------------------------------------
//
// RunDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// RunDialog class constructor
// -----------------------------------------------------------------------------
RunDialog::RunDialog(wxWindow* parent, const Archive* archive, bool show_start_3d_cb, bool run_map) :
	SDialog(parent, "Run", "run", 500, 400),
	run_map_{ run_map }
{
	auto lh = ui::LayoutHelper(this);

	// Set dialog icon + title
	wxutil::setWindowIcon(this, "run");
	if (run_map)
		SetTitle(wxS("Run Map"));
	if (archive)
		SetTitle(WX_FMT("Run Archive - {}", archive->filename(false)));

	// Get initial values
	auto run_last_extra = ui::getStateString("RunDialogLastExtra");
	auto run_last_exe   = ui::getStateString("RunDialogLastExe");
	auto run_last_cfg   = ui::getStateInt("RunDialogLastConfig");
	auto iwad_path      = app::archiveManager().currentBaseResourcePath();
	auto db_id          = archive ? app::archiveManager().archiveDbId(*archive) : -1;
	if (db_id >= 0)
	{
		auto run_cfg = database::getArchiveRunConfig(db_id);
		if (run_cfg.archive_id >= 0)
		{
			run_last_exe   = run_cfg.executable_id;
			run_last_cfg   = run_cfg.run_config;
			run_last_extra = run_cfg.run_extra;
			iwad_path      = run_cfg.iwad_path;
		}
	}

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto gb_sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	sizer->Add(gb_sizer, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxTOP).Expand());

	// Game Executable
	gb_sizer->Add(
		new wxStaticText(this, -1, wxS("Game Executable:")),
		wxGBPosition(0, 0),
		wxDefaultSpan,
		wxALIGN_CENTER_VERTICAL);
	choice_game_exes_ = new wxChoice(this, -1);
	gb_sizer->Add(choice_game_exes_, wxGBPosition(0, 1), wxGBSpan(1, 2), wxEXPAND | wxALIGN_CENTER_VERTICAL);
	btn_add_game_ = new SIconButton(this, icons::General, "plus");
	gb_sizer->Add(btn_add_game_, wxGBPosition(0, 3));
	btn_remove_game_ = new SIconButton(this, icons::General, "minus");
	gb_sizer->Add(btn_remove_game_, wxGBPosition(0, 4));

	// Executable path
	gb_sizer->Add(new wxStaticText(this, -1, wxS("Path:")), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	text_exe_path_ = new wxTextCtrl(this, -1, wxEmptyString);
	// text_exe_path_->Enable(false);
	gb_sizer->Add(text_exe_path_, wxGBPosition(1, 1), wxGBSpan(1, 3), wxEXPAND | wxALIGN_CENTER_VERTICAL);
	btn_browse_exe_ = new SIconButton(this, icons::General, "open");
	btn_browse_exe_->SetToolTip(wxS("Browse..."));
	gb_sizer->Add(btn_browse_exe_, wxGBPosition(1, 4));

	// Configuration
	gb_sizer->Add(
		new wxStaticText(this, -1, wxS("Run Configuration:")),
		wxGBPosition(2, 0),
		wxDefaultSpan,
		wxALIGN_CENTER_VERTICAL);
	choice_config_ = new wxChoice(this, -1);
	gb_sizer->Add(choice_config_, wxGBPosition(2, 1), wxDefaultSpan, wxEXPAND | wxALIGN_CENTER_VERTICAL);
	btn_edit_config_ = new SIconButton(this, icons::General, "settings");
	btn_edit_config_->SetToolTip(wxS("Edit command line"));
	gb_sizer->Add(btn_edit_config_, wxGBPosition(2, 2));
	btn_add_config_ = new SIconButton(this, icons::General, "plus");
	gb_sizer->Add(btn_add_config_, wxGBPosition(2, 3));
	btn_remove_config_ = new SIconButton(this, icons::General, "minus");
	btn_remove_config_->Enable(false);
	gb_sizer->Add(btn_remove_config_, wxGBPosition(2, 4));

	// Extra parameters
	gb_sizer->Add(
		new wxStaticText(this, -1, wxS("Extra Parameters:")),
		wxGBPosition(3, 0),
		wxDefaultSpan,
		wxALIGN_CENTER_VERTICAL);
	text_extra_params_ = new wxTextCtrl(this, -1, wxString::FromUTF8(run_last_extra));
	gb_sizer->Add(text_extra_params_, wxGBPosition(3, 1), wxGBSpan(1, 4), wxEXPAND);

	// Resources
	auto frame      = new wxStaticBox(this, -1, wxS("Resources"));
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->AddSpacer(lh.padLarge());
	sizer->Add(framesizer, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT).Expand());
	isp_iwad_ = new IwadSelectionPanel(frame, iwad_path);
	framesizer->Add(
		wxutil::createLabelHBox(frame, "IWAD:", isp_iwad_), lh.sfWithBorder(0, wxTOP | wxLEFT | wxRIGHT).Expand());
	rac_resources_ = new ResourceArchiveChooser(frame, archive);
	framesizer->Add(rac_resources_, lh.sfWithBorder(1).Expand());

	// Start from 3d mode camera
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->AddSpacer(lh.padLarge());
	sizer->Add(hbox, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
	cb_start_3d_ = new wxCheckBox(this, -1, wxS("Start from 3D mode camera position"));
	cb_start_3d_->SetValue(ui::getStateBool(ui::RUNDIALOG_START_3D));
	if (show_start_3d_cb)
		hbox->Add(cb_start_3d_, wxSizerFlags().CenterVertical());
	else
		cb_start_3d_->Show(false);

	// Dialog buttons
	btn_run_ = new wxButton(this, wxID_OK, wxS("Run"));
	btn_run_->SetDefault();
	btn_cancel_ = new wxButton(this, wxID_CANCEL, wxS("Cancel"));
	hbox->Add(wxutil::createDialogButtonBox(btn_run_, btn_cancel_), wxSizerFlags(1).Expand());

	// Populate game executables dropdown
	int last_index = -1;
	for (unsigned a = 0; a < executables::nGameExes(); a++)
	{
		auto exe = executables::gameExe(a);
		choice_game_exes_->AppendString(wxString::FromUTF8(exe->name));

		if (exe->id == run_last_exe)
			last_index = choice_game_exes_->GetCount() - 1;
	}
	if (static_cast<int>(choice_game_exes_->GetCount()) > last_index)
	{
		choice_game_exes_->Select(last_index);
		openGameExe(last_index);
		choice_config_->Select(run_last_cfg);
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
	wxTopLevelWindowBase::SetMinSize(lh.size(500, 400));
	wxWindowBase::Layout();
	CenterOnParent();
	btn_run_->SetFocusFromKbd();
}

// -----------------------------------------------------------------------------
// RunDialog class destructor
// -----------------------------------------------------------------------------
RunDialog::~RunDialog()
{
	ui::saveStateBool(ui::RUNDIALOG_START_3D, cb_start_3d_->GetValue());
}

// -----------------------------------------------------------------------------
// Loads run configurations and sets up controls for game exe [index]
// -----------------------------------------------------------------------------
void RunDialog::openGameExe(unsigned index) const
{
	// Clear
	choice_config_->Clear();
	text_exe_path_->SetValue(wxEmptyString);

	// Populate configs
	auto exe = executables::gameExe(index);
	if (exe)
	{
		const auto& configs        = run_map_ ? exe->map_configs : exe->run_configs;
		const auto& configs_custom = run_map_ ? exe->map_configs_custom : exe->run_configs_custom;

		for (auto& config : configs)
			choice_config_->AppendString(wxString::FromUTF8(config.first));

		text_exe_path_->SetValue(wxString::FromUTF8(exe->path));
		btn_remove_game_->Enable(exe->custom);
		if (choice_config_->GetCount() == 0)
			btn_edit_config_->Enable(false);
		else
		{
			choice_config_->SetSelection(0);
			btn_edit_config_->Enable();
			btn_remove_config_->Enable(configs_custom[0]);
		}
	}
}

// -----------------------------------------------------------------------------
// Returns a command line based on the currently selected run configuration and
// resources
// -----------------------------------------------------------------------------
string RunDialog::selectedCommandLine(const Config& cfg) const
{
	auto exe = executables::gameExe(choice_game_exes_->GetSelection());
	if (exe)
	{
		// Get exe path
		const auto exe_path = getExecutablePath(exe);

		if (exe_path.empty())
			return "";

		auto path = fmt::format("\"{}\"", exe_path);

		unsigned    cfg_index = choice_config_->GetSelection();
		const auto& configs   = run_map_ ? exe->map_configs : exe->run_configs;
		if (cfg_index < configs.size())
		{
			path += " ";
			path += configs[cfg_index].second;
		}

		// IWAD
		auto iwad_path = cfg.iwad_path;
		if (iwad_path.empty())
			iwad_path = dynamic_cast<IwadSelectionPanel*>(isp_iwad_)->selectedIwadPath();
		strutil::replaceIP(path, "%i", fmt::format("\"{}\"", iwad_path));

		// Resources
		strutil::replaceIP(path, "%r", selectedResourceList());

		// Archive (+ temp map if specified)
		if (cfg.map_file.empty() && !cfg.archive_path.empty())
			strutil::replaceIP(path, "%a", fmt::format("\"{}\"", cfg.archive_path));
		else
		{
			if (!cfg.archive_path.empty())
				strutil::replaceIP(path, "%a", fmt::format(R"("{}" "{}")", cfg.archive_path, cfg.map_file));
			else
				strutil::replaceIP(path, "%a", fmt::format("\"{}\"", cfg.map_file));
		}

		// Running an archive yields no map name, so don't try to warp
		if (cfg.map_name.empty())
		{
			strutil::replaceIP(path, "-warp ", "");
			strutil::replaceIP(path, "+map ", "");
			strutil::replaceIP(path, "%mn", "");
			strutil::replaceIP(path, "%mw", "");
		}
		// Map name
		else
		{
			strutil::replaceIP(path, "%mn", cfg.map_name);

			// Map warp
			if (strutil::contains(path, "%mw"))
			{
				auto mn = strutil::lower(cfg.map_name);

				// MAPxx
				if (strutil::startsWith(mn, "map"))
					strutil::replaceIP(path, "%mw", mn.substr(3));

				// ExMx
				else if (mn.length() == 4 && mn[0] == 'e' && mn[2] == 'm')
					strutil::replaceIP(path, "%mw", fmt::format("{} {}", mn[1], mn[3]));
			}
		}

		// Extra parameters
		if (!text_extra_params_->GetValue().IsEmpty())
		{
			path += " ";
			path += text_extra_params_->GetValue().utf8_string();
		}

		log::info(2, "Run command: {}", path);
		return path;
	}

	return "";
}

// -----------------------------------------------------------------------------
// Returns a space-separated list of selected resource archive filenames
// -----------------------------------------------------------------------------
string RunDialog::selectedResourceList() const
{
	return rac_resources_->selectedResourceList();
}

// -----------------------------------------------------------------------------
// Returns the directory of the currently selected executable
// -----------------------------------------------------------------------------
string RunDialog::selectedExeDir() const
{
	auto exe = executables::gameExe(choice_game_exes_->GetSelection());
	if (exe)
	{
		strutil::Path fn(exe->path);
		return string{ fn.path() };
	}

	return "";
}

// -----------------------------------------------------------------------------
// Returns the id of the currently selected game executable
// -----------------------------------------------------------------------------
string RunDialog::selectedExeId() const
{
	auto exe = executables::gameExe(choice_game_exes_->GetSelection());
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
	return cb_start_3d_->GetValue();
}

// -----------------------------------------------------------------------------
// Runs the archive+iwad+map defined in [cfg] with the currently selected
// executable+run configuration
// -----------------------------------------------------------------------------
void RunDialog::run(const Config& cfg, i64 archive_db_id) const
{
	auto command = selectedCommandLine(cfg);
	if (!command.empty())
	{
		// Save run config for archive
		if (archive_db_id >= 0)
		{
			database::ArchiveRunConfig run_cfg{ archive_db_id };
			run_cfg.executable_id = selectedExeId();
			run_cfg.run_config    = choice_config_->GetSelection();
			run_cfg.run_extra     = text_extra_params_->GetValue().utf8_string();
			run_cfg.iwad_path     = dynamic_cast<IwadSelectionPanel*>(isp_iwad_)->selectedIwadPath();
			run_cfg.write();
		}

		// Set working directory
		wxString wd = wxGetCwd();
		wxSetWorkingDirectory(wxString::FromUTF8(selectedExeDir()));

		// Run
		wxExecute(wxString::FromUTF8(command), wxEXEC_ASYNC);

		// Restore working directory
		wxSetWorkingDirectory(wd);
	}
}


// -----------------------------------------------------------------------------
//
// RunDialog Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the add game button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnAddGame(wxCommandEvent& e)
{
	auto name = wxGetTextFromUser(wxS("Enter a name for the game executable"));
	executables::addGameExe(name.utf8_string());
	choice_game_exes_->AppendString(name);
	choice_game_exes_->Select(choice_game_exes_->GetCount() - 1);
	openGameExe(executables::nGameExes() - 1);
}

// -----------------------------------------------------------------------------
// Called when the browse button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnBrowseExe(wxCommandEvent& e)
{
	auto exe = executables::gameExe(choice_game_exes_->GetSelection());

	if (exe)
	{
		filedialog::FDInfo info;
		if (filedialog::openExecutableFile(info, "Browse for game executable", this, exe->exe_name))
		{
			text_exe_path_->SetValue(wxString::FromUTF8(info.filenames[0]));
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

	auto   exe = executables::gameExe(choice_game_exes_->GetSelection());
	string init_params;
	if (choice_config_->GetSelection() >= 0)
	{
		const auto& configs = run_map_ ? exe->map_configs : exe->run_configs;
		init_params         = configs[choice_config_->GetSelection()].second;
	}

	RunConfigDialog dlg(this, fmt::format("Add Run Config for {}", exe->name), "", init_params);
	if (dlg.ShowModal() == wxID_OK)
	{
		auto name = dlg.name();

		if (name.empty())
			name = fmt::format("Config {}", choice_config_->GetCount() + 1);

		if (run_map_)
			executables::addGameExeMapConfig(choice_game_exes_->GetSelection(), name, dlg.params());
		else
			executables::addGameExeRunConfig(choice_game_exes_->GetSelection(), name, dlg.params());

		choice_config_->AppendString(wxString::FromUTF8(name));
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

	auto   exe            = executables::gameExe(choice_game_exes_->GetSelection());
	int    index          = choice_config_->GetSelection();
	auto&  configs        = run_map_ ? exe->map_configs : exe->run_configs;
	auto&  configs_custom = run_map_ ? exe->map_configs_custom : exe->run_configs_custom;
	string name           = configs[index].first;
	string params         = configs[index].second;
	bool   custom         = configs_custom[index];

	RunConfigDialog dlg(this, "Edit Run Config", name, params, custom);
	if (dlg.ShowModal() == wxID_OK)
	{
		string newname        = dlg.name().empty() ? configs[index].first : dlg.name();
		configs[index].first  = newname;
		configs[index].second = dlg.params();
		choice_config_->SetString(index, wxString::FromUTF8(newname));
	}
}

// -----------------------------------------------------------------------------
// Called when the run button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnRun(wxCommandEvent& e)
{
	// Check game executable path is valid
	auto exe_path = text_exe_path_->GetValue().utf8_string();
	if (!fileutil::validExecutable(exe_path))
	{
		wxMessageBox(wxS("Invalid executable path"), wxS("Error"), wxICON_ERROR);
		return;
	}

	// Update game executable config
	auto exe  = executables::gameExe(choice_game_exes_->GetSelection());
	exe->path = exe_path;

	// Update cvars
	ui::saveStateString(ui::RUNDIALOG_LAST_EXTRA, text_extra_params_->GetValue().utf8_string());
	ui::saveStateInt(ui::RUNDIALOG_LAST_CONFIG, choice_config_->GetSelection());
	ui::saveStateString(ui::RUNDIALOG_LAST_EXE, selectedExeId());

	EndModal(wxID_OK);
}

// -----------------------------------------------------------------------------
// Called when the cancel button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnCancel(wxCommandEvent& e)
{
	// Update cvars
	ui::saveStateString(ui::RUNDIALOG_LAST_EXTRA, text_extra_params_->GetValue().utf8_string());
	ui::saveStateInt(ui::RUNDIALOG_LAST_CONFIG, choice_config_->GetSelection());
	ui::saveStateString(ui::RUNDIALOG_LAST_EXE, selectedExeId());

	EndModal(wxID_CANCEL);
}

// -----------------------------------------------------------------------------
// Called when the game executable dropdown selection changes
// -----------------------------------------------------------------------------
void RunDialog::onChoiceGameExe(wxCommandEvent& e)
{
	openGameExe(e.GetSelection());
	ui::saveStateString(ui::RUNDIALOG_LAST_EXE, selectedExeId());
}

// -----------------------------------------------------------------------------
// Called when the run configuration dropdown selection changes
// -----------------------------------------------------------------------------
void RunDialog::onChoiceConfig(wxCommandEvent& e)
{
	ui::saveStateInt(ui::RUNDIALOG_LAST_CONFIG, choice_config_->GetSelection());
	btn_edit_config_->Enable(true);
	auto        exe            = executables::gameExe(choice_game_exes_->GetSelection());
	const auto& configs_custom = run_map_ ? exe->map_configs_custom : exe->run_configs_custom;
	btn_remove_config_->Enable(configs_custom[choice_config_->GetSelection()]);
}

// -----------------------------------------------------------------------------
// Called when the remove game button is clicked
// -----------------------------------------------------------------------------
void RunDialog::onBtnRemoveGame(wxCommandEvent& e)
{
	if (executables::removeGameExe(choice_game_exes_->GetSelection()))
	{
		choice_game_exes_->Clear();
		for (unsigned a = 0; a < executables::nGameExes(); a++)
			choice_game_exes_->AppendString(wxString::FromUTF8(executables::gameExe(a)->name));

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
	bool removed = false;

	if (run_map_)
		removed = executables::removeGameExeMapConfig(
			choice_game_exes_->GetSelection(), choice_config_->GetSelection());
	else
		removed = executables::removeGameExeRunConfig(
			choice_game_exes_->GetSelection(), choice_config_->GetSelection());

	if (removed)
		openGameExe(choice_game_exes_->GetSelection());
}
