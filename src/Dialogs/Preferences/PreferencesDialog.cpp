
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PreferencesDialog.cpp
 * Description: The SLADE Preferences dialog. Brings together all the
 *              various preference panels in a single dialog
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "App.h"
#include "PreferencesDialog.h"
#include "ACSPrefsPanel.h"
#include "AdvancedPrefsPanel.h"
#include "Archive/ArchiveManager.h"
#include "AudioPrefsPanel.h"
#include "BaseResourceArchivesPanel.h"
#include "ColorimetryPrefsPanel.h"
#include "ColourPrefsPanel.h"
#include "EditingPrefsPanel.h"
#include "GeneralPrefsPanel.h"
#include "Graphics/Icons.h"
#include "GraphicsPrefsPanel.h"
#include "HudOffsetsPrefsPanel.h"
#include "InputPrefsPanel.h"
#include "InterfacePrefsPanel.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MainEditor/UI/ArchiveManagerPanel.h"
#include "Map3DPrefsPanel.h"
#include "MapDisplayPrefsPanel.h"
#include "MapEditorPrefsPanel.h"
#include "NodesPrefsPanel.h"
#include "OpenGLPrefsPanel.h"
#include "PNGPrefsPanel.h"
#include "TextEditorPrefsPanel.h"
#include "TextStylePrefsPanel.h"



/*******************************************************************
 * VARIABLES
 *******************************************************************/
string PreferencesDialog::last_page = "";
int PreferencesDialog::width = 0;
int PreferencesDialog::height = 0;


/*******************************************************************
 * PREFERENCESDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* PreferencesDialog::PreferencesDialog
 * PreferencesDialog class constructor
 *******************************************************************/
PreferencesDialog::PreferencesDialog(wxWindow* parent) : SDialog(parent, "SLADE Preferences", "prefs")
{
	// Setup main sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Set icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "settings"));
	SetIcon(icon);

	// Create preferences TreeBook
	tree_prefs = new wxTreebook(this, -1, wxDefaultPosition, wxDefaultSize);
#if wxMAJOR_VERSION > 3 || (wxMAJOR_VERSION == 3 && wxMINOR_VERSION >= 1)
	tree_prefs->GetTreeCtrl()->EnableSystemTheme(true);
#endif

	// Setup preferences TreeBook
	PrefsPanelBase* panel;
	panel = new GeneralPrefsPanel(tree_prefs);		tree_prefs->AddPage(panel, "General", true); prefs_pages.push_back(panel);
	panel = new OpenGLPrefsPanel(tree_prefs);		tree_prefs->AddSubPage(panel, "OpenGL"); prefs_pages.push_back(panel);
	panel = new InterfacePrefsPanel(tree_prefs);	tree_prefs->AddPage(panel, "Interface"); prefs_pages.push_back(panel);
	panel = new ColourPrefsPanel(tree_prefs);		tree_prefs->AddSubPage(panel, "Colours & Theme"); prefs_pages.push_back(panel);
	panel = new InputPrefsPanel(tree_prefs);		tree_prefs->AddPage(panel, "Input"); prefs_pages.push_back(panel);
	panel = new EditingPrefsPanel(tree_prefs);		tree_prefs->AddPage(panel, "Editing"); prefs_pages.push_back(panel);
	tree_prefs->AddSubPage(setupBaseResourceArchivesPanel(), "Base Resource Archive");
	panel = new TextEditorPrefsPanel(tree_prefs);	tree_prefs->AddPage(panel, "Text Editor"); prefs_pages.push_back(panel);
	panel = new TextStylePrefsPanel(tree_prefs);	tree_prefs->AddSubPage(panel, "Fonts & Colours"); prefs_pages.push_back(panel);
	panel = new GraphicsPrefsPanel(tree_prefs);		tree_prefs->AddPage(panel, "Graphics"); prefs_pages.push_back(panel);
	panel = new PNGPrefsPanel(tree_prefs);			tree_prefs->AddSubPage(panel, "PNG"); prefs_pages.push_back(panel);
	panel = new ColorimetryPrefsPanel(tree_prefs);	tree_prefs->AddSubPage(panel, "Colorimetry"); prefs_pages.push_back(panel);
	panel = new HudOffsetsPrefsPanel(tree_prefs);	tree_prefs->AddSubPage(panel, "HUD Offsets View"); prefs_pages.push_back(panel);
	panel = new AudioPrefsPanel(tree_prefs);		tree_prefs->AddPage(panel, "Audio"); prefs_pages.push_back(panel);
	tree_prefs->AddPage(new wxPanel(tree_prefs, -1), "Scripting");
	panel = new ACSPrefsPanel(tree_prefs);			tree_prefs->AddSubPage(panel, "ACS"); prefs_pages.push_back(panel);
	panel = new MapEditorPrefsPanel(tree_prefs);	tree_prefs->AddPage(panel, "Map Editor"); prefs_pages.push_back(panel);
	panel = new MapDisplayPrefsPanel(tree_prefs);	tree_prefs->AddSubPage(panel, "Display"); prefs_pages.push_back(panel);
	panel = new Map3DPrefsPanel(tree_prefs);		tree_prefs->AddSubPage(panel, "3D Mode"); prefs_pages.push_back(panel);
	panel = new NodesPrefsPanel(tree_prefs);		tree_prefs->AddSubPage(panel, "Node Builders"); prefs_pages.push_back(panel);
	prefs_advanced = new AdvancedPrefsPanel(tree_prefs);
	tree_prefs->AddPage(prefs_advanced, "Advanced");

	// Expand all tree nodes (so it gets sized properly)
	for (unsigned page = 0; page < tree_prefs->GetPageCount(); page++)
		tree_prefs->ExpandNode(page);

	// Add preferences treebook
	sizer->Add(tree_prefs, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK|wxCANCEL|wxAPPLY), 0, wxEXPAND|wxALL, 10);

	// Bind events
	Bind(wxEVT_BUTTON, &PreferencesDialog::onButtonClicked, this);

	// Setup layout
	SetInitialSize(GetSize());
	Layout();
	Fit();
	SetMinSize(wxSize(800, 600));
	CenterOnParent();

	// Collapse all tree nodes
	for (unsigned page = 0; page < tree_prefs->GetPageCount(); page++)
		tree_prefs->CollapseNode(page);
}

/* PreferencesDialog::~PreferencesDialog
 * PreferencesDialog class destructor
 *******************************************************************/
PreferencesDialog::~PreferencesDialog()
{
}

/* PreferencesDialog::setupBaseResourceArchivesPanel
 * Creates the wxPanel containing the Base Resource Archives panel,
 * plus some extra stuff, and returns it
 *******************************************************************/
wxPanel* PreferencesDialog::setupBaseResourceArchivesPanel()
{
	// Create panel
	wxPanel* panel = new wxPanel(tree_prefs, -1);
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(panel, -1, "Base Resource Archive");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Add BRA panel
	panel_bra = new BaseResourceArchivesPanel(panel);
	sizer->Add(panel_bra, 1, wxEXPAND|wxALL, 4);

	// Add 'Open BRA' button
	btn_bra_open = new wxButton(panel, -1, "Open Selected Archive");
	sizer->Add(btn_bra_open, 0, wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Bind events
	btn_bra_open->Bind(wxEVT_BUTTON, &PreferencesDialog::onBtnBRAOpenClicked, this);

	return panel;
}

/* PreferencesDialog::showPage
 * Shows the preferences page matching [name]
 *******************************************************************/
void PreferencesDialog::showPage(string name, string subsection)
{
	// Go through all pages
	for (unsigned a = 0; a < tree_prefs->GetPageCount(); a++)
	{
		if (S_CMPNOCASE(tree_prefs->GetPageText(a), name))
		{
			tree_prefs->SetSelection(a);
			((PrefsPanelBase*)tree_prefs->GetPage(a))->showSubSection(subsection);
			return;
		}
	}
}

/* PreferencesDialog::currentPage
 * Returns the name of the currently selected page
 *******************************************************************/
string PreferencesDialog::currentPage()
{
	int sel = tree_prefs->GetSelection();

	if (sel >= 0)
		return tree_prefs->GetPageText(sel);
	else
		return "";
}

/* PreferencesDialog::initPages
 * Initialises controls on all preference panels
 *******************************************************************/
void PreferencesDialog::initPages()
{
	for (unsigned a = 0; a < prefs_pages.size(); a++)
		prefs_pages[a]->init();
	prefs_advanced->init();
}

/* PreferencesDialog::applyPreferences
 * Applies preference values from all preference panels
 *******************************************************************/
void PreferencesDialog::applyPreferences()
{
	for (unsigned a = 0; a < prefs_pages.size(); a++)
		prefs_pages[a]->applyPreferences();
	prefs_advanced->applyPreferences();

	// Write file so changes are not lost
	App::saveConfigFile();
}


/*******************************************************************
 * PREFERENCESDIALOG CLASS EVENTS
 *******************************************************************/

/* PreferencesDialog::onBtnBRAOpenClicked
 * Called when the 'Open Selected BRA' button is clicked
 *******************************************************************/
void PreferencesDialog::onBtnBRAOpenClicked(wxCommandEvent& e)
{
	theArchiveManager->openBaseResource(panel_bra->getSelectedPath());
}

/* PreferencesDialog::onButtonClicked
 * Called when a button is clicked
 *******************************************************************/
void PreferencesDialog::onButtonClicked(wxCommandEvent& e)
{
	// Check if it was the apply button
	if (e.GetId() == wxID_APPLY)
		applyPreferences();
	else
		e.Skip();
}


/*******************************************************************
 * PREFERENCESDIALOG STATIC FUNCTIONS
 *******************************************************************/

/* PreferencesDialog::openPreferences
 * Opens a preferences dialog on top of [parent], showing either the
 * last viewed page or [initial_page] if it is specified
 *******************************************************************/
void PreferencesDialog::openPreferences(wxWindow* parent, string initial_page, string subsection)
{
	// Setup dialog
	PreferencesDialog dlg(parent);
	if (initial_page.IsEmpty())
		initial_page = last_page;
	dlg.showPage(initial_page, subsection);
	if (width > 0 && height > 0)
		dlg.SetSize(width, height);
	dlg.initPages();
	dlg.CenterOnParent();

	// Show dialog
	if (dlg.ShowModal() == wxID_OK)
		dlg.applyPreferences();
	theMainWindow->getArchiveManagerPanel()->refreshAllTabs();

	// Save state
	last_page = dlg.currentPage();
	width = dlg.GetSize().x;
	height = dlg.GetSize().y;
}
