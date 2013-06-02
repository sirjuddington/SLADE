
#include "Main.h"
#include "WxStuff.h"
#include "MapEditorPrefsPanel.h"

EXTERN_CVAR(Bool, scroll_smooth)
EXTERN_CVAR(Bool, selection_clear_click)

MapEditorPrefsPanel::MapEditorPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Map Editor Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Smooth scroll
	cb_scroll_smooth = new wxCheckBox(this, -1, "Enable smooth scrolling");
	sizer->Add(cb_scroll_smooth, 0, wxEXPAND|wxALL, 4);

	// Clear selection on click
	cb_selection_clear_click = new wxCheckBox(this, -1, "Clear selection when nothing is clicked");
	sizer->Add(cb_selection_clear_click, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	Layout();
}

MapEditorPrefsPanel::~MapEditorPrefsPanel()
{
}

/* MapEditorPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void MapEditorPrefsPanel::init()
{
	cb_scroll_smooth->SetValue(scroll_smooth);
	cb_selection_clear_click->SetValue(selection_clear_click);
}

void MapEditorPrefsPanel::applyPreferences()
{
	scroll_smooth = cb_scroll_smooth->GetValue();
	selection_clear_click = cb_selection_clear_click->GetValue();
}
