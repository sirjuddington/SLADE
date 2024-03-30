
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Game/ActionSpecial.h"
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "Game/UDMFProperty.h"
#include "General/UI.h"
#include "Geometry/Geometry.h"
#include "Geometry/Rect.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/UI/ActionSpecialPanel.h"
#include "MapEditor/UI/ArgsPanel.h"
#include "MapEditor/UI/Dialogs/ThingTypeBrowser.h"
#include "MapObjectPropsPanel.h"
#include "OpenGL/Draw2D.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/SLADEMap.h"
#include "UI/Canvas/GLCanvas.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
// AngleControl Class Definition
//
// Need this here due to a circular dependency below, and I don't want to put
// this in the header since it isn't used outside this file
// -----------------------------------------------------------------------------
namespace slade
{
class AngleControl : public wxControl
{
public:
	AngleControl(wxWindow* parent);
	~AngleControl() override = default;

	int  angle(int base = 0) const;
	void setAngle(int angle, bool update_visual = true);
	void updateAngle() const;
	bool angleSet() const;

private:
	int             angle_      = 0;
	ThingDirCanvas* dc_angle_   = nullptr;
	NumberTextCtrl* text_angle_ = nullptr;

	void onAngleTextChanged(wxCommandEvent& e);
};
} // namespace slade


// -----------------------------------------------------------------------------
// SpriteTexCanvas Class
//
// A simple opengl canvas to display a thing sprite
// -----------------------------------------------------------------------------
class slade::SpriteTexCanvas : public GLCanvas
{
public:
	SpriteTexCanvas(wxWindow* parent) : GLCanvas(parent)
	{
		wxWindow::SetWindowStyleFlag(wxBORDER_SIMPLE);
		SetInitialSize(wxutil::scaledSize(128, 128));
	}

	~SpriteTexCanvas() override = default;

	wxString texName() const { return texname_; }

	// Sets the texture to display
	void setSprite(const game::ThingType& type)
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

	// Draws the canvas content
	void draw() override
	{
		gl::draw2d::Context dc(&view_);

		// Draw texture
		dc.colour  = colour_;
		dc.texture = texture_;
		if (texture_ && !icon_)
			dc.drawTextureWithin({ 0.0f, 0.0f, dc.viewSize().x, dc.viewSize().y }, 4.0f, 2.0f); // Sprite
		else if (texture_ && icon_)
			dc.drawTextureWithin({ 0.0f, 0.0f, dc.viewSize().x, dc.viewSize().y }, 0.0f, 0.25f); // Icon
	}

private:
	unsigned texture_ = 0;
	wxString texname_;
	ColRGBA  colour_ = ColRGBA::WHITE;
	bool     icon_   = false;
};


// -----------------------------------------------------------------------------
// ThingDirCanvas Class
//
// An OpenGL canvas that shows a direction and circles for each of the 8
// 'standard' directions, clicking within one of the circles will set the
// direction
// -----------------------------------------------------------------------------
namespace slade
{
class ThingDirCanvas : public wxPanel
{
public:
	ThingDirCanvas(AngleControl* parent);
	~ThingDirCanvas() override = default;

	void setAngle(int angle);

private:
	AngleControl* parent_ = nullptr;
	vector<Vec2d> dir_points_;
	int           point_hl_   = -1;
	int           point_sel_  = -1;
	long          last_check_ = 0;

	void onMouseEvent(wxMouseEvent& e);
	void onPaint(wxPaintEvent& e);
};

// -----------------------------------------------------------------------------
// ThingDirCanvas class constructor
// -----------------------------------------------------------------------------
ThingDirCanvas::ThingDirCanvas(AngleControl* parent) : wxPanel(parent), parent_{ parent }
{
	SetDoubleBuffered(true);

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
	Bind(wxEVT_PAINT, &ThingDirCanvas::onPaint, this);

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
		case 0:   point_sel_ = 6; break;
		case 45:  point_sel_ = 7; break;
		case 90:  point_sel_ = 0; break;
		case 135: point_sel_ = 1; break;
		case 180: point_sel_ = 2; break;
		case 225: point_sel_ = 3; break;
		case 270: point_sel_ = 4; break;
		case 315: point_sel_ = 5; break;
		default:  point_sel_ = -1; break;
		}
	}

	Refresh();
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
			double       x    = -1.2 + (static_cast<double>(e.GetX()) / static_cast<double>(size.x)) * 2.4;
			double       y    = -1.2 + (static_cast<double>(e.GetY()) / static_cast<double>(size.y)) * 2.4;
			Vec2d        cursor_pos(x, y);

			// Find closest dir point to cursor
			point_hl_       = -1;
			double min_dist = 0.3;
			for (unsigned a = 0; a < dir_points_.size(); a++)
			{
				double dist = glm::distance(cursor_pos, dir_points_[a]);
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
			case 6:  angle = 0; break;
			case 7:  angle = 45; break;
			case 0:  angle = 90; break;
			case 1:  angle = 135; break;
			case 2:  angle = 180; break;
			case 3:  angle = 225; break;
			case 4:  angle = 270; break;
			case 5:  angle = 315; break;
			default: angle = 0; break;
			}

			parent_->setAngle(angle, false);
			Refresh();
		}
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the canvas needs to be (re)painted
// -----------------------------------------------------------------------------
void ThingDirCanvas::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);
	auto      gc = wxGraphicsContext::Create(dc);

	auto half_size    = GetSize().x / 2;
	auto pad          = ui::scalePx(8);
	auto radius       = half_size - pad;
	auto point_radius = ui::scalePx(7);
	auto col_bg       = wxutil::systemPanelBGColour();
	auto col_fg       = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	auto pi           = wxGraphicsPenInfo(wxColour(col_fg.Red(), col_fg.Green(), col_fg.Blue(), 80), 1.75);

	// Draw angle ring
	gc->SetPen(gc->CreatePen(pi));
	gc->SetBrush(*wxTRANSPARENT_BRUSH);
	gc->DrawEllipse(pad, pad, radius * 2, radius * 2);

	// Draw dir points
	pi.Colour(col_fg);
	gc->SetPen(gc->CreatePen(pi));
	gc->SetBrush(wxBrush(col_bg));
	for (auto dir_point : dir_points_)
	{
		auto point = dir_point * static_cast<double>(radius);
		gc->DrawEllipse(
			half_size + point.x - point_radius, half_size + point.y - point_radius, point_radius * 2, point_radius * 2);
	}

	// Draw angle arrow
	if (parent_->angleSet())
	{
		auto                    tip = geometry::rotatePoint(Vec2d(0, 0), Vec2d(radius * 0.8, 0), -parent_->angle());
		vector<wxPoint2DDouble> points;
		for (auto line : geometry::arrowLines({ 0.0, 0.0, tip.x, tip.y }, pad, 60.0f))
		{
			line.move(half_size, half_size);
			gc->StrokeLine(line.x1(), line.y1(), line.x2(), line.y2());
		}
	}

	// Draw hover point
	gc->SetPen(*wxTRANSPARENT_PEN);
	auto pr = static_cast<double>(point_radius) * 0.7;
	if (point_hl_ >= 0 && point_hl_ < static_cast<int>(dir_points_.size()))
	{
		gc->SetBrush(wxBrush(wxColour(col_fg.Red(), col_fg.Green(), col_fg.Blue(), 80)));
		auto point = dir_points_[point_hl_] * static_cast<double>(radius);
		gc->DrawEllipse(half_size + point.x - pr, half_size + point.y - pr, pr * 2, pr * 2);
	}

	// Draw selected point
	if (parent_->angleSet() && point_sel_ >= 0 && point_sel_ < static_cast<int>(dir_points_.size()))
	{
		gc->SetBrush(wxBrush(col_fg));
		auto point = dir_points_[point_sel_] * static_cast<double>(radius);
		gc->DrawEllipse(half_size + point.x - pr, half_size + point.y - pr, pr * 2, pr * 2);
	}
}
} // namespace slade

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
	sizer->Add(dc_angle_ = new ThingDirCanvas(this), wxutil::sfWithBorder(1).Expand());

	// Angle text box
	sizer->Add(text_angle_ = new NumberTextCtrl(this), wxutil::sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

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
	sizer->Add(stc_tabs_, wxutil::sfWithBorder(1).Expand());

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
	namespace wx = wxutil;

	auto map_format = mapeditor::editContext().mapDesc().format;

	// Create panel
	auto panel = new wxPanel(stc_tabs_, -1);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// --- Flags ---
	auto frame      = new wxStaticBox(panel, -1, "Flags");
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wx::sfWithBorder().Expand());

	// Init flags
	auto gb_sizer = new wxGridBagSizer(ui::pad() / 2, ui::pad());
	framesizer->Add(gb_sizer, wx::sfWithBorder(1).Expand());
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
					flags.emplace_back(i.second.name());
					udmf_flags_.emplace_back(i.second.propName());
				}
				else
					udmf_flags_extra_.emplace_back(i.second.propName());
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
	sizer->Add(hbox, wx::sfWithBorder().Expand());
	frame      = new wxStaticBox(panel, -1, "Type");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, wx::sfWithBorder(1, wxRIGHT).Expand());
	framesizer->Add(gfx_sprite_ = new SpriteTexCanvas(panel), wx::sfWithBorder(1).Expand());
	framesizer->Add(
		label_type_ = new wxStaticText(panel, -1, ""), wx::sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Direction
	frame      = new wxStaticBox(panel, -1, "Direction");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 0, wxEXPAND);
	framesizer->Add(ac_direction_ = new AngleControl(panel), wxSizerFlags(1).Expand());

	if (map_format != MapFormat::Doom)
	{
		// Id
		gb_sizer = new wxGridBagSizer(ui::pad(), ui::pad());
		sizer->Add(gb_sizer, wx::sfWithBorder().Expand());
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
	sizer->Add(gb_sizer_flags, wxutil::sfWithBorder(1).Expand());
	int row = 0;
	int col = 0;

	// Get all extra flag names
	vector<wxString> flags;
	for (const auto& a : udmf_flags_extra_)
	{
		auto prop = game::configuration().getUDMFProperty(a.ToStdString(), MapObject::Type::Thing);
		flags.emplace_back(prop->name());
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
			cb_flags_[a]->SetValue(game::configuration().thingFlagSet(a, dynamic_cast<MapThing*>(objects[0])));

			// Go through subsequent things
			for (unsigned b = 1; b < objects.size(); b++)
			{
				// Check for mismatch
				if (cb_flags_[a]->GetValue()
					!= game::configuration().thingFlagSet(a, dynamic_cast<MapThing*>(objects[b])))
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
					game::configuration().setThingFlag(f, dynamic_cast<MapThing*>(object), cb_flags_[f]->GetValue());
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
