
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PreferencesDialog.cpp
// Description: The SLADE Preferences dialog. Brings together all the various
//              settings panels in a single dialog
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
#include "PreferencesDialog.h"
#include "ACSPrefsPanel.h"
#include "AdvancedPrefsPanel.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "AudioPrefsPanel.h"
#include "BaseResourceArchivesPanel.h"
#include "ColorimetryPrefsPanel.h"
#include "ColourPrefsPanel.h"
#include "EditingPrefsPanel.h"
#include "General/UI.h"
#include "GeneralPrefsPanel.h"
#include "Graphics/Icons.h"
#include "GraphicsPrefsPanel.h"
#include "HudOffsetsPrefsPanel.h"
#include "InputPrefsPanel.h"
#include "InterfacePrefsPanel.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/ArchiveManagerPanel.h"
#include "MainEditor/UI/MainWindow.h"
#include "Map3DPrefsPanel.h"
#include "MapDisplayPrefsPanel.h"
#include "MapEditorPrefsPanel.h"
#include "NodesPrefsPanel.h"
#include "OpenGLPrefsPanel.h"
#include "PNGPrefsPanel.h"
#include "TextEditorPrefsPanel.h"
#include "TextStylePrefsPanel.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
wxString PreferencesDialog::last_page_ = "";
int      PreferencesDialog::width_     = 0;
int      PreferencesDialog::height_    = 0;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Creates a sizer with a settings page title, (optional) description and
// separator line
// -----------------------------------------------------------------------------
wxSizer* createTitleSizer(wxWindow* parent, const wxString& title, const wxString& description)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);

	// Title
	auto title_label = new wxStaticText(parent, -1, title);
	auto font        = title_label->GetFont();
	title_label->SetFont(font.MakeLarger().MakeLarger().MakeBold());
	title_label->SetMinSize(wxSize(-1, title_label->GetTextExtent("Wy").y));
	sizer->Add(title_label, 0, wxEXPAND);

	// Description
	if (!description.empty())
		sizer->Add(new wxStaticText(parent, -1, description), 0, wxEXPAND);

	// Separator
	sizer->AddSpacer(ui::px(ui::Size::PadMinimum));
	sizer->Add(new wxStaticLine(parent), 0, wxEXPAND | wxBOTTOM, ui::padLarge());

	return sizer;
}
} // namespace


// -----------------------------------------------------------------------------
//
// PreferencesDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PreferencesDialog class constructor
// -----------------------------------------------------------------------------
PreferencesDialog::PreferencesDialog(wxWindow* parent) : SDialog(parent, "SLADE Settings", "prefs")
{
	// Setup main sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Set icon
	wxIcon icon;
	icon.CopyFromBitmap(icons::getIcon(icons::General, "settings"));
	SetIcon(icon);

	// Create preferences TreeBook
	tree_prefs_ = new wxTreebook(this, -1, wxDefaultPosition, wxDefaultSize);
#if wxMAJOR_VERSION > 3 || (wxMAJOR_VERSION == 3 && wxMINOR_VERSION >= 1)
	tree_prefs_->GetTreeCtrl()->EnableSystemTheme(true);
#endif

	// Setup preferences TreeBook
	addPrefsPage<GeneralPrefsPanel>("General", false, true);
	addPrefsPage<OpenGLPrefsPanel>("OpenGL", true);
	addPrefsPage<InterfacePrefsPanel>("Interface");
	addPrefsPage<ColourPrefsPanel>("Colours & Theme", true);
	addPrefsPage<InputPrefsPanel>("Keyboard Shortcuts");
	addPrefsPage<EditingPrefsPanel>("Editing");
	addPrefsPage<BaseResourceArchivesPanel>("Base Resource Archive", true);
	addPrefsPage<TextEditorPrefsPanel>("Text Editor");
	addPrefsPage<TextStylePrefsPanel>("Fonts & Colours", true);
	addPrefsPage<GraphicsPrefsPanel>("Graphics");
	addPrefsPage<PNGPrefsPanel>("PNG", true);
	addPrefsPage<ColorimetryPrefsPanel>("Colorimetry", true);
	addPrefsPage<HudOffsetsPrefsPanel>("HUD Offsets View", true);
	addPrefsPage<AudioPrefsPanel>("Audio");
	tree_prefs_->AddPage(new wxPanel(tree_prefs_, -1), "Scripting");
	addPrefsPage<ACSPrefsPanel>("ACS", true);
	addPrefsPage<MapEditorPrefsPanel>("Map Editor");
	addPrefsPage<MapDisplayPrefsPanel>("Display", true);
	addPrefsPage<Map3DPrefsPanel>("3D Mode", true);
	addPrefsPage<NodesPrefsPanel>("Node Builder", true);
	tree_prefs_->AddPage(setupAdvancedPanel(), "Advanced");

	// Expand all tree nodes (so it gets sized properly)
	for (unsigned page = 0; page < tree_prefs_->GetPageCount(); page++)
		tree_prefs_->ExpandNode(page);

	// Add preferences treebook
	sizer->Add(tree_prefs_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, ui::padLarge());

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL | wxAPPLY), 0, wxEXPAND | wxALL, ui::padLarge());

	// Bind events
	Bind(wxEVT_BUTTON, &PreferencesDialog::onButtonClicked, this);

	// Setup layout
	Layout();
	const wxSize size = GetSize() * GetContentScaleFactor();
	SetInitialSize(size);
	//Fit();
	SetMinSize(wxSize(ui::scalePx(800), ui::scalePx(600)));
	CenterOnParent();

	// Collapse all tree nodes
	for (unsigned page = 0; page < tree_prefs_->GetPageCount(); page++)
		tree_prefs_->CollapseNode(page);
}

// -----------------------------------------------------------------------------
// Adds a settings [page] to the treebook with [title] in the tree. If
// [sub_page] is true the page is added as a subpage of the previously added
// (non-sub) page
// -----------------------------------------------------------------------------
void PreferencesDialog::addPrefsPage(PrefsPanelBase* page, const wxString& title, bool sub_page, bool select)
{
	// Create a panel to put the preferences page in
	// (since we want some extra padding between the tree and the page)
	auto panel = new wxPanel(tree_prefs_, -1);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Add page title section
	sizer->Add(
		createTitleSizer(
			panel, page->pageTitle().empty() ? title + " Settings" : page->pageTitle(), page->pageDescription()),
		0,
		wxEXPAND | wxLEFT,
		ui::pad());

	// Add prefs page to panel
	page->Reparent(panel);
	panel->GetSizer()->Add(page, 1, wxEXPAND | wxLEFT, ui::pad());

	// Add panel to treebook
	if (sub_page)
		tree_prefs_->AddSubPage(panel, title, select);
	else
		tree_prefs_->AddPage(panel, title, select);

	// Add page to map of prefs pages
	prefs_pages_[title] = page;
}

// -----------------------------------------------------------------------------
// Creates the wxPanel containing the Base Resource Archives panel, plus some
// extra stuff, and returns it
// -----------------------------------------------------------------------------
wxPanel* PreferencesDialog::setupBaseResourceArchivesPanel()
{
	// Create panel
	auto panel  = new wxPanel(tree_prefs_, -1);
	auto psizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(psizer);

	// Add page title section
	psizer->Add(createTitleSizer(panel, "Base Resource Archive", ""), 0, wxEXPAND | wxLEFT, ui::pad());

	// Add BRA panel
	panel_bra_ = new BaseResourceArchivesPanel(panel);
	psizer->Add(panel_bra_, 1, wxEXPAND | wxLEFT, ui::pad());

	return panel;
}

// -----------------------------------------------------------------------------
// Creates and returns a wxPanel containing the advanced settings page
// -----------------------------------------------------------------------------
wxPanel* PreferencesDialog::setupAdvancedPanel()
{
	// Create panel
	auto panel  = new wxPanel(tree_prefs_, -1);
	auto psizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(psizer);

	// Add page title section
	psizer->Add(
		createTitleSizer(
			panel,
			"Advanced Settings",
			"Warning: Only modify these values if you know what you are doing!\n"
			"Most of these settings can be changed more safely from the other sections."),
		0,
		wxEXPAND | wxLEFT,
		ui::pad());

	// Add advanced settings panel
	prefs_advanced_ = new AdvancedPrefsPanel(panel);
	psizer->Add(prefs_advanced_, 1, wxEXPAND | wxLEFT, ui::pad());

	return panel;
}

// -----------------------------------------------------------------------------
// Shows the preferences page matching [name]
// -----------------------------------------------------------------------------
void PreferencesDialog::showPage(const wxString& name, const wxString& subsection)
{
	// Go through all pages
	for (unsigned a = 0; a < tree_prefs_->GetPageCount(); a++)
	{
		if (S_CMPNOCASE(tree_prefs_->GetPageText(a), name))
		{
			tree_prefs_->SetSelection(a);
			tree_prefs_->ExpandNode(a);
			if (prefs_pages_[name])
				prefs_pages_[name]->showSubSection(subsection);
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Returns the name of the currently selected page
// -----------------------------------------------------------------------------
wxString PreferencesDialog::currentPage() const
{
	int sel = tree_prefs_->GetSelection();

	if (sel >= 0)
		return tree_prefs_->GetPageText(sel);
	else
		return "";
}

// -----------------------------------------------------------------------------
// Initialises controls on all preference panels
// -----------------------------------------------------------------------------
void PreferencesDialog::initPages()
{
	for (auto i : prefs_pages_)
		if (i.second)
			i.second->init();
	prefs_advanced_->init();
}

// -----------------------------------------------------------------------------
// Applies preference values from all preference panels
// -----------------------------------------------------------------------------
void PreferencesDialog::applyPreferences()
{
	for (auto i : prefs_pages_)
		if (i.second)
			i.second->applyPreferences();
	prefs_advanced_->applyPreferences();

	// Write file so changes are not lost
	app::saveConfigFile();
}


// -----------------------------------------------------------------------------
//
// PreferencesDialog Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a button is clicked
// -----------------------------------------------------------------------------
void PreferencesDialog::onButtonClicked(wxCommandEvent& e)
{
	// Check if it was the apply button
	if (e.GetId() == wxID_APPLY)
		applyPreferences();
	else
		e.Skip();
}


// -----------------------------------------------------------------------------
//
// PreferencesDialog Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Opens a preferences dialog on top of [parent], showing either the last viewed
// page or [initial_page] if it is specified
// -----------------------------------------------------------------------------
void PreferencesDialog::openPreferences(wxWindow* parent, wxString initial_page, const wxString& subsection)
{
	// Setup dialog
	PreferencesDialog dlg(parent);
	if (initial_page.IsEmpty())
		initial_page = last_page_;
	dlg.showPage(initial_page, subsection);
	dlg.initPages();
	dlg.CenterOnParent();

	// Show dialog
	if (dlg.ShowModal() == wxID_OK)
		dlg.applyPreferences();
	theMainWindow->archiveManagerPanel()->refreshAllTabs();

	// Save state
	last_page_ = dlg.currentPage();
}
