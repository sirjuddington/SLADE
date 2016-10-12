
#include "Main.h"
#include "LauncherPanel.h"
#include "UI/Lists/ListView.h"
#include "UI/STabCtrl.h"
#include "Graphics/Icons.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "Archive/ArchiveManager.h"
#include "General/Executables.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "IdGamesPanel.h"
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/dirctrl.h>
#include <wx/filename.h>


CVAR(Int, launcher_panel_split_pos, -1, CVAR_SAVE)


class LauncherFileBrowser : public wxGenericDirCtrl
{
private:
	

public:
	LauncherFileBrowser(wxWindow* parent, LauncherPanel* launcher)
		: wxGenericDirCtrl(parent,
			-1,
			wxDirDialogDefaultFolderStr,
			wxDefaultPosition,
			wxDefaultSize,
			wxDIRCTRL_MULTIPLE,
			theArchiveManager->getArchiveExtensionsString(false)),
		launcher(launcher)
	{
		// Connect a custom event for when an item in the file tree is activated
		GetTreeCtrl()->Connect(GetTreeCtrl()->GetId(), wxEVT_TREE_ITEM_ACTIVATED, wxTreeEventHandler(LauncherFileBrowser::onItemActivated));
	}

	~LauncherFileBrowser() {}

	void onItemActivated(wxTreeEvent& e)
	{
		// Get related objects
		wxTreeCtrl* tree = (wxTreeCtrl*)e.GetEventObject();
		LauncherFileBrowser* browser = (LauncherFileBrowser*)tree->GetParent();

		// If the selected item has no children (ie it's a file),
		// open it in the archive manager
		if (!tree->ItemHasChildren(e.GetItem()))
			browser->launcher->addFile(browser->GetPath());

		e.Skip();
	}

	LauncherPanel*	launcher;
};


LauncherPanel::LauncherPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	SetName("launcher");

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	wxSplitterWindow* splitter = new wxSplitterWindow(this);
	splitter->SetSashSize(8);
	sizer->Add(splitter, 1, wxEXPAND | wxALL, 8);

	// --- Launcher controls ---
	wxPanel* panel_controls = setupControlsPanel(splitter);


	// --- Tabs ---
	tabs_library = new STabCtrl(splitter, false, false, 28);

	// Library tab
	tabs_library->AddPage(new wxPanel(tabs_library), "Library");
	tabs_library->SetPageBitmap(0, Icons::getIcon(Icons::GENERAL, "properties"));

	// idgames tab
	tabs_library->AddPage(new IdGamesPanel(tabs_library), "idGames");
	tabs_library->SetPageBitmap(1, Icons::getIcon(Icons::GENERAL, "wiki"));

	// File browser tab
	tabs_library->AddPage(setupFileBrowserTab(), "File Browser");
	tabs_library->SetPageBitmap(2, Icons::getIcon(Icons::GENERAL, "open"));


	// Split
	if (launcher_panel_split_pos < 0)
		launcher_panel_split_pos = panel_controls->GetEffectiveMinSize().x + 64;

	splitter->SplitVertically(panel_controls, tabs_library, launcher_panel_split_pos);


	// Bind events
	splitter->Bind(wxEVT_SPLITTER_SASH_POS_CHANGED, &LauncherPanel::onSplitterSashPosChanged, this);
	btn_launch->Bind(wxEVT_BUTTON, &LauncherPanel::onLaunchClicked, this);
}

LauncherPanel::~LauncherPanel()
{
}

wxPanel* LauncherPanel::setupControlsPanel(wxWindow* parent)
{
	wxPanel* panel_controls = new wxPanel(parent, -1);
	wxBoxSizer* psizer = new wxBoxSizer(wxHORIZONTAL);
	panel_controls->SetSizer(psizer);
	wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
	psizer->Add(gb_sizer, 1, wxEXPAND | wxRIGHT, 4);

	// Game selection
	choice_game = new wxChoice(panel_controls, -1);
	gb_sizer->Add(new wxStaticText(panel_controls, -1, "Game:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	gb_sizer->Add(choice_game, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);
	loadGames();

	// Port selection
	choice_port = new wxChoice(panel_controls, -1);
	gb_sizer->Add(new wxStaticText(panel_controls, -1, "Executable:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	gb_sizer->Add(choice_port, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
	loadExecutables();

	// Files
	lv_files = new ListView(panel_controls, -1);
	lv_files->AppendColumn("Filename");
	lv_files->AppendColumn("Path");
	lv_files->enableSizeUpdate(false);
	gb_sizer->Add(new wxStaticText(panel_controls, -1, "Files:"), wxGBPosition(2, 0), wxGBSpan(1, 2));
	gb_sizer->Add(lv_files, wxGBPosition(3, 0), wxGBSpan(1, 2), wxEXPAND);

	// Launch
	btn_launch = new wxButton(panel_controls, -1, "LAUNCH", wxDefaultPosition, wxSize(-1, 40));
	gb_sizer->Add(btn_launch, wxGBPosition(4, 0), wxGBSpan(1, 2), wxEXPAND);

	gb_sizer->AddGrowableCol(1);
	gb_sizer->AddGrowableRow(3);

	return panel_controls;
}

wxPanel* LauncherPanel::setupFileBrowserTab()
{
	wxPanel* panel = new wxPanel(tabs_library, -1);
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(sizer);

	// File browser
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 1, wxEXPAND | wxALL, 8);
	LauncherFileBrowser* fb = new LauncherFileBrowser(panel, this);
	vbox->Add(new wxStaticText(panel, -1, "Browse:"), 0, wxEXPAND | wxBOTTOM, 4);
	vbox->Add(fb, 1, wxEXPAND);

	// Recent files list
	vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 8);
	lv_recent_files = new ListView(panel, -1);
	lv_recent_files->AppendColumn("Filename");
	lv_recent_files->AppendColumn("Path");
	lv_recent_files->enableSizeUpdate(false);
	vbox->Add(new wxStaticText(panel, -1, "Recent Files:"), 0, wxEXPAND | wxBOTTOM, 4);
	vbox->Add(lv_recent_files, 1, wxEXPAND);

	return panel;
}

void LauncherPanel::loadExecutables()
{
	wxArrayString exes;
	for (unsigned a = 0; a < Executables::nGameExes(); a++)
		exes.Add(Executables::getGameExe(a)->name);
	choice_port->Append(exes);
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
	choice_game->Append(games);
	choice_game->SetSelection(index);
}

string LauncherPanel::commandLine()
{
	// Get currently selected executable
	Executables::game_exe_t* exe = Executables::getGameExe(choice_port->GetSelection());
	if (!exe)
		return "";

	// Start with executable path
	string cmdline = "\"" + exe->path + "\"";

	// Get currently selected game
	string game = theGameConfiguration->gameConfig(choice_game->GetSelection()).name;
	string br_path = theGameConfiguration->getGameBaseResourcePath(game);

	// Add base resorce (iwad) path
	if (!br_path.IsEmpty())
		cmdline += " -iwad \"" + br_path + "\"";

	// Add files
	if (lv_files->GetItemCount() > 0)
	{
		cmdline += " -file";

		for (unsigned a = 0; a < lv_files->GetItemCount(); a++)
		{
			wxFileName fn;
			fn.SetFullName(lv_files->GetItemText(a, 0));
			fn.SetPath(lv_files->GetItemText(a, 1));

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
	lv_files->addItem(lv_files->GetItemCount(), cols);
}



void LauncherPanel::onSplitterSashPosChanged(wxSplitterEvent& e)
{
	launcher_panel_split_pos = e.GetSashPosition();
}

void LauncherPanel::onLaunchClicked(wxCommandEvent& e)
{
	// Get currently selected executable
	Executables::game_exe_t* exe = Executables::getGameExe(choice_port->GetSelection());
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
