
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ThingPropsPanel.cpp
 * Description: UI for editing thing properties
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
#include "Game/Configuration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/UI/Dialogs/ActionSpecialDialog.h"
#include "MapEditor/UI/Dialogs/ThingTypeBrowser.h"
#include "MapObjectPropsPanel.h"
#include "OpenGL/Drawing.h"
#include "ThingPropsPanel.h"
#include "UI/NumberTextCtrl.h"
#include "UI/STabCtrl.h"
#include "Utility/MathStuff.h"


/*******************************************************************
 * SPRITETEXCANVAS CLASS FUNCTIONS
 *******************************************************************
 * A simple opengl canvas to display a thing sprite
 */

/* SpriteTexCanvas::SideTexCanvas
 * SpriteTexCanvas class constructor
 *******************************************************************/
SpriteTexCanvas::SpriteTexCanvas(wxWindow* parent) : OGLCanvas(parent, -1)
{
	// Init variables
	texture = nullptr;
	col = COL_WHITE;
	icon = false;
	SetWindowStyleFlag(wxBORDER_SIMPLE);

	SetInitialSize(wxSize(128, 128));
}

/* SpriteTexCanvas::~SpriteTexCanvas
 * SpriteTexCanvas class destructor
 *******************************************************************/
SpriteTexCanvas::~SpriteTexCanvas()
{
}

/* SpriteTexCanvas::getTexName
 * Returns the name of the loaded texture
 *******************************************************************/
string SpriteTexCanvas::getTexName()
{
	return texname;
}

/* SpriteTexCanvas::setTexture
 * Sets the texture to display
 *******************************************************************/
void SpriteTexCanvas::setSprite(const Game::ThingType& type)
{
	texname = type.sprite();
	icon = false;
	col = COL_WHITE;

	// Sprite
	texture = MapEditor::textureManager().getSprite(texname, type.translation(), type.palette());

	// Icon
	if (!texture)
	{
		texture = MapEditor::textureManager().getEditorImage(S_FMT("thing/%s", type.icon()));
		col = type.colour();
		icon = true;
	}

	// Unknown
	if (!texture)
	{
		texture = MapEditor::textureManager().getEditorImage("thing/unknown");
		icon = true;
	}

	Refresh();
}

/* SpriteTexCanvas::draw
 * Draws the canvas content
 *******************************************************************/
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
	OpenGL::setColour(col);
	if (texture && !icon)
	{
		// Sprite
		glEnable(GL_TEXTURE_2D);
		Drawing::drawTextureWithin(texture, 0, 0, GetSize().x, GetSize().y, 4, 2);
	}
	else if (texture && icon)
	{
		// Icon
		glEnable(GL_TEXTURE_2D);
		Drawing::drawTextureWithin(texture, 0, 0, GetSize().x, GetSize().y, 0, 0.25);
	}

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


/*******************************************************************
 * THINGDIRCANVAS CLASS FUNCTIONS
 *******************************************************************
 * An OpenGL canvas that shows a direction and circles for each of
 * the 8 'standard' directions, clicking within one of the circles
 * will set the direction
 */

/* ThingDirCanvas::ThingDirCanvas
 * ThingDirCanvas class constructor
 *******************************************************************/
ThingDirCanvas::ThingDirCanvas(wxWindow* parent) : OGLCanvas(parent, -1, true, 15)
{
	// Init variables
	angle = 0;
	point_hl = -1;
	last_check = 0;
	point_sel = -1;

	// Get system panel background colour
	wxColour bgcolwx = Drawing::getPanelBGColour();
	col_bg.set(COLWX(bgcolwx));

	// Get system text colour
	wxColour textcol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	col_fg.set(COLWX(textcol));

	// Setup dir points
	double rot = 0;
	for (int a = 0; a < 8; a++)
	{
		dir_points.push_back(fpoint2_t(sin(rot), 0 - cos(rot)));
		rot -= (3.1415926535897932384626433832795 * 2) / 8.0;
	}
	
	// Bind Events
	Bind(wxEVT_MOTION, &ThingDirCanvas::onMouseEvent, this);
	Bind(wxEVT_LEAVE_WINDOW, &ThingDirCanvas::onMouseEvent, this);
	Bind(wxEVT_LEFT_DOWN, &ThingDirCanvas::onMouseEvent, this);

	// Fixed size
	SetInitialSize(wxSize(128, 128));
	SetMaxSize(wxSize(128, 128));
}

/* ThingDirCanvas::setAngle
 * Sets the angle to display
 *******************************************************************/
void ThingDirCanvas::setAngle(int angle)
{
	// Clamp angle
	while (angle >= 360)
		angle -= 360;
	while (angle < 0)
		angle += 360;

	// Set angle
	this->angle = angle;

	// Set selected dir point (if any)
	if (!dir_points.empty())
	{
		switch (angle)
		{
		case 0:		point_sel = 6; break;
		case 45:	point_sel = 7; break;
		case 90:	point_sel = 0; break;
		case 135:	point_sel = 1; break;
		case 180:	point_sel = 2; break;
		case 225:	point_sel = 3; break;
		case 270:	point_sel = 4; break;
		case 315:	point_sel = 5; break;
		default:	point_sel = -1; break;
		}
	}

	Refresh();
}

/* ThingDirCanvas::draw
 * Draws the control
 *******************************************************************/
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
	glClearColor(col_bg.fr(), col_bg.fg(), col_bg.fb(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Draw angle ring
	glLineWidth(1.5f);
	glEnable(GL_LINE_SMOOTH);
	rgba_t col_faded(col_bg.r * 0.6 + col_fg.r * 0.4,
		col_bg.g * 0.6 + col_fg.g * 0.4,
		col_bg.b * 0.6 + col_fg.b * 0.4);
	Drawing::drawEllipse(fpoint2_t(0, 0), 1, 1, 48, col_faded);

	// Draw dir points
	for (unsigned a = 0; a < dir_points.size(); a++)
	{
		Drawing::drawFilledEllipse(dir_points[a], 0.12, 0.12, 8, col_bg);
		Drawing::drawEllipse(dir_points[a], 0.12, 0.12, 16, col_fg);
	}

	// Draw angle arrow
	glLineWidth(2.0f);
	fpoint2_t tip = MathStuff::rotatePoint(fpoint2_t(0, 0), fpoint2_t(0.8, 0), -angle);
	Drawing::drawArrow(tip, fpoint2_t(0, 0), col_fg, false, 1.2, 0.2);

	// Draw hover point
	glPointSize(8.0f);
	glEnable(GL_POINT_SMOOTH);
	if (point_hl >= 0 && point_hl < (int)dir_points.size())
	{
		OpenGL::setColour(col_faded);
		glBegin(GL_POINTS);
		glVertex2d(dir_points[point_hl].x, dir_points[point_hl].y);
		glEnd();
	}

	// Draw selected point
	if (point_sel >= 0 && point_sel < (int)dir_points.size())
	{
		OpenGL::setColour(col_fg);
		glBegin(GL_POINTS);
		glVertex2d(dir_points[point_sel].x, dir_points[point_sel].y);
		glEnd();
	}

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


/*******************************************************************
 * THINGDIRCANVAS CLASS EVENTS
 *******************************************************************/

/* ThingDirCanvas::onMouseEvent
 * Called when a mouse event happens in the canvas
 *******************************************************************/
void ThingDirCanvas::onMouseEvent(wxMouseEvent& e)
{
	// Motion
	if (e.Moving())
	{
		if (App::runTimer() > last_check + 15)
		{
			// Get cursor position in canvas coordinates
			double x = -1.2 + ((double)e.GetX() / (double)GetSize().x) * 2.4;
			double y = -1.2 + ((double)e.GetY() / (double)GetSize().y) * 2.4;
			fpoint2_t cursor_pos(x, y);

			// Find closest dir point to cursor
			point_hl = -1;
			double min_dist = 0.3;
			for (unsigned a = 0; a < dir_points.size(); a++)
			{
				double dist = MathStuff::distance(cursor_pos, dir_points[a]);
				if (dist < min_dist)
				{
					point_hl = a;
					min_dist = dist;
				}
			}

			last_check = App::runTimer();
		}
	}

	// Leaving
	else if (e.Leaving())
		point_hl = -1;

	// Left click
	else if (e.LeftDown())
	{
		if (point_hl >= 0)
		{
			point_sel = point_hl;
			switch (point_sel)
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
		}
	}

	e.Skip();
}


/*******************************************************************
 * ANGLECONTROL CLASS FUNCTIONS
 *******************************************************************/

/* AngleControl::AngleControl
 * AngleControl class constructor
 *******************************************************************/
AngleControl::AngleControl(wxWindow* parent) : wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Setup visual angle panel
	wxPanel* panel = new wxPanel(this, -1);
	sizer->Add(panel, 1, wxEXPAND|wxALL, 4);
	wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
	panel->SetSizer(gb_sizer);

	// Fixed size
	panel->SetInitialSize(wxSize(140, 140));
	panel->SetMaxSize(wxSize(140, 140));

	// Angle buttons
	gb_sizer->Add(rb_angles[0] = new wxRadioButton(panel, -1, ""), wxGBPosition(2, 4), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT);		// East
	gb_sizer->Add(rb_angles[1] = new wxRadioButton(panel, -1, ""), wxGBPosition(1, 3), wxDefaultSpan, wxALIGN_TOP|wxALIGN_RIGHT);					// NorthEast
	gb_sizer->Add(rb_angles[2] = new wxRadioButton(panel, -1, ""), wxGBPosition(0, 2), wxDefaultSpan, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM);	// North
	gb_sizer->Add(rb_angles[3] = new wxRadioButton(panel, -1, ""), wxGBPosition(1, 1), wxDefaultSpan, wxALIGN_TOP|wxALIGN_LEFT);					// NorthWest
	gb_sizer->Add(rb_angles[4] = new wxRadioButton(panel, -1, ""), wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);		// West
	gb_sizer->Add(rb_angles[5] = new wxRadioButton(panel, -1, ""), wxGBPosition(3, 1), wxDefaultSpan, wxALIGN_BOTTOM|wxALIGN_LEFT);					// SouthWest
	gb_sizer->Add(rb_angles[6] = new wxRadioButton(panel, -1, ""), wxGBPosition(4, 2), wxDefaultSpan, wxALIGN_CENTER_HORIZONTAL|wxALIGN_TOP);		// South
	gb_sizer->Add(rb_angles[7] = new wxRadioButton(panel, -1, ""), wxGBPosition(3, 3), wxDefaultSpan, wxALIGN_BOTTOM|wxALIGN_RIGHT);				// SouthEast
	for (unsigned a = 0; a < 5; a++)
	{
		gb_sizer->AddGrowableCol(a, 1);
		gb_sizer->AddGrowableRow(a, 1);
	}

	// Angle text box
	text_angle = new NumberTextCtrl(this);
	sizer->Add(text_angle, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Bind events
	for (unsigned a = 0; a < 8; a++)
		rb_angles[a]->Bind(wxEVT_RADIOBUTTON, &AngleControl::onAngleButtonClicked, this);
}

/* AngleControl::~AngleControl
 * AngleControl class destructor
 *******************************************************************/
AngleControl::~AngleControl()
{
}

/* AngleControl::getAngle
 * Returns the current angle
 *******************************************************************/
int AngleControl::getAngle(int base)
{
	return text_angle->getNumber(base);
}

/* AngleControl::setAngle
 * Sets the angle to display
 *******************************************************************/
void AngleControl::setAngle(int angle)
{
	this->angle = angle;
	text_angle->setNumber(angle);
	updateAngle();
}

/* AngleControl::updateAngle
 * Updates the visual angle buttons
 *******************************************************************/
void AngleControl::updateAngle()
{
	// Set angle button
	for (unsigned a = 0; a < 8; a++)
		rb_angles[a]->SetValue(false);

	if (angleSet())
	{
		switch (angle)
		{
		case 0:		rb_angles[0]->SetValue(true); break;
		case 45:	rb_angles[1]->SetValue(true); break;
		case 90:	rb_angles[2]->SetValue(true); break;
		case 135:	rb_angles[3]->SetValue(true); break;
		case 180:	rb_angles[4]->SetValue(true); break;
		case 225:	rb_angles[5]->SetValue(true); break;
		case 270:	rb_angles[6]->SetValue(true); break;
		case 315:	rb_angles[7]->SetValue(true); break;
		default:	break;
		}
	}
}

/* AngleControl::angleSet
 * Returns true if an angle is specified
 *******************************************************************/
bool AngleControl::angleSet()
{
	return !(text_angle->GetValue().IsEmpty());
}


/*******************************************************************
 * ANGLECONTROL CLASS EVENTS
 *******************************************************************/

/* AngleControl::onAngleButtonClicked
 * Called when an angle radio button is clicked
 *******************************************************************/
void AngleControl::onAngleButtonClicked(wxCommandEvent& e)
{
	// Set angle text
	if (e.GetEventObject() == rb_angles[0])
		text_angle->setNumber(0);
	else if (e.GetEventObject() == rb_angles[1])
		text_angle->setNumber(45);
	else if (e.GetEventObject() == rb_angles[2])
		text_angle->setNumber(90);
	else if (e.GetEventObject() == rb_angles[3])
		text_angle->setNumber(135);
	else if (e.GetEventObject() == rb_angles[4])
		text_angle->setNumber(180);
	else if (e.GetEventObject() == rb_angles[5])
		text_angle->setNumber(225);
	else if (e.GetEventObject() == rb_angles[6])
		text_angle->setNumber(270);
	else if (e.GetEventObject() == rb_angles[7])
		text_angle->setNumber(315);
}

/* AngleControl::onAngleTextChanged
 * Called when the angle text box is changed
 *******************************************************************/
void AngleControl::onAngleTextChanged(wxCommandEvent& e)
{
	this->angle = text_angle->getNumber();
	updateAngle();
}


/*******************************************************************
 * THINGPROPSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ThingPropsPanel::ThingPropsPanel
 * ThingPropsPanel class constructor
 *******************************************************************/
ThingPropsPanel::ThingPropsPanel(wxWindow* parent) : PropsPanelBase(parent)
{
	panel_special = nullptr;
	panel_args = nullptr;

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Tabs
	stc_tabs = STabCtrl::createControl(this);
	sizer->Add(stc_tabs, 1, wxEXPAND|wxALL, 4);

	// General tab
	stc_tabs->AddPage(setupGeneralTab(), "General");

	// Extra Flags tab
	if (!udmf_flags_extra.empty())
		stc_tabs->AddPage(setupExtraFlagsTab(), "Extra Flags");

	// Special tab
	if (MapEditor::editContext().mapDesc().format != MAP_DOOM)
		stc_tabs->AddPage(panel_special = new ActionSpecialPanel(stc_tabs, false), "Special");

	// Args tab
	if (MapEditor::editContext().mapDesc().format != MAP_DOOM)
	{
		stc_tabs->AddPage(panel_args = new ArgsPanel(stc_tabs), "Args");
		if (panel_special)
			panel_special->setArgsPanel(panel_args);
	}

	// Other Properties tab
	stc_tabs->AddPage(mopp_other_props = new MapObjectPropsPanel(stc_tabs, true), "Other Properties");
	mopp_other_props->hideFlags(true);
	mopp_other_props->hideProperty("height");
	mopp_other_props->hideProperty("angle");
	mopp_other_props->hideProperty("type");
	mopp_other_props->hideProperty("id");
	mopp_other_props->hideProperty("special");
	mopp_other_props->hideProperty("arg0");
	mopp_other_props->hideProperty("arg1");
	mopp_other_props->hideProperty("arg2");
	mopp_other_props->hideProperty("arg3");
	mopp_other_props->hideProperty("arg4");

	// Bind events
	gfx_sprite->Bind(wxEVT_LEFT_DOWN, &ThingPropsPanel::onSpriteClicked, this);

	Layout();
}

/* ThingPropsPanel::~ThingPropsPanel
 * ThingPropsPanel class destructor
 *******************************************************************/
ThingPropsPanel::~ThingPropsPanel()
{
}

/* ThingPropsPanel::setupGeneralTab
 * Creates and sets up the 'General' properties tab
 *******************************************************************/
wxPanel* ThingPropsPanel::setupGeneralTab()
{
	int map_format = MapEditor::editContext().mapDesc().format;

	// Create panel
	wxPanel* panel = new wxPanel(stc_tabs, -1);

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// --- Flags ---
	wxStaticBox* frame = new wxStaticBox(panel, -1, "Flags");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	// Init flags
	wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
	framesizer->Add(gb_sizer, 1, wxEXPAND|wxALL, 4);
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
					udmf_flags.push_back(i.second.propName());
				}
				else
					udmf_flags_extra.push_back(i.second.propName());
			}
		}

		// Add flag checkboxes
		int flag_mid = flags.size() / 3;
		if (flags.size() % 3 == 0) flag_mid--;
		for (unsigned a = 0; a < flags.size(); a++)
		{
			wxCheckBox* cb_flag = new wxCheckBox(panel, -1, flags[a], wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
			gb_sizer->Add(cb_flag, wxGBPosition(row++, col), wxDefaultSpan, wxEXPAND);
			cb_flags.push_back(cb_flag);

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
			wxCheckBox* cb_flag = new wxCheckBox(panel, -1, Game::configuration().thingFlag(a), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
			gb_sizer->Add(cb_flag, wxGBPosition(row++, col), wxDefaultSpan, wxEXPAND);
			cb_flags.push_back(cb_flag);

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
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	frame = new wxStaticBox(panel, -1, "Type");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 1, wxEXPAND|wxRIGHT, 4);
	framesizer->Add(gfx_sprite = new SpriteTexCanvas(panel), 1, wxEXPAND|wxALL, 4);
	framesizer->Add(label_type = new wxStaticText(panel, -1, ""), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Direction
	frame = new wxStaticBox(panel, -1, "Direction");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 0, wxEXPAND|wxRIGHT, 4);
	framesizer->Add(ac_direction = new AngleControl(panel), 1, wxEXPAND);
	
#ifdef __WXMSW__
	//ac_direction->SetBackgroundColour(stc_tabs->GetThemeBackgroundColour());
#endif

	if (map_format != MAP_DOOM)
	{
		// Id
		gb_sizer = new wxGridBagSizer(4, 4);
		sizer->Add(gb_sizer, 0, wxEXPAND|wxALL, 4);
		gb_sizer->Add(new wxStaticText(panel, -1, "TID:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		gb_sizer->Add(text_id = new NumberTextCtrl(panel), wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

		// Z Height
		gb_sizer->Add(new wxStaticText(panel, -1, "Z Height:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		gb_sizer->Add(text_height = new NumberTextCtrl(panel), wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
		if (map_format == MAP_UDMF)
			text_height->allowDecimal(true);

		gb_sizer->AddGrowableCol(1, 1);
	}

	return panel;
}

/* ThingPropsPanel::setupExtraFlagsTab
 * Creates and sets up the 'Extra Flags' tab
 *******************************************************************/
wxPanel* ThingPropsPanel::setupExtraFlagsTab()
{
	// Create panel
	wxPanel* panel = new wxPanel(stc_tabs, -1);

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Init flags
	wxGridBagSizer* gb_sizer_flags = new wxGridBagSizer(4, 4);
	sizer->Add(gb_sizer_flags, 1, wxEXPAND|wxALL, 10);
	int row = 0;
	int col = 0;

	// Get all extra flag names
	vector<string> flags;
	for (unsigned a = 0; a < udmf_flags_extra.size(); a++)
	{
		UDMFProperty* prop = Game::configuration().getUDMFProperty(udmf_flags_extra[a], MOBJ_THING);
		flags.push_back(prop->name());
	}

	// Add flag checkboxes
	int flag_mid = flags.size() / 3;
	if (flags.size() % 3 == 0) flag_mid--;
	for (unsigned a = 0; a < flags.size(); a++)
	{
		wxCheckBox* cb_flag = new wxCheckBox(panel, -1, flags[a], wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
		gb_sizer_flags->Add(cb_flag, wxGBPosition(row++, col), wxDefaultSpan, wxEXPAND);
		cb_flags_extra.push_back(cb_flag);

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

/* ThingPropsPanel::openObjects
 * Loads values from things in [objects]
 *******************************************************************/
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
		for (unsigned a = 0; a < udmf_flags.size(); a++)
		{
			if (MapObject::multiBoolProperty(objects, udmf_flags[a], val))
				cb_flags[a]->SetValue(val);
			else
				cb_flags[a]->Set3StateValue(wxCHK_UNDETERMINED);
		}
	}
	else
	{
		for (int a = 0; a < Game::configuration().nThingFlags(); a++)
		{
			// Set initial flag checked value
			cb_flags[a]->SetValue(Game::configuration().thingFlagSet(a, (MapThing*)objects[0]));

			// Go through subsequent things
			for (unsigned b = 1; b < objects.size(); b++)
			{
				// Check for mismatch			
				if (cb_flags[a]->GetValue() != Game::configuration().thingFlagSet(a, (MapThing*)objects[b]))
				{
					// Set undefined
					cb_flags[a]->Set3StateValue(wxCHK_UNDETERMINED);
					break;
				}
			}
		}
	}

	// Type
	type_current = 0;
	if (MapObject::multiIntProperty(objects, "type", type_current))
	{
		auto& tt = Game::configuration().thingType(type_current);
		gfx_sprite->setSprite(tt);
		label_type->SetLabel(S_FMT("%d: %s", type_current, tt.name()));
		label_type->Wrap(136);
	}

	// Special
	ival = -1;
	if (panel_special)
	{
		if (MapObject::multiIntProperty(objects, "special", ival))
			panel_special->setSpecial(ival);
	}

	// Args
	if (map_format == MAP_HEXEN || map_format == MAP_UDMF)
	{
		// Setup
		if (ival > 0)
		{
			auto& as = Game::configuration().actionSpecial(ival).argSpec();
			panel_args->setup(as, (map_format == MAP_UDMF));
		}
		else
		{
			auto& as = Game::configuration().thingType(type_current).argSpec();
			panel_args->setup(as, (map_format == MAP_UDMF));
		}

		// Load values
		int args[5] = { -1, -1, -1, -1, -1 };
		for (unsigned a = 0; a < 5; a++)
			MapObject::multiIntProperty(objects, S_FMT("arg%d", a), args[a]);
		panel_args->setValues(args);
	}

	// Direction
	if (MapObject::multiIntProperty(objects, "angle", ival))
		ac_direction->setAngle(ival);

	// Id
	if (map_format != MAP_DOOM && MapObject::multiIntProperty(objects, "id", ival))
		text_id->setNumber(ival);

	// Z Height
	if (map_format == MAP_HEXEN && MapObject::multiIntProperty(objects, "height", ival))
		text_height->setNumber(ival);
	else if (map_format == MAP_UDMF && MapObject::multiFloatProperty(objects, "height", fval))
		text_height->setDecNumber(fval);

	// Load other properties
	mopp_other_props->openObjects(objects);

	// Update internal objects list
	this->objects.clear();
	for (unsigned a = 0; a < objects.size(); a++)
		this->objects.push_back(objects[a]);

	// Update layout
	Layout();
	Refresh();
}

/* ThingPropsPanel::applyChanges
 * Applies values to currently open things
 *******************************************************************/
void ThingPropsPanel::applyChanges()
{
	int map_format = MapEditor::editContext().mapDesc().format;

	// Apply general properties
	for (unsigned a = 0; a < objects.size(); a++)
	{
		// Flags
		if (udmf_flags.empty())
		{
			for (int f = 0; f < Game::configuration().nThingFlags(); f++)
			{
				if (cb_flags[f]->Get3StateValue() != wxCHK_UNDETERMINED)
					Game::configuration().setThingFlag(f, (MapThing*)objects[a], cb_flags[f]->GetValue());
			}
		}

		// UDMF flags
		else
		{
			for (unsigned f = 0; f < udmf_flags.size(); f++)
			{
				if (cb_flags[f]->Get3StateValue() != wxCHK_UNDETERMINED)
					objects[a]->setBoolProperty(udmf_flags[f], cb_flags[f]->GetValue());
			}
		}

		// UDMF extra flags
		if (!udmf_flags_extra.empty())
		{
			for (unsigned f = 0; f < udmf_flags_extra.size(); f++)
			{
				if (cb_flags_extra[f]->Get3StateValue() != wxCHK_UNDETERMINED)
					objects[a]->setBoolProperty(udmf_flags_extra[f], cb_flags_extra[f]->GetValue());
			}
		}

		// Type
		if (type_current > 0)
			objects[a]->setIntProperty("type", type_current);

		// Direction
		if (ac_direction->angleSet())
			objects[a]->setIntProperty("angle", ac_direction->getAngle(objects[a]->intProperty("angle")));

		if (map_format != MAP_DOOM)
		{
			// ID
			if (!text_id->GetValue().IsEmpty())
				objects[a]->setIntProperty("id", text_id->getNumber(objects[a]->intProperty("id")));

			// Z Height
			if (!text_height->GetValue().IsEmpty())
			{
				if (map_format == MAP_UDMF)
					objects[a]->setFloatProperty("height", text_height->getDecNumber(objects[a]->floatProperty("height")));
				else
					objects[a]->setIntProperty("height", text_height->getNumber(objects[a]->intProperty("height")));
			}
		}
	}

	// Special
	if (panel_special)
		panel_special->applyTo(objects, true);

	mopp_other_props->applyChanges();
}


/*******************************************************************
 * THINGPROPSPANEL CLASS EVENTS
 *******************************************************************/

/* ThingPropsPanel::onSpriteClicked
 * Called when the thing type sprite canvas is clicked
 *******************************************************************/
void ThingPropsPanel::onSpriteClicked(wxMouseEvent& e)
{
	ThingTypeBrowser browser(this, type_current);
	if (browser.ShowModal() == wxID_OK)
	{
		// Get selected type
		type_current = browser.getSelectedType();
		auto& tt = Game::configuration().thingType(type_current);

		// Update sprite
		gfx_sprite->setSprite(tt);
		label_type->SetLabel(tt.name());

		// Update args
		if (panel_args)
		{
			auto& as = tt.argSpec();
			panel_args->setup(as, (MapEditor::editContext().mapDesc().format == MAP_UDMF));
		}

		// Update layout
		Layout();
		Refresh();
	}
}
