
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ObjectEditPanel.h"
#include "General/KeyBind.h"
#include "MapEditor/Edit/ObjectEdit.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// ObjectEditPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ObjectEditPanel class constructor
// -----------------------------------------------------------------------------
ObjectEditPanel::ObjectEditPanel(wxWindow* parent) : wxPanel(parent)
{
	wxIntegerValidator<int>          val_int(nullptr, wxNUM_VAL_DEFAULT);
	wxIntegerValidator<unsigned int> val_uint(nullptr, wxNUM_VAL_DEFAULT);
	wxFloatingPointValidator<double> val_double(2, nullptr, wxNUM_VAL_DEFAULT);
	auto                             tb_size = FromDIP(wxSize(64, -1));

	// Create controls
	text_xoff_      = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, tb_size, 0, val_int);
	text_yoff_      = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, tb_size, 0, val_int);
	text_scalex_    = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, tb_size, 0, val_uint);
	text_scaley_    = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, tb_size, 0, val_uint);
	combo_rotation_ = new wxComboBox(this, -1, wxEmptyString, wxDefaultPosition, tb_size);
	cb_mirror_x_    = new wxCheckBox(this, -1, wxS("Mirror X"));
	cb_mirror_y_    = new wxCheckBox(this, -1, wxS("Mirror Y"));
	btn_preview_    = new SIconButton(this, "eye", "Preview");
	btn_cancel_     = new SIconButton(this, "close", "Cancel");
	btn_apply_      = new SIconButton(this, "tick", "Apply");

	// Init controls
	combo_rotation_->Set(wxutil::arrayStringStd({ "0", "45", "90", "135", "180", "225", "270", "315" }));
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
	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Initialises the panel with values from [group]
// -----------------------------------------------------------------------------
void ObjectEditPanel::init(const ObjectEditGroup* group)
{
	// Check group was given
	if (!group)
		return;

	// Set initial values from group
	auto bbox   = group->bbox();
	old_x_      = bbox.midX();
	old_y_      = bbox.midY();
	old_width_  = bbox.width();
	old_height_ = bbox.height();

	// Init UI values
	text_xoff_->SetValue(WX_FMT("{}", 0));
	text_yoff_->SetValue(WX_FMT("{}", 0));
	text_scalex_->SetValue(WX_FMT("{}", 100));
	text_scaley_->SetValue(WX_FMT("{}", 100));
	combo_rotation_->Select(0);
	cb_mirror_x_->SetValue(false);
	cb_mirror_y_->SetValue(false);
}

// -----------------------------------------------------------------------------
// Updates the panel with values from [group]
// -----------------------------------------------------------------------------
void ObjectEditPanel::update(const ObjectEditGroup* group, bool lock_rotation) const
{
	auto   bbox   = group->bbox();
	int    xoff   = bbox.midX() - old_x_;
	int    yoff   = bbox.midY() - old_y_;
	double xscale = bbox.width() / old_width_;
	double yscale = bbox.height() / old_height_;

	text_xoff_->SetValue(WX_FMT("{}", xoff));
	text_yoff_->SetValue(WX_FMT("{}", yoff));
	text_scalex_->SetValue(WX_FMT("{}", static_cast<int>(100 * xscale)));
	text_scaley_->SetValue(WX_FMT("{}", static_cast<int>(100 * yscale)));
	combo_rotation_->SetValue(WX_FMT("{:1.2f}", group->rotation()));
}

// -----------------------------------------------------------------------------
// Lays out the controls on the panel
// -----------------------------------------------------------------------------
void ObjectEditPanel::setupLayout()
{
	auto lh = ui::LayoutHelper(this);

	// Init sizer
	SetSizer(new wxBoxSizer(wxVERTICAL));
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	GetSizer()->Add(sizer, lh.sfWithBorder(1).Expand());

	// X offset
	sizer->Add(
		wxutil::createLabelHBox(this, "X Offset:", text_xoff_), lh.sfWithLargeBorder(0, wxRIGHT).CenterVertical());

	// Y offset
	sizer->Add(
		wxutil::createLabelHBox(this, "Y Offset:", text_yoff_), lh.sfWithLargeBorder(0, wxRIGHT).CenterVertical());

	// X scale
	sizer->Add(
		wxutil::createLabelHBox(this, "X Scale:", text_scalex_), lh.sfWithSmallBorder(0, wxRIGHT).CenterVertical());
	sizer->Add(new wxStaticText(this, -1, wxS("%")), lh.sfWithLargeBorder(0, wxRIGHT).CenterVertical());

	// Y scale
	sizer->Add(
		wxutil::createLabelHBox(this, "Y Scale:", text_scaley_), lh.sfWithSmallBorder(0, wxRIGHT).CenterVertical());
	sizer->Add(new wxStaticText(this, -1, wxS("%")), lh.sfWithLargeBorder(0, wxRIGHT).CenterVertical());

	// Rotation
	sizer->Add(
		wxutil::createLabelHBox(this, "Rotation:", combo_rotation_), lh.sfWithLargeBorder(0, wxRIGHT).CenterVertical());

	// Mirror x/y
	sizer->Add(cb_mirror_x_, lh.sfWithLargeBorder(0, wxRIGHT).CenterVertical());
	sizer->Add(cb_mirror_y_, lh.sfWithLargeBorder(0, wxRIGHT).CenterVertical());

	// Buttons
	sizer->Add(btn_preview_, lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	sizer->Add(btn_cancel_, lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	sizer->Add(btn_apply_, wxSizerFlags().CenterVertical());
}


// -----------------------------------------------------------------------------
//
// ObjectEditPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'preview' button is clicked
// -----------------------------------------------------------------------------
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

	mapeditor::editContext().objectEdit().group().doAll(
		xoff, yoff, xscale / 100.0, yscale / 100.0, rotation, mirror_x, mirror_y);
}
