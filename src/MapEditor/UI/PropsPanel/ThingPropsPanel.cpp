
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ThingPropsPanel.h"
#include "App.h"
#include "Game/Configuration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/UI/Dialogs/ActionSpecialDialog.h"
#include "MapEditor/UI/Dialogs/ThingTypeBrowser.h"
#include "MapObjectPropsPanel.h"
#include "OpenGL/Drawing.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/WxUtils.h"
#include "Utility/MathStuff.h"

using namespace slade;


// -----------------------------------------------------------------------------
// SpriteTexCanvas Class Functions
//
// A simple opengl canvas to display a thing sprite
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SpriteTexCanvas class constructor
// -----------------------------------------------------------------------------
SpriteTexCanvas::SpriteTexCanvas(wxWindow* parent) : OGLCanvas(parent, -1)
{
	wxWindow::SetWindowStyleFlag(wxBORDER_SIMPLE);
	SetInitialSize(wxutil::scaledSize(128, 128));
}

// -----------------------------------------------------------------------------
// Sets the texture to display
// -----------------------------------------------------------------------------
void SpriteTexCanvas::setSprite(const game::ThingType& type)
{
	texname_ = type.sprite();
	icon_    = false;
	colour_  = ColRGBA::WHITE;

	// Sprite
	texture_ = mapeditor::textureManager().sprite(texname_.ToStdString(), type.translation(), type.palette()).gl_id;

	// Icon
	if (!texture_)
	{
		texture_ = mapeditor::textureManager().editorImage(fmt::format("thing/{}", type.icon())).gl_id;
		colour_  = type.colour();
		icon_    = true;
	}

	// Unknown
	if (!texture_)
	{
		texture_ = mapeditor::textureManager().editorImage("thing/unknown").gl_id;
		icon_    = true;
	}

	Refresh();
}

// -----------------------------------------------------------------------------
// Draws the canvas content
// -----------------------------------------------------------------------------
void SpriteTexCanvas::draw()
{
	// Setup the viewport
	const wxSize size = GetSize() * GetContentScaleFactor();
	glViewport(0, 0, size.x, size.y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, size.x, size.y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (gl::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw background
	drawCheckeredBackground();

	// Draw texture
	gl::setColour(colour_);
	if (texture_ && !icon_)
	{
		// Sprite
		glEnable(GL_TEXTURE_2D);
		drawing::drawTextureWithin(texture_, 0, 0, size.x, size.y, 4, 2);
	}
	else if (texture_ && icon_)
	{
		// Icon
		glEnable(GL_TEXTURE_2D);
		drawing::drawTextureWithin(texture_, 0, 0, size.x, size.y, 0, 0.25);
	}

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


// -----------------------------------------------------------------------------
// ThingDirCanvas Class Functions
//
// An OpenGL canvas that shows a direction and circles for each of the 8
// 'standard' directions, clicking within one of the circles will set the
// direction
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ThingDirCanvas class constructor
// -----------------------------------------------------------------------------
ThingDirCanvas::ThingDirCanvas(AngleControl* parent) : OGLCanvas(parent, -1, true, 15), parent_{ parent }
{
	// Get system panel background colour
	auto bgcolwx = drawing::systemPanelBGColour();
	col_bg_.set(bgcolwx);

	// Get system text colour
	auto textcol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	col_fg_.set(textcol);

	// Setup dir points
	double rot = 0;
	for (int a = 0; a < 8; a++)
	{
		dir_points_.emplace_back(sin(rot), 0 - cos(rot));
		rot -= (3.1415926535897932384626433832795 * 2) / 8.0;
	}

	// Bind Events
	Bind(wxEVT_MOTION, &ThingDirCanvas::onMouseEvent, this);
	Bind(wxEVT_LEAVE_WINDOW, &ThingDirCanvas::onMouseEvent, this);
	Bind(wxEVT_LEFT_DOWN, &ThingDirCanvas::onMouseEvent, this);

	// Fixed size
	auto size = ui::scalePx(128);
	SetInitialSize(wxSize(size, size));
	wxWindowBase::SetMaxSize(wxSize(size, size));
}

// -----------------------------------------------------------------------------
// Sets the selected angle point based on [angle]
// -----------------------------------------------------------------------------
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
		case 0: point_sel_ = 6; break;
		case 45: point_sel_ = 7; break;
		case 90: point_sel_ = 0; break;
		case 135: point_sel_ = 1; break;
		case 180: point_sel_ = 2; break;
		case 225: point_sel_ = 3; break;
		case 270: point_sel_ = 4; break;
		case 315: point_sel_ = 5; break;
		default: point_sel_ = -1; break;
		}
	}

	Refresh();
}

// -----------------------------------------------------------------------------
// Draws the control
// -----------------------------------------------------------------------------
void ThingDirCanvas::draw()
{
	// Setup the viewport
	const wxSize size = GetSize() * GetContentScaleFactor();
	glViewport(0, 0, size.x, size.y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.2, 1.2, 1.2, -1.2, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(col_bg_.fr(), col_bg_.fg(), col_bg_.fb(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw angle ring
	glDisable(GL_TEXTURE_2D);
	glLineWidth(1.5f);
	glEnable(GL_LINE_SMOOTH);
	ColRGBA col_faded(
		col_bg_.r * 0.6 + col_fg_.r * 0.4, col_bg_.g * 0.6 + col_fg_.g * 0.4, col_bg_.b * 0.6 + col_fg_.b * 0.4);
	drawing::drawEllipse(Vec2d(0, 0), 1, 1, 48, col_faded);

	// Draw dir points
	for (auto dir_point : dir_points_)
	{
		drawing::drawFilledEllipse(dir_point, 0.12, 0.12, 8, col_bg_);
		drawing::drawEllipse(dir_point, 0.12, 0.12, 16, col_fg_);
	}

	// Draw angle arrow
	glLineWidth(2.0f);
	if (parent_->angleSet())
	{
		auto tip = math::rotatePoint(Vec2d(0, 0), Vec2d(0.8, 0), -parent_->angle());
		drawing::drawArrow(tip, Vec2d(0, 0), col_fg_, false, 1.2, 0.2);
	}

	// Draw hover point
	glPointSize(8.0f);
	glEnable(GL_POINT_SMOOTH);
	if (point_hl_ >= 0 && point_hl_ < (int)dir_points_.size())
	{
		gl::setColour(col_faded);
		glBegin(GL_POINTS);
		glVertex2d(dir_points_[point_hl_].x, dir_points_[point_hl_].y);
		glEnd();
	}

	// Draw selected point
	if (parent_->angleSet() && point_sel_ >= 0 && point_sel_ < (int)dir_points_.size())
	{
		gl::setColour(col_fg_);
		glBegin(GL_POINTS);
		glVertex2d(dir_points_[point_sel_].x, dir_points_[point_sel_].y);
		glEnd();
	}

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


// -----------------------------------------------------------------------------
//
// ThingDirCanvas Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a mouse event happens in the canvas
// -----------------------------------------------------------------------------
void ThingDirCanvas::onMouseEvent(wxMouseEvent& e)
{
	// Motion
	if (e.Moving())
	{
		auto last_point = point_hl_;
		if (app::runTimer() > last_check_ + 15)
		{
			// Get cursor position in canvas coordinates
			const wxSize size = GetSize();
			double       x    = -1.2 + ((double)e.GetX() / (double)size.x) * 2.4;
			double       y    = -1.2 + ((double)e.GetY() / (double)size.y) * 2.4;
			Vec2d        cursor_pos(x, y);

			// Find closest dir point to cursor
			point_hl_       = -1;
			double min_dist = 0.3;
			for (unsigned a = 0; a < dir_points_.size(); a++)
			{
				double dist = math::distance(cursor_pos, dir_points_[a]);
				if (dist < min_dist)
				{
					point_hl_ = a;
					min_dist  = dist;
				}
			}

			last_check_ = app::runTimer();
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
			int angle  = 0;
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


// -----------------------------------------------------------------------------
//
// AngleControl Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// AngleControl class constructor
// -----------------------------------------------------------------------------
AngleControl::AngleControl(wxWindow* parent) : wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Angle visual control
	sizer->Add(dc_angle_ = new ThingDirCanvas(this), 1, wxEXPAND | wxALL, ui::pad());

	// Angle text box
	text_angle_ = new NumberTextCtrl(this);
	sizer->Add(text_angle_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());

	// Bind events
	text_angle_->Bind(wxEVT_TEXT, &AngleControl::onAngleTextChanged, this);
}

// -----------------------------------------------------------------------------
// Returns the current angle
// -----------------------------------------------------------------------------
int AngleControl::angle(int base) const
{
	return text_angle_->number(base);
}

// -----------------------------------------------------------------------------
// Sets the angle to display
// -----------------------------------------------------------------------------
void AngleControl::setAngle(int angle, bool update_visual)
{
	angle_ = angle;
	text_angle_->setNumber(angle);

	if (update_visual)
		updateAngle();
}

// -----------------------------------------------------------------------------
// Updates the visual angle control
// -----------------------------------------------------------------------------
void AngleControl::updateAngle() const
{
	dc_angle_->setAngle(angle_);
	dc_angle_->Refresh();
}

// -----------------------------------------------------------------------------
// Returns true if an angle is specified
// -----------------------------------------------------------------------------
bool AngleControl::angleSet() const
{
	return !(text_angle_->GetValue().IsEmpty());
}


// -----------------------------------------------------------------------------
//
// AngleControl Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the angle text box is changed
// -----------------------------------------------------------------------------
void AngleControl::onAngleTextChanged(wxCommandEvent& e)
{
	angle_ = text_angle_->number();
	updateAngle();
}


// -----------------------------------------------------------------------------
//
// ThingPropsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ThingPropsPanel class constructor
// -----------------------------------------------------------------------------
ThingPropsPanel::ThingPropsPanel(wxWindow* parent) : PropsPanelBase(parent)
{
	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Tabs
	stc_tabs_ = STabCtrl::createControl(this);
	sizer->Add(stc_tabs_, 1, wxEXPAND | wxALL, ui::pad());

	// General tab
	stc_tabs_->AddPage(setupGeneralTab(), "General");

	// Extra Flags tab
	if (!udmf_flags_extra_.empty())
		stc_tabs_->AddPage(setupExtraFlagsTab(), "Extra Flags");

	// Special tab
	if (mapeditor::editContext().mapDesc().format != MapFormat::Doom)
	{
		panel_special_ = new ActionSpecialPanel(this, false);
		stc_tabs_->AddPage(wxutil::createPadPanel(stc_tabs_, panel_special_), "Special");
	}

	// Args tab
	if (mapeditor::editContext().mapDesc().format != MapFormat::Doom)
	{
		panel_args_ = new ArgsPanel(this);
		stc_tabs_->AddPage(wxutil::createPadPanel(stc_tabs_, panel_args_), "Args");
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

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Creates and sets up the 'General' properties tab
// -----------------------------------------------------------------------------
wxPanel* ThingPropsPanel::setupGeneralTab()
{
	auto map_format = mapeditor::editContext().mapDesc().format;

	// Create panel
	auto panel = new wxPanel(stc_tabs_, -1);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// --- Flags ---
	auto frame      = new wxStaticBox(panel, -1, "Flags");
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND | wxALL, ui::pad());

	// Init flags
	auto gb_sizer = new wxGridBagSizer(ui::pad() / 2, ui::pad());
	framesizer->Add(gb_sizer, 1, wxEXPAND | wxALL, ui::pad());
	int row = 0;
	int col = 0;

	// Get all UDMF properties
	auto& props = game::configuration().allUDMFProperties(MapObject::Type::Thing);

	// UDMF flags
	if (map_format == MapFormat::UDMF)
	{
		// Get all udmf flag properties
		vector<wxString> flags;
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
		if (flags.size() % 3 == 0)
			flag_mid--;
		for (const auto& flag : flags)
		{
			auto cb_flag = new wxCheckBox(panel, -1, flag, wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
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
		int flag_mid = game::configuration().nThingFlags() / 3;
		if (game::configuration().nThingFlags() % 3 == 0)
			flag_mid--;
		for (int a = 0; a < game::configuration().nThingFlags(); a++)
		{
			auto cb_flag = new wxCheckBox(
				panel, -1, game::configuration().thingFlag(a), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
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
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxALL, ui::pad());
	frame      = new wxStaticBox(panel, -1, "Type");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 1, wxEXPAND | wxRIGHT, ui::pad());
	framesizer->Add(gfx_sprite_ = new SpriteTexCanvas(panel), 1, wxEXPAND | wxALL, ui::pad());
	framesizer->Add(
		label_type_ = new wxStaticText(panel, -1, ""), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());

	// Direction
	frame      = new wxStaticBox(panel, -1, "Direction");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 0, wxEXPAND);
	framesizer->Add(ac_direction_ = new AngleControl(panel), 1, wxEXPAND);

#ifdef __WXMSW__
	// ac_direction->SetBackgroundColour(stc_tabs->GetThemeBackgroundColour());
#endif

	if (map_format != MapFormat::Doom)
	{
		// Id
		gb_sizer = new wxGridBagSizer(ui::pad(), ui::pad());
		sizer->Add(gb_sizer, 0, wxEXPAND | wxALL, ui::pad());
		gb_sizer->Add(new wxStaticText(panel, -1, "TID:"), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		gb_sizer->Add(text_id_ = new NumberTextCtrl(panel), { 0, 1 }, { 1, 1 }, wxEXPAND | wxALIGN_CENTER_VERTICAL);
		gb_sizer->Add(btn_new_id_ = new wxButton(panel, -1, "New TID"), { 0, 2 }, { 1, 1 });

		// Z Height
		gb_sizer->Add(new wxStaticText(panel, -1, "Z Height:"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		gb_sizer->Add(text_height_ = new NumberTextCtrl(panel), { 1, 1 }, { 1, 2 }, wxEXPAND);
		if (map_format == MapFormat::UDMF)
			text_height_->allowDecimal(true);

		gb_sizer->AddGrowableCol(1, 1);

		// 'New TID' button event
		btn_new_id_->Bind(
			wxEVT_BUTTON,
			[&](wxCommandEvent&) { text_id_->setNumber(mapeditor::editContext().map().things().firstFreeId()); });
	}

	return panel;
}

// -----------------------------------------------------------------------------
// Creates and sets up the 'Extra Flags' tab
// -----------------------------------------------------------------------------
wxPanel* ThingPropsPanel::setupExtraFlagsTab()
{
	// Create panel
	auto panel = new wxPanel(stc_tabs_, -1);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Init flags
	auto gb_sizer_flags = new wxGridBagSizer(ui::pad() / 2, ui::pad());
	sizer->Add(gb_sizer_flags, 1, wxEXPAND | wxALL, ui::pad());
	int row = 0;
	int col = 0;

	// Get all extra flag names
	vector<wxString> flags;
	for (const auto& a : udmf_flags_extra_)
	{
		auto prop = game::configuration().getUDMFProperty(a.ToStdString(), MapObject::Type::Thing);
		flags.push_back(prop->name());
	}

	// Add flag checkboxes
	int flag_mid = flags.size() / 3;
	if (flags.size() % 3 == 0)
		flag_mid--;
	for (const auto& flag : flags)
	{
		auto cb_flag = new wxCheckBox(panel, -1, flag, wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
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

// -----------------------------------------------------------------------------
// Loads values from things in [objects]
// -----------------------------------------------------------------------------
void ThingPropsPanel::openObjects(vector<MapObject*>& objects)
{
	if (objects.empty())
		return;

	auto   map_format = mapeditor::editContext().mapDesc().format;
	int    ival;
	double fval;

	// Load flags
	if (map_format == MapFormat::UDMF)
	{
		bool val = false;
		for (unsigned a = 0; a < udmf_flags_.size(); a++)
		{
			if (MapObject::multiBoolProperty(objects, udmf_flags_[a].ToStdString(), val))
				cb_flags_[a]->SetValue(val);
			else
				cb_flags_[a]->Set3StateValue(wxCHK_UNDETERMINED);
		}
	}
	else
	{
		for (int a = 0; a < game::configuration().nThingFlags(); a++)
		{
			// Set initial flag checked value
			cb_flags_[a]->SetValue(game::configuration().thingFlagSet(a, (MapThing*)objects[0]));

			// Go through subsequent things
			for (unsigned b = 1; b < objects.size(); b++)
			{
				// Check for mismatch
				if (cb_flags_[a]->GetValue() != game::configuration().thingFlagSet(a, (MapThing*)objects[b]))
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
		auto& tt = game::configuration().thingType(type_current_);
		gfx_sprite_->setSprite(tt);
		label_type_->SetLabel(wxString::Format("%d: %s", type_current_, tt.name()));
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
	if (map_format == MapFormat::Hexen || map_format == MapFormat::UDMF)
	{
		// Setup
		if (ival > 0)
		{
			auto& as = game::configuration().actionSpecial(ival).argSpec();
			panel_args_->setup(as, (map_format == MapFormat::UDMF));
		}
		else
		{
			auto& as = game::configuration().thingType(type_current_).argSpec();
			panel_args_->setup(as, (map_format == MapFormat::UDMF));
		}

		// Load values
		int args[5] = { -1, -1, -1, -1, -1 };
		for (unsigned a = 0; a < 5; a++)
			MapObject::multiIntProperty(objects, fmt::format("arg{}", a), args[a]);
		panel_args_->setValues(args);
	}

	// Direction
	if (MapObject::multiIntProperty(objects, "angle", ival))
		ac_direction_->setAngle(ival);

	// Id
	if (map_format != MapFormat::Doom && MapObject::multiIntProperty(objects, "id", ival))
		text_id_->setNumber(ival);

	// Z Height
	if (map_format == MapFormat::Hexen && MapObject::multiIntProperty(objects, "height", ival))
		text_height_->setNumber(ival);
	else if (map_format == MapFormat::UDMF && MapObject::multiFloatProperty(objects, "height", fval))
		text_height_->setDecNumber(fval);

	// Load other properties
	mopp_other_props_->openObjects(objects);

	// Update internal objects list
	objects_.clear();
	for (auto object : objects)
		objects_.push_back(object);

	// Update layout
	Layout();
	Refresh();
}

// -----------------------------------------------------------------------------
// Applies values to currently open things
// -----------------------------------------------------------------------------
void ThingPropsPanel::applyChanges()
{
	auto map_format = mapeditor::editContext().mapDesc().format;

	// Apply general properties
	for (auto& object : objects_)
	{
		// Flags
		if (udmf_flags_.empty())
		{
			for (int f = 0; f < game::configuration().nThingFlags(); f++)
			{
				if (cb_flags_[f]->Get3StateValue() != wxCHK_UNDETERMINED)
					game::configuration().setThingFlag(f, (MapThing*)object, cb_flags_[f]->GetValue());
			}
		}

		// UDMF flags
		else
		{
			for (unsigned f = 0; f < udmf_flags_.size(); f++)
			{
				if (cb_flags_[f]->Get3StateValue() != wxCHK_UNDETERMINED)
					object->setBoolProperty(udmf_flags_[f].ToStdString(), cb_flags_[f]->GetValue());
			}
		}

		// UDMF extra flags
		if (!udmf_flags_extra_.empty())
		{
			for (unsigned f = 0; f < udmf_flags_extra_.size(); f++)
			{
				if (cb_flags_extra_[f]->Get3StateValue() != wxCHK_UNDETERMINED)
					object->setBoolProperty(udmf_flags_extra_[f].ToStdString(), cb_flags_extra_[f]->GetValue());
			}
		}

		// Type
		if (type_current_ > 0)
			object->setIntProperty("type", type_current_);

		// Direction
		if (ac_direction_->angleSet())
			object->setIntProperty("angle", ac_direction_->angle(object->intProperty("angle")));

		if (map_format != MapFormat::Doom)
		{
			// ID
			if (!text_id_->GetValue().IsEmpty())
				object->setIntProperty("id", text_id_->number(object->intProperty("id")));

			// Z Height
			if (!text_height_->GetValue().IsEmpty())
			{
				if (map_format == MapFormat::UDMF)
					object->setFloatProperty("height", text_height_->decNumber(object->floatProperty("height")));
				else
					object->setIntProperty("height", text_height_->number(object->intProperty("height")));
			}
		}
	}

	// Special
	if (panel_special_)
		panel_special_->applyTo(objects_, true);

	mopp_other_props_->applyChanges();
}


// -----------------------------------------------------------------------------
//
// ThingPropsPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the thing type sprite canvas is clicked
// -----------------------------------------------------------------------------
void ThingPropsPanel::onSpriteClicked(wxMouseEvent& e)
{
	ThingTypeBrowser browser(this, type_current_);
	if (browser.ShowModal() == wxID_OK)
	{
		// Get selected type
		type_current_ = browser.selectedType();
		auto& tt      = game::configuration().thingType(type_current_);

		// Update sprite
		gfx_sprite_->setSprite(tt);
		label_type_->SetLabel(tt.name());

		// Update args
		if (panel_args_)
		{
			auto& as = tt.argSpec();
			panel_args_->setup(as, (mapeditor::editContext().mapDesc().format == MapFormat::UDMF));
		}

		// Update layout
		Layout();
		Refresh();
	}
}
