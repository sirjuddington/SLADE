
#include "Main.h"
#include "LauncherPanel.h"
#include "UI/Lists/ListView.h"
#include "UI/STabCtrl.h"
#include "Graphics/Icons.h"
#include "Archive/ArchiveManager.h"
#include "General/Executables.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "IdGamesPanel.h"
#include "LibraryPanel.h"
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/dirctrl.h>
#include <wx/filename.h>


CVAR(Int, launcher_panel_split_pos, -1, CVAR_SAVE)


class LauncherFileBrowser : public wxGenericDirCtrl
{
public:
	LauncherFileBrowser(wxWindow* parent, LauncherPanel* launcher) :
		wxGenericDirCtrl(
			parent,
			-1,
			wxDirDialogDefaultFolderStr,
			wxDefaultPosition,
			wxDefaultSize,
			wxDIRCTRL_MULTIPLE,
			theArchiveManager->getArchiveExtensionsString(false)
		),
		launcher_(launcher)
	{
		// Connect a custom event for when an item in the file tree is activated
		wxGenericDirCtrl::GetTreeCtrl()->Connect(
			wxGenericDirCtrl::GetTreeCtrl()->GetId(),
			wxEVT_TREE_ITEM_ACTIVATED,
			wxTreeEventHandler(LauncherFileBrowser::onItemActivated)
		);
	}

	~LauncherFileBrowser() {}

	void onItemActivated(wxTreeEvent& e)
	{
		// Get related objects
		auto tree = (wxTreeCtrl*)e.GetEventObject();
		auto browser = (LauncherFileBrowser*)tree->GetParent();

		// If the selected item has no children (ie it's a file),
		// open it in the archive manager
		if (!tree->ItemHasChildren(e.GetItem()))
			browser->launcher_->addFile(browser->GetPath());

		e.Skip();
	}

protected:
	LauncherPanel*	launcher_;
};


LauncherPanel::LauncherPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	wxWindowBase::SetName("launcher");

	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	auto splitter = new wxSplitterWindow(this);
	//splitter->SetSashSize(8);
	sizer->Add(splitter, 1, wxEXPAND | wxALL, 8);

	// --- Launcher controls ---
	auto panel_controls = setupControlsPanel(splitter);


	// --- Tabs ---
	tabs_library_ = new STabCtrl(splitter, false, false, 28);

	// Library tab
	panel_library_ = new LibraryPanel(tabs_library_);
	tabs_library_->AddPage(panel_library_, "Library");
	tabs_library_->SetPageBitmap(0, Icons::getIcon(Icons::GENERAL, "properties"));

	// idgames tab
	tabs_library_->AddPage(new IdGamesPanel(tabs_library_), "idGames");
	tabs_library_->SetPageBitmap(1, Icons::getIcon(Icons::GENERAL, "wiki"));

	// File browser tab
	tabs_library_->AddPage(setupFileBrowserTab(), "File Browser");
	tabs_library_->SetPageBitmap(2, Icons::getIcon(Icons::GENERAL, "open"));


	// Split
	if (launcher_panel_split_pos < 0)
		launcher_panel_split_pos = panel_controls->GetEffectiveMinSize().x + 64;

	splitter->SplitVertically(panel_controls, tabs_library_, launcher_panel_split_pos);


	// Bind events
	splitter->Bind(wxEVT_SPLITTER_SASH_POS_CHANGED, &LauncherPanel::onSplitterSashPosChanged, this);
	btn_launch_->Bind(wxEVT_BUTTON, &LauncherPanel::onLaunchClicked, this);
}

LauncherPanel::~LauncherPanel()
{
}

wxPanel* LauncherPanel::setupControlsPanel(wxWindow* parent)
{
	auto panel_controls = new wxPanel(parent, -1);
	auto psizer = new wxBoxSizer(wxHORIZONTAL);
	panel_controls->SetSizer(psizer);
	auto gb_sizer = new wxGridBagSizer(4, 4);
	psizer->Add(gb_sizer, 1, wxEXPAND | wxRIGHT, 4);

	// Game selection
	choice_game_ = new wxChoice(panel_controls, -1);
	gb_sizer->Add(new wxStaticText(panel_controls, -1, "Game:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	gb_sizer->Add(choice_game_, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);
	loadGames();

	// Port selection
	choice_port_ = new wxChoice(panel_controls, -1);
	gb_sizer->Add(new wxStaticText(panel_controls, -1, "Executable:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	gb_sizer->Add(choice_port_, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
	loadExecutables();

	// Files
	lv_files_ = new ListView(panel_controls, -1);
	lv_files_->AppendColumn("Filename");
	lv_files_->AppendColumn("Path");
	lv_files_->enableSizeUpdate(false);
	gb_sizer->Add(new wxStaticText(panel_controls, -1, "Files:"), wxGBPosition(2, 0), wxGBSpan(1, 2));
	gb_sizer->Add(lv_files_, wxGBPosition(3, 0), wxGBSpan(1, 2), wxEXPAND);

	// Launch
	btn_launch_ = new wxButton(panel_controls, -1, "LAUNCH", wxDefaultPosition, wxSize(-1, 40));
	gb_sizer->Add(btn_launch_, wxGBPosition(4, 0), wxGBSpan(1, 2), wxEXPAND);

	gb_sizer->AddGrowableCol(1);
	gb_sizer->AddGrowableRow(3);

	return panel_controls;
}

wxPanel* LauncherPanel::setupFileBrowserTab()
{
	auto panel = new wxPanel(tabs_library_, -1);
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(sizer);

	// File browser
	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 1, wxEXPAND | wxALL, 8);
	auto fb = new LauncherFileBrowser(panel, this);
	vbox->Add(new wxStaticText(panel, -1, "Browse:"), 0, wxEXPAND | wxBOTTOM, 4);
	vbox->Add(fb, 1, wxEXPAND);

	// Recent files list
	vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 8);
	lv_recent_files_ = new ListView(panel, -1);
	lv_recent_files_->AppendColumn("Filename");
	lv_recent_files_->AppendColumn("Path");
	lv_recent_files_->enableSizeUpdate(false);
	vbox->Add(new wxStaticText(panel, -1, "Recent Files:"), 0, wxEXPAND | wxBOTTOM, 4);
	vbox->Add(lv_recent_files_, 1, wxEXPAND);

	return panel;
}

void LauncherPanel::loadExecutables()
{
	wxArrayString exes;
	for (unsigned a = 0; a < Executables::nGameExes(); a++)
		exes.Add(Executables::getGameExe(a)->name);
	choice_port_->Append(exes);
}

void LauncherPanel::loadGames()
{
	wxArrayString games;
	int index = -1;
	for (unsigned a = 0; a < theGameConfiguration->nGameConfigs(); a++)
	{
		games.Add(theGameConfiguration->gameConfig(a).title);
		if (theGameConfiguration->gameConfig(a).name == "doom2")
			index = a;
	}
	choice_game_->Append(games);
	choice_game_->SetSelection(index);
}

string LauncherPanel::commandLine()
{
	// Get currently selected executable
	auto exe = Executables::getGameExe(choice_port_->GetSelection());
	if (!exe)
		return "";

	// Start with executable path
	string cmdline = "\"" + exe->path + "\"";

	// Get currently selected game
	string game = theGameConfiguration->gameConfig(choice_game_->GetSelection()).name;
	string br_path = theGameConfiguration->getGameBaseResourcePath(game);

	// Add base resorce (iwad) path
	if (!br_path.IsEmpty())
		cmdline += " -iwad \"" + br_path + "\"";

	// Add files
	if (lv_files_->GetItemCount() > 0)
	{
		cmdline += " -file";

		for (unsigned a = 0; a < lv_files_->GetItemCount(); a++)
		{
			wxFileName fn;
			fn.SetFullName(lv_files_->GetItemText(a, 0));
			fn.SetPath(lv_files_->GetItemText(a, 1));

			cmdline += " \"" + fn.GetFullPath() + "\"";
		}
	}

	return cmdline;
}

void LauncherPanel::addFile(string path)
{
	wxFileName fn(path);
	wxArrayString cols;
	cols.Add(fn.GetFullName());
	cols.Add(fn.GetPath());
	lv_files_->addItem(lv_files_->GetItemCount(), cols);
}



void LauncherPanel::onSplitterSashPosChanged(wxSplitterEvent& e)
{
	launcher_panel_split_pos = e.GetSashPosition();
}

void LauncherPanel::onLaunchClicked(wxCommandEvent& e)
{
	// Get currently selected executable
	auto exe = Executables::getGameExe(choice_port_->GetSelection());
	if (!exe)
		return;

	wxFileName fn(exe->path);

	// Set working directory
	string wd = wxGetCwd();
	wxSetWorkingDirectory(fn.GetPath());

	// Run
	wxExecute(commandLine(), wxEXEC_ASYNC);

	// Restore working directory
	wxSetWorkingDirectory(wd);
}
