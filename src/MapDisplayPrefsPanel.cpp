
#include "Main.h"
#include "WxStuff.h"
#include "MapDisplayPrefsPanel.h"
#include <wx/statline.h>

EXTERN_CVAR(Bool, grid_dashed)
EXTERN_CVAR(Bool, vertex_round)
EXTERN_CVAR(Int, vertex_size)
EXTERN_CVAR(Int, vertices_always)
EXTERN_CVAR(Float, line_width)
EXTERN_CVAR(Bool, line_smooth)
EXTERN_CVAR(Int, thing_drawtype)
EXTERN_CVAR(Int, things_always)
EXTERN_CVAR(Bool, thing_force_dir)
EXTERN_CVAR(Bool, thing_overlay_square)
EXTERN_CVAR(Float, thing_shadow)
EXTERN_CVAR(Float, flat_brightness)
EXTERN_CVAR(Bool, sector_hilight_fill)
EXTERN_CVAR(Bool, flat_ignore_light)
EXTERN_CVAR(Bool, line_tabs_always)
EXTERN_CVAR(Bool, map_animate_hilight)
EXTERN_CVAR(Bool, map_animate_selection)
EXTERN_CVAR(Bool, map_animate_tagged)
EXTERN_CVAR(Bool, line_fade)
EXTERN_CVAR(Bool, flat_fade)
EXTERN_CVAR(Int, map_crosshair)
EXTERN_CVAR(Bool, arrow_colour)
EXTERN_CVAR(Float, arrow_alpha)

MapDisplayPrefsPanel::MapDisplayPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Map Editor Display Preferences");
	wxStaticBoxSizer* fsizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(fsizer, 1, wxEXPAND|wxALL, 4);

	// Create notebook
	nb_pages = new wxNotebook(this, -1);
	fsizer->Add(nb_pages, 1, wxEXPAND|wxALL, 4);


	// General tab
	wxPanel* panel = new wxPanel(nb_pages, -1);
	nb_pages->AddPage(panel, "General", true);
	wxBoxSizer* sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Dashed grid
	cb_grid_dashed = new wxCheckBox(panel, -1, "Dashed grid");
	sizer->Add(cb_grid_dashed, 0, wxEXPAND|wxALL, 4);

	// Always show line direction tabs
	cb_line_tabs_always = new wxCheckBox(panel, -1, "Always show line direction tabs");
	sizer->Add(cb_line_tabs_always, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Animate hilighted object
	cb_animate_hilight = new wxCheckBox(panel, -1, "Animated hilight");
	sizer->Add(cb_animate_hilight, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Animate selected objects
	cb_animate_selection = new wxCheckBox(panel, -1, "Animated selection");
	sizer->Add(cb_animate_selection, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Animate tagged objects
	cb_animate_tagged = new wxCheckBox(panel, -1, "Animated tag indicator");
	sizer->Add(cb_animate_tagged, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Crosshair
	string ch[] ={ "None", "Small", "Full" };
	choice_crosshair = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, ch);
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(panel, -1, "Cursor Crosshair:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(choice_crosshair, 1, wxEXPAND);


	// Vertices tab
	panel = new wxPanel(nb_pages, -1);
	nb_pages->AddPage(panel, "Vertices");
	sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Vertex size
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "Vertex size: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	slider_vertex_size = new wxSlider(panel, -1, vertex_size, 2, 16, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	hbox->Add(slider_vertex_size, 1, wxEXPAND);

	// Round vertices
	cb_vertex_round = new wxCheckBox(panel, -1, "Round vertices");
	sizer->Add(cb_vertex_round, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// When not in vertices mode
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "When not in vertices mode: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	string nonmodeshow[] = { "Hide", "Show", "Fade" };
	choice_vertices_always = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, nonmodeshow);
	hbox->Add(choice_vertices_always, 1, wxEXPAND);


	// Lines tab
	panel = new wxPanel(nb_pages, -1);
	nb_pages->AddPage(panel, "Lines");
	sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Line width
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "Line width: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	slider_line_width = new wxSlider(panel, -1, line_width*10, 10, 30, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	hbox->Add(slider_line_width, 1, wxEXPAND);

	// Smooth lines
	cb_line_smooth = new wxCheckBox(panel, -1, "Smooth lines");
	sizer->Add(cb_line_smooth, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Fade when not in lines mode
	cb_line_fade = new wxCheckBox(panel, -1, "Fade when not in lines mode");
	sizer->Add(cb_line_fade, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);


	// Things tab
	panel = new wxPanel(nb_pages, -1);
	nb_pages->AddPage(panel, "Things");
	sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Thing style
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "Thing style: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	string t_types[] = { "Square", "Round", "Sprite", "Square + Sprite", "Framed Sprite" };
	choice_thing_drawtype = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 5, t_types);
	hbox->Add(choice_thing_drawtype, 1, wxEXPAND);

	// Always show angles
	cb_thing_force_dir = new wxCheckBox(panel, -1, "Always show thing angles");
	sizer->Add(cb_thing_force_dir, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Colour angle arrows
	cb_thing_arrow_colour = new wxCheckBox(panel, -1, "Colour thing angle arrows");
	sizer->Add(cb_thing_arrow_colour, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Force square hilight/selection
	cb_thing_overlay_square = new wxCheckBox(panel, -1, "Force square thing hilight/selection overlay");
	sizer->Add(cb_thing_overlay_square, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Shadow opacity
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "Thing shadow opacity: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	slider_thing_shadow = new wxSlider(panel, -1, thing_shadow*10, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	hbox->Add(slider_thing_shadow, 1, wxEXPAND);

	// Arrow opacity
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "Thing angle arrow opacity: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	slider_thing_arrow_alpha = new wxSlider(panel, -1, thing_shadow*10, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	hbox->Add(slider_thing_arrow_alpha, 1, wxEXPAND);

	// When not in things mode
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "When not in things mode: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	choice_things_always = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, nonmodeshow);
	hbox->Add(choice_things_always, 1, wxEXPAND);


	// Sectors tab
	panel = new wxPanel(nb_pages, -1);
	nb_pages->AddPage(panel, "Sectors");
	sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Flat brightness
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "Flat brightness: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	slider_flat_brightness = new wxSlider(panel, -1, flat_brightness*10, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	hbox->Add(slider_flat_brightness, 1, wxEXPAND);

	// Ignore sector light
	cb_flat_ignore_light = new wxCheckBox(panel, -1, "Flats ignore sector brightness");
	sizer->Add(cb_flat_ignore_light, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Fill sector hilight
	cb_sector_hilight_fill = new wxCheckBox(panel, -1, "Filled sector hilight");
	sizer->Add(cb_sector_hilight_fill, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Fade when not in sectors mode
	cb_flat_fade = new wxCheckBox(panel, -1, "Fade flats when not in sectors mode");
	sizer->Add(cb_flat_fade, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	Layout();
}

MapDisplayPrefsPanel::~MapDisplayPrefsPanel()
{
}

/* MapDisplayPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void MapDisplayPrefsPanel::init()
{
	cb_vertex_round->SetValue(vertex_round);
	cb_line_smooth->SetValue(line_smooth);
	cb_line_tabs_always->SetValue(line_tabs_always);
	choice_thing_drawtype->SetSelection(thing_drawtype);
	cb_thing_force_dir->SetValue(thing_force_dir);
	cb_thing_overlay_square->SetValue(thing_overlay_square);
	cb_thing_arrow_colour->SetValue(arrow_colour);
	cb_flat_ignore_light->SetValue(flat_ignore_light);
	cb_sector_hilight_fill->SetValue(sector_hilight_fill);
	cb_animate_hilight->SetValue(map_animate_hilight);
	cb_animate_selection->SetValue(map_animate_selection);
	cb_animate_tagged->SetValue(map_animate_tagged);
	choice_vertices_always->SetSelection(vertices_always);
	choice_things_always->SetSelection(things_always);
	cb_line_fade->SetValue(line_fade);
	cb_flat_fade->SetValue(flat_fade);
	cb_grid_dashed->SetValue(grid_dashed);
	slider_vertex_size->SetValue(vertex_size);
	slider_line_width->SetValue(line_width * 10);
	slider_thing_shadow->SetValue(thing_shadow * 10);
	slider_thing_arrow_alpha->SetValue(arrow_alpha * 10);
	slider_flat_brightness->SetValue(flat_brightness * 10);
	choice_crosshair->Select(map_crosshair);
}

void MapDisplayPrefsPanel::applyPreferences()
{
	grid_dashed = cb_grid_dashed->GetValue();
	vertex_round = cb_vertex_round->GetValue();
	vertex_size = slider_vertex_size->GetValue();
	line_width = (float)slider_line_width->GetValue() * 0.1f;
	line_smooth = cb_line_smooth->GetValue();
	line_tabs_always = cb_line_tabs_always->GetValue();
	thing_drawtype = choice_thing_drawtype->GetSelection();
	thing_force_dir = cb_thing_force_dir->GetValue();
	thing_overlay_square = cb_thing_overlay_square->GetValue();
	thing_shadow = (float)slider_thing_shadow->GetValue() * 0.1f;
	arrow_colour = cb_thing_arrow_colour->GetValue();
	arrow_alpha = (float)slider_thing_arrow_alpha->GetValue() * 0.1f;
	flat_brightness = (float)slider_flat_brightness->GetValue() * 0.1f;
	flat_ignore_light = cb_flat_ignore_light->GetValue();
	sector_hilight_fill = cb_sector_hilight_fill->GetValue();
	map_animate_hilight = cb_animate_hilight->GetValue();
	map_animate_selection = cb_animate_selection->GetValue();
	map_animate_tagged = cb_animate_tagged->GetValue();
	vertices_always = choice_vertices_always->GetSelection();
	things_always = choice_things_always->GetSelection();
	line_fade = cb_line_fade->GetValue();
	flat_fade = cb_flat_fade->GetValue();
	map_crosshair = choice_crosshair->GetSelection();
}
