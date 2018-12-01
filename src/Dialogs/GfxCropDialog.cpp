
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GfxCropDialog.cpp
// Description: A dialog UI for cropping a graphic
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
#include "GfxCropDialog.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "Graphics/SImage/SImage.h"
#include "OpenGL/Drawing.h"
#include "UI/Controls/NumberTextCtrl.h"


// -----------------------------------------------------------------------------
//
// CropCanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CropCanvas class constructor
// -----------------------------------------------------------------------------
CropCanvas::CropCanvas(wxWindow* parent, SImage* image, Palette* palette) : OGLCanvas(parent, -1, false)
{
	if (image && image->isValid())
	{
		texture_ = std::make_unique<GLTexture>();
		texture_->loadImage(image, palette);
		crop_rect_.set(0, 0, image->width(), image->height());
	}

	int size = UI::scalePx(220);
	SetInitialSize(wxSize(size, size));
}

// -----------------------------------------------------------------------------
// Draw the canvas contents
// -----------------------------------------------------------------------------
void CropCanvas::draw()
{
	// Setup for 2d
	setup2D();

	// Draw background
	drawCheckeredBackground();

	// Determine graphic position & scale
	double width  = GetSize().x;
	double height = GetSize().y;

	// Get image dimensions
	double x_dim = (double)texture_->width();
	double y_dim = (double)texture_->height();

	// Get max scale for x and y (including padding)
	double x_scale = ((double)width - UI::scalePx(24)) / x_dim;
	double y_scale = ((double)height - UI::scalePx(24)) / y_dim;

	// Set scale to smallest of the 2 (so that none of the texture will be clipped)
	double scale = MIN(x_scale, y_scale);

	glPushMatrix();
	glTranslated(width * 0.5, height * 0.5, 0); // Translate to middle of area
	glScaled(scale, scale, scale);              // Scale to fit within area

	// Draw graphic
	double hw = 0;
	double hh = 0;
	if (texture_)
	{
		glEnable(GL_TEXTURE_2D);
		hw = texture_->width() * -0.5;
		hh = texture_->height() * -0.5;
		texture_->draw2d(hw, hh);
	}

	// Draw cropping rectangle
	OpenGL::setColour(COL_BLACK);
	glDisable(GL_TEXTURE_2D);
	glTranslated(hw, hh, 0);                                          // Translate to top-left of graphic
	Drawing::drawLine(crop_rect_.tl.x, -1000, crop_rect_.tl.x, 1000); // Left
	Drawing::drawLine(-1000, crop_rect_.tl.y, 1000, crop_rect_.tl.y); // Top
	Drawing::drawLine(crop_rect_.br.x, -1000, crop_rect_.br.x, 1000); // Right
	Drawing::drawLine(-1000, crop_rect_.br.y, 1000, crop_rect_.br.y); // Bottom

	// Shade cropped-out area
	OpenGL::setColour(ColRGBA(0, 0, 0, 100, BLEND_NORMAL));
	Drawing::drawFilledRect(-1000, -1000, crop_rect_.tl.x, 1000);                      // Left
	Drawing::drawFilledRect(crop_rect_.br.x, -1000, 1000, 1000);                       // Right
	Drawing::drawFilledRect(crop_rect_.tl.x, -1000, crop_rect_.br.x, crop_rect_.tl.y); // Top
	Drawing::drawFilledRect(crop_rect_.tl.x, crop_rect_.br.y, crop_rect_.br.x, 1000);  // Bottom

	glPopMatrix();

	SwapBuffers();
}


// -----------------------------------------------------------------------------
//
// GfxCropDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GfxCropDialog class constructor
// -----------------------------------------------------------------------------
GfxCropDialog::GfxCropDialog(wxWindow* parent, SImage* image, Palette* palette) :
	wxDialog(parent, -1, "Crop", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// Set max crop size
	if (image)
	{
		max_width_  = image->width();
		max_height_ = image->height();
	}
	else
		max_width_ = max_height_ = 0;
	crop_rect_.set(0, 0, max_width_, max_height_);

	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::General, "settings"));
	SetIcon(icon);

	// Setup main sizer
	wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(msizer);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	msizer->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());

	// Add preview
	canvas_preview_ = new CropCanvas(this, image, palette);
	sizer->Add(canvas_preview_, 1, wxEXPAND | wxBOTTOM, UI::pad());

	// Add crop controls
	wxStaticBox*      frame      = new wxStaticBox(this, -1, "Crop Borders");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND | wxBOTTOM, UI::padLarge());

	// Absolute
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	framesizer->Add(hbox, 0, wxEXPAND | wxALL, UI::pad());
	rb_absolute_ = new wxRadioButton(frame, -1, "Absolute");
	rb_absolute_->SetValue(true);
	hbox->Add(rb_absolute_, 0, wxEXPAND | wxRIGHT, UI::pad());

	// Relative
	rb_relative_ = new wxRadioButton(frame, -1, "Relative");
	hbox->Add(rb_relative_, 0, wxEXPAND);

	wxGridBagSizer* gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	framesizer->Add(gb_sizer, 1, wxEXPAND | wxALL, UI::pad());

	// Left
	gb_sizer->Add(new wxStaticText(frame, -1, "Left:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	text_left_ = new NumberTextCtrl(frame);
	text_left_->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
	text_left_->setNumber(0);
	gb_sizer->Add(text_left_, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// Top
	gb_sizer->Add(new wxStaticText(frame, -1, "Top:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	text_top_ = new NumberTextCtrl(frame);
	text_top_->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
	text_top_->setNumber(0);
	gb_sizer->Add(text_top_, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

	// Right
	gb_sizer->Add(new wxStaticText(frame, -1, "Right:"), wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	text_right_ = new NumberTextCtrl(frame);
	text_right_->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
	text_right_->setNumber(image->width());
	gb_sizer->Add(text_right_, wxGBPosition(2, 1), wxDefaultSpan, wxEXPAND);

	// Bottom
	gb_sizer->Add(new wxStaticText(frame, -1, "Bottom:"), wxGBPosition(3, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	text_bottom_ = new NumberTextCtrl(frame);
	text_bottom_->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
	text_bottom_->setNumber(image->height());
	gb_sizer->Add(text_bottom_, wxGBPosition(3, 1), wxDefaultSpan, wxEXPAND);

	gb_sizer->AddGrowableCol(1);

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND);

	// Bind events
	text_left_->Bind(wxEVT_TEXT, &GfxCropDialog::onTextChanged, this);
	text_left_->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_left_->Bind(wxEVT_KILL_FOCUS, &GfxCropDialog::onLeftTextFocus, this);
	text_top_->Bind(wxEVT_TEXT, &GfxCropDialog::onTextChanged, this);
	text_top_->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_top_->Bind(wxEVT_KILL_FOCUS, &GfxCropDialog::onTopTextFocus, this);
	text_right_->Bind(wxEVT_TEXT, &GfxCropDialog::onTextChanged, this);
	text_right_->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_right_->Bind(wxEVT_KILL_FOCUS, &GfxCropDialog::onRightTextFocus, this);
	text_bottom_->Bind(wxEVT_TEXT, &GfxCropDialog::onTextChanged, this);
	text_bottom_->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_bottom_->Bind(wxEVT_KILL_FOCUS, &GfxCropDialog::onBottomTextFocus, this);
	rb_absolute_->Bind(wxEVT_RADIOBUTTON, &GfxCropDialog::onAbsoluteRelativeChanged, this);
	rb_relative_->Bind(wxEVT_RADIOBUTTON, &GfxCropDialog::onAbsoluteRelativeChanged, this);

	// Setup dialog size
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
	CenterOnParent();
}

// -----------------------------------------------------------------------------
// Update the preview canvas with the current crop settings
// -----------------------------------------------------------------------------
void GfxCropDialog::updatePreview()
{
	canvas_preview_->setCropRect(crop_rect_);
	canvas_preview_->Refresh();
	canvas_preview_->Update();
}

// -----------------------------------------------------------------------------
// Update the number text box values
// -----------------------------------------------------------------------------
void GfxCropDialog::updateValues()
{
	if (rb_absolute_->GetValue())
	{
		text_left_->setNumber(crop_rect_.tl.x);
		text_top_->setNumber(crop_rect_.tl.y);
		text_right_->setNumber(crop_rect_.br.x);
		text_bottom_->setNumber(crop_rect_.br.y);
	}
	else
	{
		text_left_->setNumber(crop_rect_.tl.x);
		text_top_->setNumber(crop_rect_.tl.y);
		text_right_->setNumber(crop_rect_.br.x - max_width_);
		text_bottom_->setNumber(crop_rect_.br.y - max_height_);
	}
}

// -----------------------------------------------------------------------------
// Set the left crop boundary to the current value in the text box, including
// some range checks
// -----------------------------------------------------------------------------
void GfxCropDialog::setLeft()
{
	int left = text_left_->number();

	if (left < 0)
		left = 0;
	else if (left > crop_rect_.br.x)
		left = crop_rect_.br.x - 1;

	crop_rect_.tl.x = left;
	text_left_->setNumber(left);
	updatePreview();
}

// -----------------------------------------------------------------------------
// Set the top crop boundary to the current value in the text box, including
// some range checks
// -----------------------------------------------------------------------------
void GfxCropDialog::setTop()
{
	int top = text_top_->number();

	if (top < 0)
		top = 0;
	else if (top > crop_rect_.br.y)
		top = crop_rect_.br.y - 1;

	crop_rect_.tl.y = top;
	text_top_->setNumber(top);
	updatePreview();
}

// -----------------------------------------------------------------------------
// Set the right crop boundary to the current value in the text box, including
// some range checks
// -----------------------------------------------------------------------------
void GfxCropDialog::setRight()
{
	int right = text_right_->number();
	if (rb_relative_->GetValue())
		right += max_width_;

	if (right > max_width_)
		right = max_width_;
	else if (right < crop_rect_.tl.x)
		right = crop_rect_.tl.x + 1;

	crop_rect_.br.x = right;
	if (rb_relative_->GetValue())
		right -= max_width_;
	text_right_->setNumber(right);
	updatePreview();
}

// -----------------------------------------------------------------------------
// Set the bottom crop boundary to the current value in the text box, including
// some range checks
// -----------------------------------------------------------------------------
void GfxCropDialog::setBottom()
{
	int bottom = text_bottom_->number();
	if (rb_relative_->GetValue())
		bottom += max_height_;

	if (bottom > max_height_)
		bottom = max_height_;
	else if (bottom < crop_rect_.tl.y)
		bottom = crop_rect_.tl.y + 1;

	crop_rect_.br.y = bottom;
	if (rb_relative_->GetValue())
		bottom -= max_height_;
	text_bottom_->setNumber(bottom);
	updatePreview();
}


// -----------------------------------------------------------------------------
//
// GfxCropDialog Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a textbox value is changed
// -----------------------------------------------------------------------------
void GfxCropDialog::onTextChanged(wxCommandEvent& e)
{
	// updatePreview();
}

// -----------------------------------------------------------------------------
// Called when enter is pressed in a text box
// -----------------------------------------------------------------------------
void GfxCropDialog::onTextEnter(wxCommandEvent& e)
{
	if (e.GetEventObject() == text_left_)
		setLeft();
	else if (e.GetEventObject() == text_top_)
		setTop();
	else if (e.GetEventObject() == text_right_)
		setRight();
	else if (e.GetEventObject() == text_bottom_)
		setBottom();
}

// -----------------------------------------------------------------------------
// Called when the left text box is (un)focused
// -----------------------------------------------------------------------------
void GfxCropDialog::onLeftTextFocus(wxFocusEvent& e)
{
	setLeft();
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the top text box is (un)focused
// -----------------------------------------------------------------------------
void GfxCropDialog::onTopTextFocus(wxFocusEvent& e)
{
	setTop();
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the right text box is (un)focused
// -----------------------------------------------------------------------------
void GfxCropDialog::onRightTextFocus(wxFocusEvent& e)
{
	setRight();
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the bottom text box is (un)focused
// -----------------------------------------------------------------------------
void GfxCropDialog::onBottomTextFocus(wxFocusEvent& e)
{
	setBottom();
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the 'Absolute'/'Relative' radio buttons change
// -----------------------------------------------------------------------------
void GfxCropDialog::onAbsoluteRelativeChanged(wxCommandEvent& e)
{
	updateValues();
}
