
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    ThingPropsPanel.cpp
// Description: UI for editing thing properties
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "App.h"
#include "Game/Configuration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/UI/Dialogs/ActionSpecialDialog.h"
#include "MapEditor/UI/Dialogs/ThingTypeBrowser.h"
#include "MapObjectPropsPanel.h"
#include "OpenGL/Drawing.h"
#include "ThingPropsPanel.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/Controls/STabCtrl.h"
#include "Utility/MathStuff.h"
#include "UI/WxUtils.h"


// ----------------------------------------------------------------------------
// SpriteTexCanvas Class Functions
//
// A simple opengl canvas to display a thing sprite
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SpriteTexCanvas::SideTexCanvas
//
// SpriteTexCanvas class constructor
// ----------------------------------------------------------------------------
SpriteTexCanvas::SpriteTexCanvas(wxWindow* parent) : OGLCanvas(parent, -1)
{
	SetWindowStyleFlag(wxBORDER_SIMPLE);
	SetInitialSize(WxUtils::scaledSize(128, 128));
}

// ----------------------------------------------------------------------------
// SpriteTexCanvas::setTexture
//
// Sets the texture to display
// ----------------------------------------------------------------------------
void SpriteTexCanvas::setSprite(const Game::ThingType& type)
{
	texname_ = type.sprite();
	icon_ = false;
	colour_ = COL_WHITE;

	// Sprite
	texture_ = MapEditor::textureManager().getSprite(texname_, type.translation(), type.palette());

	// Icon
	if (!texture_)
	{
		texture_ = MapEditor::textureManager().getEditorImage(S_FMT("thing/%s", type.icon()));
		colour_ = type.colour();
		icon_ = true;
	}

	// Unknown
	if (!texture_)
	{
		texture_ = MapEditor::textureManager().getEditorImage("thing/unknown");
		icon_ = true;
	}

	Refresh();
}

// ----------------------------------------------------------------------------
// SpriteTexCanvas::draw
//
// Draws the canvas content
// ----------------------------------------------------------------------------
void SpriteTexCanvas::draw()
{
	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GetSize().x, GetSize().y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw background
	drawCheckeredBackground();

	// Draw texture
	OpenGL::setColour(colour_);
	if (texture_ && !icon_)
	{
		// Sprite
		glEnable(GL_TEXTURE_2D);
		Drawing::drawTextureWithin(texture_, 0, 0, GetSize().x, GetSize().y, 4, 2);
	}
	else if (texture_ && icon_)
	{
		// Icon
		glEnable(GL_TEXTURE_2D);
		Drawing::drawTextureWithin(texture_, 0, 0, GetSize().x, GetSize().y, 0, 0.25);
	}

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


// ----------------------------------------------------------------------------
// ThingDirCanvas Class Functions
//
// An OpenGL canvas that shows a direction and circles for each of the 8
// 'standard' directions, clicking within one of the circles will set the
// direction
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ThingDirCanvas::ThingDirCanvas
//
// ThingDirCanvas class constructor
// ----------------------------------------------------------------------------
ThingDirCanvas::ThingDirCanvas(AngleControl* parent) : OGLCanvas(parent, -1, true, 15)
{
	// Init variables
	point_hl_ = -1;
	last_check_ = 0;
	point_sel_ = -1;
	parent_ = parent;

	// Get system panel background colour
	wxColour bgcolwx = Drawing::getPanelBGColour();
	col_bg_.set(COLWX(bgcolwx));

	// Get system text colour
	wxColour textcol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	col_fg_.set(COLWX(textcol));

	// Setup dir points
	double rot = 0;
	for (int a = 0; a < 8; a++)
	{
		dir_points_.push_back(fpoint2_t(sin(rot), 0 - cos(rot)));
		rot -= (3.1415926535897932384626433832795 * 2) / 8.0;
	}
	
	// Bind Events
	Bind(wxEVT_MOTION, &ThingDirCanvas::onMouseEvent, this);
	Bind(wxEVT_LEAVE_WINDOW, &ThingDirCanvas::onMouseEvent, this);
	Bind(wxEVT_LEFT_DOWN, &ThingDirCanvas::onMouseEvent, this);

	// Fixed size
	auto size = UI::scalePx(128);
	SetInitialSize(wxSize(size, size));
	SetMaxSize(wxSize(size, size));
}

// ----------------------------------------------------------------------------
// ThingDirCanvas::setAngle
//
// Sets the selected angle point based on [angle]
// ----------------------------------------------------------------------------
void ThingDirCanvas::setAngle(int angle)
{
	// Clamp angle
	while (angle >= 360)
		angle -= 360;
	while (angle < 0)
		angle += 360;

	// Set selected dir point (if any)
	if (!dir_points_.empty())
	{
		switch (angle)
		{
		case 0:		point_sel_ = 6; break;
		case 45:	point_sel_ = 7; break;
		case 90:	point_sel_ = 0; break;
		case 135:	point_sel_ = 1; break;
		case 180:	point_sel_ = 2; break;
		case 225:	point_sel_ = 3; break;
		case 270:	point_sel_ = 4; break;
		case 315:	point_sel_ = 5; break;
		default:	point_sel_ = -1; break;
		}
	}

	Refresh();
}

// ----------------------------------------------------------------------------
// ThingDirCanvas::draw
//
// Draws the control
// ----------------------------------------------------------------------------
void ThingDirCanvas::draw()
{
	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.2, 1.2, 1.2, -1.2, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(col_bg_.fr(), col_bg_.fg(), col_bg_.fb(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Draw angle ring
	glDisable(GL_TEXTURE_2D);
	glLineWidth(1.5f);
	glEnable(GL_LINE_SMOOTH);
	rgba_t col_faded(col_bg_.r * 0.6 + col_fg_.r * 0.4,
		col_bg_.g * 0.6 + col_fg_.g * 0.4,
		col_bg_.b * 0.6 + col_fg_.b * 0.4);
	Drawing::drawEllipse(fpoint2_t(0, 0), 1, 1, 48, col_faded);

	// Draw dir points
	for (unsigned a = 0; a < dir_points_.size(); a++)
	{
		Drawing::drawFilledEllipse(dir_points_[a], 0.12, 0.12, 8, col_bg_);
		Drawing::drawEllipse(dir_points_[a], 0.12, 0.12, 16, col_fg_);
	}

	// Draw angle arrow
	glLineWidth(2.0f);
	if (parent_->angleSet())
	{
		fpoint2_t tip = MathStuff::rotatePoint(fpoint2_t(0, 0), fpoint2_t(0.8, 0), -parent_->angle());
		Drawing::drawArrow(tip, fpoint2_t(0, 0), col_fg_, false, 1.2, 0.2);
	}

	// Draw hover point
	glPointSize(8.0f);
	glEnable(GL_POINT_SMOOTH);
	if (point_hl_ >= 0 && point_hl_ < (int)dir_points_.size())
	{
		OpenGL::setColour(col_faded);
		glBegin(GL_POINTS);
		glVertex2d(dir_points_[point_hl_].x, dir_points_[point_hl_].y);
		glEnd();
	}

	// Draw selected point
	if (parent_->angleSet() && point_sel_ >= 0 && point_sel_ < (int)dir_points_.size())
	{
		OpenGL::setColour(col_fg_);
		glBegin(GL_POINTS);
		glVertex2d(dir_points_[point_sel_].x, dir_points_[point_sel_].y);
		glEnd();
	}

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


// ----------------------------------------------------------------------------
//
// ThingDirCanvas Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ThingDirCanvas::onMouseEvent
//
// Called when a mouse event happens in the canvas
// ----------------------------------------------------------------------------
void ThingDirCanvas::onMouseEvent(wxMouseEvent& e)
{
	// Motion
	if (e.Moving())
	{
		auto last_point = point_hl_;
		if (App::runTimer() > last_check_ + 15)
		{
			// Get cursor position in canvas coordinates
			double x = -1.2 + ((double)e.GetX() / (double)GetSize().x) * 2.4;
			double y = -1.2 + ((double)e.GetY() / (double)GetSize().y) * 2.4;
			fpoint2_t cursor_pos(x, y);

			// Find closest dir point to cursor
			point_hl_ = -1;
			double min_dist = 0.3;
			for (unsigned a = 0; a < dir_points_.size(); a++)
			{
				double dist = MathStuff::distance(cursor_pos, dir_points_[a]);
				if (dist < min_dist)
				{
					point_hl_ = a;
					min_dist = dist;
				}
			}

			last_check_ = App::runTimer();
		}

		if (last_point != point_hl_)
			Refresh();
	}

	// Leaving
	else if (e.Leaving())
	{
		point_hl_ = -1;
		Refresh();
	}

	// Left click
	else if (e.LeftDown())
	{
		if (point_hl_ >= 0)
		{
			point_sel_ = point_hl_;
			int angle = 0;
			switch (point_sel_)
			{
			case 6: angle = 0; break;
			case 7: angle = 45; break;
			case 0: angle = 90; break;
			case 1: angle = 135; break;
			case 2: angle = 180; break;
			case 3: angle = 225; break;
			case 4: angle = 270; break;
			case 5: angle = 315; break;
			default: angle = 0; break;
			}

			parent_->setAngle(angle, false);
			Refresh();
		}
	}

	e.Skip();
}


// ----------------------------------------------------------------------------
//
// AngleControl Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// AngleControl::AngleControl
//
// AngleControl class constructor
// ----------------------------------------------------------------------------
AngleControl::AngleControl(wxWindow* parent) : wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Angle visual control
	sizer->Add(dc_angle_ = new ThingDirCanvas(this), 1, wxEXPAND|wxALL, UI::pad());

	// Angle text box
	text_angle_ = new NumberTextCtrl(this);
	sizer->Add(text_angle_, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, UI::pad());

	// Bind events
	text_angle_->Bind(wxEVT_TEXT, &AngleControl::onAngleTextChanged, this);
}

// ----------------------------------------------------------------------------
// AngleControl::getAngle
//
// Returns the current angle
// ----------------------------------------------------------------------------
int AngleControl::angle(int base)
{
	return text_angle_->getNumber(base);
}

// ----------------------------------------------------------------------------
// AngleControl::setAngle
//
// Sets the angle to display
// ----------------------------------------------------------------------------
void AngleControl::setAngle(int angle, bool update_visual)
{
	angle_ = angle;
	text_angle_->setNumber(angle);

	if (update_visual)
		updateAngle();
}

// ----------------------------------------------------------------------------
// AngleControl::updateAngle
//
// Updates the visual angle control
// ----------------------------------------------------------------------------
void AngleControl::updateAngle()
{
	dc_angle_->setAngle(angle_);
	dc_angle_->Refresh();
}

// ----------------------------------------------------------------------------
// AngleControl::angleSet
//
// Returns true if an angle is specified
// ----------------------------------------------------------------------------
bool AngleControl::angleSet()
{
	return !(text_angle_->GetValue().IsEmpty());
}


// ----------------------------------------------------------------------------
//
// AngleControl Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// AngleControl::onAngleTextChanged
//
// Called when the angle text box is changed
// ----------------------------------------------------------------------------
void AngleControl::onAngleTextChanged(wxCommandEvent& e)
{
	angle_ = text_angle_->getNumber();
	updateAngle();
}


// ----------------------------------------------------------------------------
//
// ThingPropsPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ThingPropsPanel::ThingPropsPanel
//
// ThingPropsPanel class constructor
// ----------------------------------------------------------------------------
ThingPropsPanel::ThingPropsPanel(wxWindow* parent) : PropsPanelBase(parent)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Tabs
	stc_tabs_ = STabCtrl::createControl(this);
	sizer->Add(stc_tabs_, 1, wxEXPAND|wxALL, UI::pad());

	// General tab
	stc_tabs_->AddPage(setupGeneralTab(), "General");

	// Extra Flags tab
	if (!udmf_flags_extra_.empty())
		stc_tabs_->AddPage(setupExtraFlagsTab(), "Extra Flags");

	// Special tab
	if (MapEditor::editContext().mapDesc().format != MAP_DOOM)
	{
		panel_special_ = new ActionSpecialPanel(this, false);
		stc_tabs_->AddPage(WxUtils::createPadPanel(stc_tabs_, panel_special_), "Special");
	}

	// Args tab
	if (MapEditor::editContext().mapDesc().format != MAP_DOOM)
	{
		panel_args_ = new ArgsPanel(this);
		stc_tabs_->AddPage(WxUtils::createPadPanel(stc_tabs_, panel_args_), "Args");
		if (panel_special_)
			panel_special_->setArgsPanel(panel_args_);
	}

	// Other Properties tab
	stc_tabs_->AddPage(mopp_other_props_ = new MapObjectPropsPanel(stc_tabs_, true), "Other Properties");
	mopp_other_props_->hideFlags(true);
	mopp_other_props_->hideProperty("height");
	mopp_other_props_->hideProperty("angle");
	mopp_other_props_->hideProperty("type");
	mopp_other_props_->hideProperty("id");
	mopp_other_props_->hideProperty("special");
	mopp_other_props_->hideProperty("arg0");
	mopp_other_props_->hideProperty("arg1");
	mopp_other_props_->hideProperty("arg2");
	mopp_other_props_->hideProperty("arg3");
	mopp_other_props_->hideProperty("arg4");

	// Bind events
	gfx_sprite_->Bind(wxEVT_LEFT_DOWN, &ThingPropsPanel::onSpriteClicked, this);

	Layout();
}

// ----------------------------------------------------------------------------
// ThingPropsPanel::setupGeneralTab
//
// Creates and sets up the 'General' properties tab
// ----------------------------------------------------------------------------
wxPanel* ThingPropsPanel::setupGeneralTab()
{
	int map_format = MapEditor::editContext().mapDesc().format;

	// Create panel
	wxPanel* panel = new wxPanel(stc_tabs_, -1);

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// --- Flags ---
	wxStaticBox* frame = new wxStaticBox(panel, -1, "Flags");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, UI::pad());

	// Init flags
	wxGridBagSizer* gb_sizer = new wxGridBagSizer(UI::pad() / 2, UI::pad());
	framesizer->Add(gb_sizer, 1, wxEXPAND|wxALL, UI::pad());
	int row = 0;
	int col = 0;

	// Get all UDMF properties
	auto& props = Game::configuration().allUDMFProperties(MOBJ_THING);

	// UDMF flags
	if (map_format == MAP_UDMF)
	{
		// Get all udmf flag properties
		vector<string> flags;
		for (auto& i : props)
		{
			if (i.second.isFlag())
			{
				if (i.second.showAlways())
				{
					flags.push_back(i.second.name());
					udmf_flags_.push_back(i.second.propName());
				}
				else
					udmf_flags_extra_.push_back(i.second.propName());
			}
		}

		// Add flag checkboxes
		int flag_mid = flags.size() / 3;
		if (flags.size() % 3 == 0) flag_mid--;
		for (unsigned a = 0; a < flags.size(); a++)
		{
			wxCheckBox* cb_flag = new wxCheckBox(panel, -1, flags[a], wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
			gb_sizer->Add(cb_flag, { row++, col }, { 1, 1 }, wxEXPAND);
			cb_flags_.push_back(cb_flag);

			if (row > flag_mid)
			{
				row = 0;
				col++;
			}
		}
	}

	// Non-UDMF flags
	else
	{
		// Add flag checkboxes
		int flag_mid = Game::configuration().nThingFlags() / 3;
		if (Game::configuration().nThingFlags() % 3 == 0) flag_mid--;
		for (int a = 0; a < Game::configuration().nThingFlags(); a++)
		{
			wxCheckBox* cb_flag = new wxCheckBox(
				panel,
				-1,
				Game::configuration().thingFlag(a),
				wxDefaultPosition,
				wxDefaultSize,
				wxCHK_3STATE
			);
			gb_sizer->Add(cb_flag, { row++, col }, { 1, 1 }, wxEXPAND);
			cb_flags_.push_back(cb_flag);

			if (row > flag_mid)
			{
				row = 0;
				col++;
			}
		}
	}

	gb_sizer->AddGrowableCol(0, 1);
	gb_sizer->AddGrowableCol(1, 1);
	gb_sizer->AddGrowableCol(2, 1);

	// Type
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, UI::pad());
	frame = new wxStaticBox(panel, -1, "Type");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 1, wxEXPAND|wxRIGHT, UI::pad());
	framesizer->Add(gfx_sprite_ = new SpriteTexCanvas(panel), 1, wxEXPAND|wxALL, UI::pad());
	framesizer->Add(label_type_ = new wxStaticText(panel, -1, ""), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, UI::pad());

	// Direction
	frame = new wxStaticBox(panel, -1, "Direction");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 0, wxEXPAND);
	framesizer->Add(ac_direction_ = new AngleControl(panel), 1, wxEXPAND);
	
#ifdef __WXMSW__
	//ac_direction->SetBackgroundColour(stc_tabs->GetThemeBackgroundColour());
#endif

	if (map_format != MAP_DOOM)
	{
		// Id
		gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
		sizer->Add(gb_sizer, 0, wxEXPAND|wxALL, UI::pad());
		gb_sizer->Add(new wxStaticText(panel, -1, "TID:"), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		gb_sizer->Add(text_id_ = new NumberTextCtrl(panel), { 0, 1 }, { 1, 1 }, wxEXPAND);

		// Z Height
		gb_sizer->Add(new wxStaticText(panel, -1, "Z Height:"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		gb_sizer->Add(text_height_ = new NumberTextCtrl(panel), { 1, 1 }, { 1, 1 }, wxEXPAND);
		if (map_format == MAP_UDMF)
			text_height_->allowDecimal(true);

		gb_sizer->AddGrowableCol(1, 1);
	}

	return panel;
}

// ----------------------------------------------------------------------------
// ThingPropsPanel::setupExtraFlagsTab
//
// Creates and sets up the 'Extra Flags' tab
// ----------------------------------------------------------------------------
wxPanel* ThingPropsPanel::setupExtraFlagsTab()
{
	// Create panel
	wxPanel* panel = new wxPanel(stc_tabs_, -1);

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Init flags
	wxGridBagSizer* gb_sizer_flags = new wxGridBagSizer(UI::pad() / 2, UI::pad());
	sizer->Add(gb_sizer_flags, 1, wxEXPAND|wxALL, UI::pad());
	int row = 0;
	int col = 0;

	// Get all extra flag names
	vector<string> flags;
	for (unsigned a = 0; a < udmf_flags_extra_.size(); a++)
	{
		UDMFProperty* prop = Game::configuration().getUDMFProperty(udmf_flags_extra_[a], MOBJ_THING);
		flags.push_back(prop->name());
	}

	// Add flag checkboxes
	int flag_mid = flags.size() / 3;
	if (flags.size() % 3 == 0) flag_mid--;
	for (unsigned a = 0; a < flags.size(); a++)
	{
		wxCheckBox* cb_flag = new wxCheckBox(panel, -1, flags[a], wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
		gb_sizer_flags->Add(cb_flag, wxGBPosition(row++, col), wxDefaultSpan, wxEXPAND);
		cb_flags_extra_.push_back(cb_flag);

		if (row > flag_mid)
		{
			row = 0;
			col++;
		}
	}

	gb_sizer_flags->AddGrowableCol(0, 1);
	gb_sizer_flags->AddGrowableCol(1, 1);
	gb_sizer_flags->AddGrowableCol(2, 1);

	return panel;
}

// ----------------------------------------------------------------------------
// ThingPropsPanel::openObjects
//
// Loads values from things in [objects]
// ----------------------------------------------------------------------------
void ThingPropsPanel::openObjects(vector<MapObject*>& objects)
{
	if (objects.empty())
		return;

	int map_format = MapEditor::editContext().mapDesc().format;
	int ival;
	double fval;

	// Load flags
	if (map_format == MAP_UDMF)
	{
		bool val = false;
		for (unsigned a = 0; a < udmf_flags_.size(); a++)
		{
			if (MapObject::multiBoolProperty(objects, udmf_flags_[a], val))
				cb_flags_[a]->SetValue(val);
			else
				cb_flags_[a]->Set3StateValue(wxCHK_UNDETERMINED);
		}
	}
	else
	{
		for (int a = 0; a < Game::configuration().nThingFlags(); a++)
		{
			// Set initial flag checked value
			cb_flags_[a]->SetValue(Game::configuration().thingFlagSet(a, (MapThing*)objects[0]));

			// Go through subsequent things
			for (unsigned b = 1; b < objects.size(); b++)
			{
				// Check for mismatch			
				if (cb_flags_[a]->GetValue() != Game::configuration().thingFlagSet(a, (MapThing*)objects[b]))
				{
					// Set undefined
					cb_flags_[a]->Set3StateValue(wxCHK_UNDETERMINED);
					break;
				}
			}
		}
	}

	// Type
	type_current_ = 0;
	if (MapObject::multiIntProperty(objects, "type", type_current_))
	{
		auto& tt = Game::configuration().thingType(type_current_);
		gfx_sprite_->setSprite(tt);
		label_type_->SetLabel(S_FMT("%d: %s", type_current_, tt.name()));
		label_type_->Wrap(136);
	}

	// Special
	ival = -1;
	if (panel_special_)
	{
		if (MapObject::multiIntProperty(objects, "special", ival))
			panel_special_->setSpecial(ival);
	}

	// Args
	if (map_format == MAP_HEXEN || map_format == MAP_UDMF)
	{
		// Setup
		if (ival > 0)
		{
			auto& as = Game::configuration().actionSpecial(ival).argSpec();
			panel_args_->setup(as, (map_format == MAP_UDMF));
		}
		else
		{
			auto& as = Game::configuration().thingType(type_current_).argSpec();
			panel_args_->setup(as, (map_format == MAP_UDMF));
		}

		// Load values
		int args[5] = { -1, -1, -1, -1, -1 };
		for (unsigned a = 0; a < 5; a++)
			MapObject::multiIntProperty(objects, S_FMT("arg%d", a), args[a]);
		panel_args_->setValues(args);
	}

	// Direction
	if (MapObject::multiIntProperty(objects, "angle", ival))
		ac_direction_->setAngle(ival);

	// Id
	if (map_format != MAP_DOOM && MapObject::multiIntProperty(objects, "id", ival))
		text_id_->setNumber(ival);

	// Z Height
	if (map_format == MAP_HEXEN && MapObject::multiIntProperty(objects, "height", ival))
		text_height_->setNumber(ival);
	else if (map_format == MAP_UDMF && MapObject::multiFloatProperty(objects, "height", fval))
		text_height_->setDecNumber(fval);

	// Load other properties
	mopp_other_props_->openObjects(objects);

	// Update internal objects list
	this->objects_.clear();
	for (unsigned a = 0; a < objects.size(); a++)
		this->objects_.push_back(objects[a]);

	// Update layout
	Layout();
	Refresh();
}

// ----------------------------------------------------------------------------
// ThingPropsPanel::applyChanges
//
// Applies values to currently open things
// ----------------------------------------------------------------------------
void ThingPropsPanel::applyChanges()
{
	int map_format = MapEditor::editContext().mapDesc().format;

	// Apply general properties
	for (unsigned a = 0; a < objects_.size(); a++)
	{
		// Flags
		if (udmf_flags_.empty())
		{
			for (int f = 0; f < Game::configuration().nThingFlags(); f++)
			{
				if (cb_flags_[f]->Get3StateValue() != wxCHK_UNDETERMINED)
					Game::configuration().setThingFlag(f, (MapThing*)objects_[a], cb_flags_[f]->GetValue());
			}
		}

		// UDMF flags
		else
		{
			for (unsigned f = 0; f < udmf_flags_.size(); f++)
			{
				if (cb_flags_[f]->Get3StateValue() != wxCHK_UNDETERMINED)
					objects_[a]->setBoolProperty(udmf_flags_[f], cb_flags_[f]->GetValue());
			}
		}

		// UDMF extra flags
		if (!udmf_flags_extra_.empty())
		{
			for (unsigned f = 0; f < udmf_flags_extra_.size(); f++)
			{
				if (cb_flags_extra_[f]->Get3StateValue() != wxCHK_UNDETERMINED)
					objects_[a]->setBoolProperty(udmf_flags_extra_[f], cb_flags_extra_[f]->GetValue());
			}
		}

		// Type
		if (type_current_ > 0)
			objects_[a]->setIntProperty("type", type_current_);

		// Direction
		if (ac_direction_->angleSet())
			objects_[a]->setIntProperty("angle", ac_direction_->angle(objects_[a]->intProperty("angle")));

		if (map_format != MAP_DOOM)
		{
			// ID
			if (!text_id_->GetValue().IsEmpty())
				objects_[a]->setIntProperty("id", text_id_->getNumber(objects_[a]->intProperty("id")));

			// Z Height
			if (!text_height_->GetValue().IsEmpty())
			{
				if (map_format == MAP_UDMF)
					objects_[a]->setFloatProperty(
						"height",
						text_height_->getDecNumber(objects_[a]->floatProperty("height"))
					);
				else
					objects_[a]->setIntProperty("height", text_height_->getNumber(objects_[a]->intProperty("height")));
			}
		}
	}

	// Special
	if (panel_special_)
		panel_special_->applyTo(objects_, true);

	mopp_other_props_->applyChanges();
}


// ----------------------------------------------------------------------------
//
// ThingPropsPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ThingPropsPanel::onSpriteClicked
//
// Called when the thing type sprite canvas is clicked
// ----------------------------------------------------------------------------
void ThingPropsPanel::onSpriteClicked(wxMouseEvent& e)
{
	ThingTypeBrowser browser(this, type_current_);
	if (browser.ShowModal() == wxID_OK)
	{
		// Get selected type
		type_current_ = browser.getSelectedType();
		auto& tt = Game::configuration().thingType(type_current_);

		// Update sprite
		gfx_sprite_->setSprite(tt);
		label_type_->SetLabel(tt.name());

		// Update args
		if (panel_args_)
		{
			auto& as = tt.argSpec();
			panel_args_->setup(as, (MapEditor::editContext().mapDesc().format == MAP_UDMF));
		}

		// Update layout
		Layout();
		Refresh();
	}
}
