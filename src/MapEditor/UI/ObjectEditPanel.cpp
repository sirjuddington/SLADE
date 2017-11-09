
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ObjectEditPanel.cpp
// Description: A bar that shows up during object edit mode that contains
//              controls to manually enter the scaling/offset/rotation values
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
#include "General/KeyBind.h"
#include "Graphics/Icons.h"
#include "MapEditor/Edit/ObjectEdit.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "ObjectEditPanel.h"
#include "UI/WxUtils.h"
#include "UI/Controls/SIconButton.h"


// ----------------------------------------------------------------------------
//
// ObjectEditPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ObjectEditPanel::ObjectEditPanel
//
// ObjectEditPanel class constructor
// ----------------------------------------------------------------------------
ObjectEditPanel::ObjectEditPanel(wxWindow* parent) : wxPanel(parent)
{
	wxIntegerValidator<int> val_int(nullptr, wxNUM_VAL_DEFAULT);
	wxIntegerValidator<unsigned int> val_uint(nullptr, wxNUM_VAL_DEFAULT);
	wxFloatingPointValidator<double> val_double(2, nullptr, wxNUM_VAL_DEFAULT);
	auto tb_size = WxUtils::scaledSize(64, -1);

	// Create controls
	text_xoff_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, tb_size, 0, val_int);
	text_yoff_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, tb_size, 0, val_int);
	text_scalex_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, tb_size, 0, val_uint);
	text_scaley_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, tb_size, 0, val_uint);
	combo_rotation_ = new wxComboBox(this, -1, "", wxDefaultPosition, tb_size);
	cb_mirror_x_ = new wxCheckBox(this, -1, "Mirror X");
	cb_mirror_y_ = new wxCheckBox(this, -1, "Mirror Y");
	btn_preview_ = new SIconButton(this, "eye", "Preview");
	btn_cancel_ = new SIconButton(this, "close", "Cancel");
	btn_apply_ = new SIconButton(this, "tick", "Apply");

	// Init controls
	combo_rotation_->Set(vector<string>{ "0", "45", "90", "135", "180", "225", "270", "315" });
	combo_rotation_->SetValidator(val_double);
	btn_preview_->SetDefault();

	// Layout
	setupLayout();

	// Bind events
	btn_preview_->Bind(wxEVT_BUTTON, &ObjectEditPanel::onBtnPreviewClicked, this);
	btn_cancel_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { KeyBind::pressBind("map_edit_cancel"); });
	btn_apply_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { KeyBind::pressBind("map_edit_accept"); });
	cb_mirror_x_->Bind(wxEVT_CHECKBOX, &ObjectEditPanel::onBtnPreviewClicked, this);
	cb_mirror_y_->Bind(wxEVT_CHECKBOX, &ObjectEditPanel::onBtnPreviewClicked, this);

	// Init layout
	Layout();
}

// ----------------------------------------------------------------------------
// ObjectEditPanel::init
//
// Initialises the panel with values from [group]
// ----------------------------------------------------------------------------
void ObjectEditPanel::init(ObjectEditGroup* group)
{
	// Check group was given
	if (!group)
		return;

	// Set initial values from group
	bbox_t bbox = group->getBBox();
	old_x_ = bbox.mid_x();
	old_y_ = bbox.mid_y();
	old_width_ = bbox.width();
	old_height_ = bbox.height();

	// Init UI values
	text_xoff_->SetValue(S_FMT("%d", 0));
	text_yoff_->SetValue(S_FMT("%d", 0));
	text_scalex_->SetValue(S_FMT("%d", 100));
	text_scaley_->SetValue(S_FMT("%d", 100));
	combo_rotation_->Select(0);
}

// ----------------------------------------------------------------------------
// ObjectEditPanel::update
//
// Updates the panel with values from [group]
// ----------------------------------------------------------------------------
void ObjectEditPanel::update(ObjectEditGroup* group, bool lock_rotation) const
{
	bbox_t bbox = group->getBBox();
	int xoff = bbox.mid_x() - old_x_;
	int yoff = bbox.mid_y() - old_y_;
	double xscale = bbox.width() / old_width_;
	double yscale = bbox.height() / old_height_;

	text_xoff_->SetValue(S_FMT("%d", xoff));
	text_yoff_->SetValue(S_FMT("%d", yoff));
	text_scalex_->SetValue(S_FMT("%d", int(100 * xscale)));
	text_scaley_->SetValue(S_FMT("%d", int(100 * yscale)));
	combo_rotation_->SetValue(S_FMT("%1.2f", group->getRotation()));
}

// ----------------------------------------------------------------------------
// ObjectEditPanel::setupLayout
//
// Lays out the controls on the panel
// ----------------------------------------------------------------------------
void ObjectEditPanel::setupLayout()
{
	// Init sizer
	SetSizer(new wxBoxSizer(wxVERTICAL));
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	GetSizer()->Add(sizer, 1, wxEXPAND | wxALL, UI::pad());

	// X offset
	sizer->Add(
		WxUtils::createLabelHBox(this, "X Offset:", text_xoff_),
		0,
		wxALIGN_CENTER_VERTICAL | wxRIGHT,
		UI::padLarge()
	);

	// Y offset
	sizer->Add(
		WxUtils::createLabelHBox(this, "Y Offset:", text_yoff_),
		0,
		wxALIGN_CENTER_VERTICAL | wxRIGHT,
		UI::padLarge()
	);

	// X scale
	sizer->Add(
		WxUtils::createLabelHBox(this, "X Scale:", text_scalex_),
		0,
		wxALIGN_CENTER_VERTICAL | wxRIGHT,
		UI::px(UI::Size::PadMinimum)
	);
	sizer->Add(new wxStaticText(this, -1, "%"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::padLarge());

	// Y scale
	sizer->Add(
		WxUtils::createLabelHBox(this, "Y Scale:", text_scaley_),
		0,
		wxALIGN_CENTER_VERTICAL | wxRIGHT,
		UI::px(UI::Size::PadMinimum)
	);
	sizer->Add(new wxStaticText(this, -1, "%"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::padLarge());

	// Rotation
	sizer->Add(
		WxUtils::createLabelHBox(this, "Rotation:", combo_rotation_),
		0,
		wxALIGN_CENTER_VERTICAL | wxRIGHT,
		UI::padLarge()
	);

	// Mirror x/y
	sizer->Add(cb_mirror_x_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::padLarge());
	sizer->Add(cb_mirror_y_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::padLarge());

	// Buttons
	sizer->Add(btn_preview_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	sizer->Add(btn_cancel_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	sizer->Add(btn_apply_, 0, wxALIGN_CENTER_VERTICAL);
}


// ----------------------------------------------------------------------------
//
// ObjectEditPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ObjectEditPanel::onBtnPreviewClicked
//
// Called when the 'preview' button is clicked
// ----------------------------------------------------------------------------
void ObjectEditPanel::onBtnPreviewClicked(wxCommandEvent& e)
{
	double xoff, yoff, xscale, yscale, rotation;
	text_xoff_->GetValue().ToDouble(&xoff);
	text_yoff_->GetValue().ToDouble(&yoff);
	text_scalex_->GetValue().ToDouble(&xscale);
	text_scaley_->GetValue().ToDouble(&yscale);
	combo_rotation_->GetValue().ToDouble(&rotation);
	bool mirror_x = cb_mirror_x_->GetValue();
	bool mirror_y = cb_mirror_y_->GetValue();

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
