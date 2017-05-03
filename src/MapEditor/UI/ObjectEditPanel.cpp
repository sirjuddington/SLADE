
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ObjectEditPanel.cpp
 * Description: A bar that shows up during object edit mode that
 *              contains controls to manually enter the
 *              scaling/offset/rotation values
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
#include "General/KeyBind.h"
#include "Graphics/Icons.h"
#include "MapEditor/Edit/ObjectEdit.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "ObjectEditPanel.h"


/*******************************************************************
 * OBJECTEDITPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ObjectEditPanel::ObjectEditPanel
 * ObjectEditPanel class constructor
 *******************************************************************/
ObjectEditPanel::ObjectEditPanel(wxWindow* parent) : wxPanel(parent)
{
	// Setup sizer
	wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(msizer);
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	msizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	wxIntegerValidator<int> val_int(NULL, wxNUM_VAL_DEFAULT);
	wxIntegerValidator<unsigned int> val_uint(NULL, wxNUM_VAL_DEFAULT);
	wxFloatingPointValidator<double> val_double(2, NULL, wxNUM_VAL_DEFAULT);

	// X offset
	text_xoff = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(64, -1), 0, val_int);
	sizer->Add(new wxStaticText(this, -1, "X Offset:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	sizer->Add(text_xoff, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8);

	// Y offset
	text_yoff = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(64, -1), 0, val_int);
	sizer->Add(new wxStaticText(this, -1, "Y Offset:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	sizer->Add(text_yoff, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8);

	// X scale
	text_scalex = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(64, -1), 0, val_uint);
	sizer->Add(new wxStaticText(this, -1, "X Scale:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	sizer->Add(text_scalex, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT);
	sizer->Add(new wxStaticText(this, -1, "%"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8);

	// Y scale
	text_scaley = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(64, -1), 0, val_uint);
	sizer->Add(new wxStaticText(this, -1, "Y Scale:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	sizer->Add(text_scaley, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT);
	sizer->Add(new wxStaticText(this, -1, "%"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8);

	// Rotation
	string angles[] = { "0", "45", "90", "135", "180", "225", "270", "315" };
	combo_rotation = new wxComboBox(this, -1, "", wxDefaultPosition, wxSize(64, -1), 8, angles, 0, wxDefaultValidator);
	combo_rotation->SetValidator(val_double);
	sizer->Add(new wxStaticText(this, -1, "Rotation:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	sizer->Add(combo_rotation, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8);

	// Mirror X
	cb_mirror_x = new wxCheckBox(this, -1, "Mirror X");
	sizer->Add(cb_mirror_x, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8);

	// Mirror Y
	cb_mirror_y = new wxCheckBox(this, -1, "Mirror Y");
	sizer->Add(cb_mirror_y, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8);

	// Preview button
	btn_preview = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "eye"));
	btn_preview->SetToolTip("Preview");
	btn_preview->SetDefault();
	sizer->Add(btn_preview, 0, wxEXPAND|wxRIGHT, 4);

	// Cancel button
	btn_cancel = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "close"));
	btn_cancel->SetToolTip("Cancel");
	sizer->Add(btn_cancel, 0, wxEXPAND|wxRIGHT, 4);

	// Apply button
	btn_apply = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "tick"));
	sizer->Add(btn_apply, 0, wxEXPAND);
	btn_apply->SetToolTip("Apply");

	// Bind events
	btn_preview->Bind(wxEVT_BUTTON, &ObjectEditPanel::onBtnPreviewClicked, this);
	btn_cancel->Bind(wxEVT_BUTTON, &ObjectEditPanel::onBtnCancelClicked, this);
	btn_apply->Bind(wxEVT_BUTTON, &ObjectEditPanel::onBtnApplyClicked, this);
	cb_mirror_x->Bind(wxEVT_CHECKBOX, &ObjectEditPanel::onBtnPreviewClicked, this);
	cb_mirror_y->Bind(wxEVT_CHECKBOX, &ObjectEditPanel::onBtnPreviewClicked, this);

	// Init layout
	Layout();
}

/* ObjectEditPanel::~ObjectEditPanel
 * ObjectEditPanel class destructor
 *******************************************************************/
ObjectEditPanel::~ObjectEditPanel()
{
}

/* ObjectEditPanel::init
 * Initialises the panel with values from [group]
 *******************************************************************/
void ObjectEditPanel::init(ObjectEditGroup* group)
{
	// Check group was given
	if (!group)
		return;

	// Set initial values from group
	bbox_t bbox = group->getBBox();
	old_x = bbox.mid_x();
	old_y = bbox.mid_y();
	old_width = bbox.width();
	old_height = bbox.height();

	// Init UI values
	text_xoff->SetValue(S_FMT("%d", 0));
	text_yoff->SetValue(S_FMT("%d", 0));
	text_scalex->SetValue(S_FMT("%d", 100));
	text_scaley->SetValue(S_FMT("%d", 100));
	combo_rotation->Select(0);
}

/* ObjectEditPanel::update
 * Updates the panel with values from [group]
 *******************************************************************/
void ObjectEditPanel::update(ObjectEditGroup* group, bool lock_rotation)
{
	bbox_t bbox = group->getBBox();
	int xoff = bbox.mid_x() - old_x;
	int yoff = bbox.mid_y() - old_y;
	double xscale = bbox.width() / old_width;
	double yscale = bbox.height() / old_height;

	text_xoff->SetValue(S_FMT("%d", xoff));
	text_yoff->SetValue(S_FMT("%d", yoff));
	text_scalex->SetValue(S_FMT("%d", int(100 * xscale)));
	text_scaley->SetValue(S_FMT("%d", int(100 * yscale)));
	combo_rotation->SetValue(S_FMT("%1.2f", group->getRotation()));
}


/*******************************************************************
 * OBJECTEDITPANEL CLASS EVENTS
 *******************************************************************/

/* ObjectEditPanel::onBtnApplyClicked
 * Called when the 'apply' button is clicked
 *******************************************************************/
void ObjectEditPanel::onBtnApplyClicked(wxCommandEvent& e)
{
	KeyBind::pressBind("map_edit_accept");
}

/* ObjectEditPanel::onBtnPreviewClicked
 * Called when the 'preview' button is clicked
 *******************************************************************/
void ObjectEditPanel::onBtnPreviewClicked(wxCommandEvent& e)
{
	double xoff, yoff, xscale, yscale, rotation;
	text_xoff->GetValue().ToDouble(&xoff);
	text_yoff->GetValue().ToDouble(&yoff);
	text_scalex->GetValue().ToDouble(&xscale);
	text_scaley->GetValue().ToDouble(&yscale);
	combo_rotation->GetValue().ToDouble(&rotation);
	bool mirror_x = cb_mirror_x->GetValue();
	bool mirror_y = cb_mirror_y->GetValue();

	MapEditor::editContext().objectEdit().group().doAll(
		xoff,
		yoff,
		xscale / 100.0,
		yscale / 100.0,
		rotation,
		mirror_x,
		mirror_y
	);
}

/* ObjectEditPanel::onBtnCancelClicked
 * Called when the 'cancel' button is clicked
 *******************************************************************/
void ObjectEditPanel::onBtnCancelClicked(wxCommandEvent& e)
{
	KeyBind::pressBind("map_edit_cancel");
}
