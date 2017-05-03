
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    GfxCropDialog.cpp
 * Description: A dialog UI for cropping a graphic
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
#include "GfxCropDialog.h"
#include "Graphics/Icons.h"
#include "Graphics/SImage/SImage.h"
#include "OpenGL/Drawing.h"
#include "UI/NumberTextCtrl.h"


/*******************************************************************
 * CROPCANVAS CLASS FUNCTIONS
 *******************************************************************/

/* CropCanvas::CropCanvas
 * CropCanvas class constructor
 *******************************************************************/
CropCanvas::CropCanvas(wxWindow* parent, SImage* image, Palette8bit* palette) :
	OGLCanvas(parent, -1, false)
{
	if (image && image->isValid())
	{
		texture = std::make_unique<GLTexture>();
		texture->loadImage(image, palette);
		crop_rect.set(0, 0, image->getWidth(), image->getHeight());
	}

	SetInitialSize(wxSize(220, 220));
}

/* CropCanvas::draw
 * Draw the canvas contents
 *******************************************************************/
void CropCanvas::draw()
{
	// Setup for 2d
	setup2D();

	// Draw background
	drawCheckeredBackground();

	// Determine graphic position & scale
	double width = GetSize().x;
	double height = GetSize().y;

	// Get image dimensions
	double x_dim = (double)texture->getWidth();
	double y_dim = (double)texture->getHeight();

	// Get max scale for x and y (including padding)
	double x_scale = ((double)width - 24) / x_dim;
	double y_scale = ((double)height - 24) / y_dim;

	// Set scale to smallest of the 2 (so that none of the texture will be clipped)
	double scale = MIN(x_scale, y_scale);

	glPushMatrix();
	glTranslated(width*0.5, height*0.5, 0);	// Translate to middle of area
	glScaled(scale, scale, scale);			// Scale to fit within area

	// Draw graphic
	double hw = 0;
	double hh = 0;
	if (texture)
	{
		glEnable(GL_TEXTURE_2D);
		hw = texture->getWidth()*-0.5;
		hh = texture->getHeight()*-0.5;
		texture->draw2d(hw, hh);
	}

	// Draw cropping rectangle
	OpenGL::setColour(COL_BLACK);
	glDisable(GL_TEXTURE_2D);
	glTranslated(hw, hh, 0);	// Translate to top-left of graphic
	Drawing::drawLine(crop_rect.tl.x, -1000, crop_rect.tl.x, 1000);	// Left
	Drawing::drawLine(-1000, crop_rect.tl.y, 1000, crop_rect.tl.y);	// Top
	Drawing::drawLine(crop_rect.br.x, -1000, crop_rect.br.x, 1000);	// Right
	Drawing::drawLine(-1000, crop_rect.br.y, 1000, crop_rect.br.y);	// Bottom

	// Shade cropped-out area
	OpenGL::setColour(rgba_t(0, 0, 0, 100, BLEND_NORMAL));
	Drawing::drawFilledRect(-1000, -1000, crop_rect.tl.x, 1000);					// Left
	Drawing::drawFilledRect(crop_rect.br.x, -1000, 1000, 1000);						// Right
	Drawing::drawFilledRect(crop_rect.tl.x, -1000, crop_rect.br.x, crop_rect.tl.y);	// Top
	Drawing::drawFilledRect(crop_rect.tl.x, crop_rect.br.y, crop_rect.br.x, 1000);	// Bottom

	glPopMatrix();

	SwapBuffers();
}


/*******************************************************************
 * GFXCROPDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* GfxCropDialog::GfxCropDialog
 * GfxCropDialog class constructor
 *******************************************************************/
GfxCropDialog::GfxCropDialog(wxWindow* parent, SImage* image, Palette8bit* palette) :
	wxDialog(parent, -1, "Crop", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// Set max crop size
	if (image)
	{
		max_width = image->getWidth();
		max_height = image->getHeight();
	}
	else
		max_width = max_height = 0;
	crop_rect.set(0, 0, max_width, max_height);

	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "settings"));
	SetIcon(icon);

	// Setup main sizer
	wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(msizer);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	msizer->Add(sizer, 1, wxEXPAND | wxALL, 6);

	// Add preview
	canvas_preview = new CropCanvas(this, image, palette);
	sizer->Add(canvas_preview, 1, wxEXPAND | wxALL, 4);

	// Add crop controls
	wxStaticBox* frame = new wxStaticBox(this, -1, "Crop Borders");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND | wxALL, 4);

	// Absolute
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	framesizer->Add(hbox, 0, wxEXPAND | wxALL, 4);
	rb_absolute = new wxRadioButton(frame, -1, "Absolute");
	rb_absolute->SetValue(true);
	hbox->Add(rb_absolute, 0, wxEXPAND | wxRIGHT, 4);

	// Relative
	rb_relative = new wxRadioButton(frame, -1, "Relative");
	hbox->Add(rb_relative, 0, wxEXPAND);

	wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
	framesizer->Add(gb_sizer, 1, wxEXPAND | wxALL, 4);

	// Left
	gb_sizer->Add(
		new wxStaticText(frame, -1, "Left:"),
		wxGBPosition(0, 0),
		wxDefaultSpan,
		wxALIGN_CENTER_VERTICAL
	);
	text_left = new NumberTextCtrl(frame);
	text_left->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
	text_left->setNumber(0);
	gb_sizer->Add(text_left, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// Top
	gb_sizer->Add(
		new wxStaticText(frame, -1, "Top:"),
		wxGBPosition(1, 0),
		wxDefaultSpan,
		wxALIGN_CENTER_VERTICAL
	);
	text_top = new NumberTextCtrl(frame);
	text_top->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
	text_top->setNumber(0);
	gb_sizer->Add(text_top, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

	// Right
	gb_sizer->Add(
		new wxStaticText(frame, -1, "Right:"),
		wxGBPosition(2, 0),
		wxDefaultSpan,
		wxALIGN_CENTER_VERTICAL
	);
	text_right = new NumberTextCtrl(frame);
	text_right->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
	text_right->setNumber(image->getWidth());
	gb_sizer->Add(text_right, wxGBPosition(2, 1), wxDefaultSpan, wxEXPAND);

	// Bottom
	gb_sizer->Add(
		new wxStaticText(frame, -1, "Bottom:"),
		wxGBPosition(3, 0),
		wxDefaultSpan,
		wxALIGN_CENTER_VERTICAL
	);
	text_bottom = new NumberTextCtrl(frame);
	text_bottom->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
	text_bottom->setNumber(image->getHeight());
	gb_sizer->Add(text_bottom, wxGBPosition(3, 1), wxDefaultSpan, wxEXPAND);

	gb_sizer->AddGrowableCol(1);

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxBOTTOM, 4);

	// Bind events
	text_left->Bind(wxEVT_TEXT, &GfxCropDialog::onTextChanged, this);
	text_left->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_left->Bind(wxEVT_KILL_FOCUS, &GfxCropDialog::onLeftTextFocus, this);
	text_top->Bind(wxEVT_TEXT, &GfxCropDialog::onTextChanged, this);
	text_top->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_top->Bind(wxEVT_KILL_FOCUS, &GfxCropDialog::onTopTextFocus, this);
	text_right->Bind(wxEVT_TEXT, &GfxCropDialog::onTextChanged, this);
	text_right->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_right->Bind(wxEVT_KILL_FOCUS, &GfxCropDialog::onRightTextFocus, this);
	text_bottom->Bind(wxEVT_TEXT, &GfxCropDialog::onTextChanged, this);
	text_bottom->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_bottom->Bind(wxEVT_KILL_FOCUS, &GfxCropDialog::onBottomTextFocus, this);
	rb_absolute->Bind(wxEVT_RADIOBUTTON, &GfxCropDialog::onAbsoluteRelativeChanged, this);
	rb_relative->Bind(wxEVT_RADIOBUTTON, &GfxCropDialog::onAbsoluteRelativeChanged, this);

	// Setup dialog size
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
	CenterOnParent();
}

/* GfxCropDialog::updatePreview
 * Update the preview canvas with the current crop settings
 *******************************************************************/
void GfxCropDialog::updatePreview()
{
	canvas_preview->setCropRect(crop_rect);
	canvas_preview->Refresh();
	canvas_preview->Update();
}

/* GfxCropDialog::updateValues
 * Update the number text box values
 *******************************************************************/
void GfxCropDialog::updateValues()
{
	if (rb_absolute->GetValue())
	{
		text_left->setNumber(crop_rect.tl.x);
		text_top->setNumber(crop_rect.tl.y);
		text_right->setNumber(crop_rect.br.x);
		text_bottom->setNumber(crop_rect.br.y);
	}
	else
	{
		text_left->setNumber(crop_rect.tl.x);
		text_top->setNumber(crop_rect.tl.y);
		text_right->setNumber(crop_rect.br.x - max_width);
		text_bottom->setNumber(crop_rect.br.y - max_height);
	}
}

/* GfxCropDialog::setLeft
 * Set the left crop boundary to the current value in the text box,
 * including some range checks
 *******************************************************************/
void GfxCropDialog::setLeft()
{
	int left = text_left->getNumber();

	if (left < 0)
		left = 0;
	else if (left > crop_rect.br.x)
		left = crop_rect.br.x - 1;
	
	crop_rect.tl.x = left;
	text_left->setNumber(left);
	updatePreview();
}

/* GfxCropDialog::setTop
 * Set the top crop boundary to the current value in the text box,
 * including some range checks
 *******************************************************************/
void GfxCropDialog::setTop()
{
	int top = text_top->getNumber();

	if (top < 0)
		top = 0;
	else if (top > crop_rect.br.y)
		top = crop_rect.br.y - 1;
	
	crop_rect.tl.y = top;
	text_top->setNumber(top);
	updatePreview();
}

/* GfxCropDialog::setRight
 * Set the right crop boundary to the current value in the text box,
 * including some range checks
 *******************************************************************/
void GfxCropDialog::setRight()
{
	int right = text_right->getNumber();
	if (rb_relative->GetValue())
		right += max_width;

	if (right > max_width)
		right = max_width;
	else if (right < crop_rect.tl.x)
		right = crop_rect.tl.x + 1;
	
	crop_rect.br.x = right;
	if (rb_relative->GetValue())
		right -= max_width;
	text_right->setNumber(right);
	updatePreview();
}

/* GfxCropDialog::setBottom
 * Set the bottom crop boundary to the current value in the text box,
 * including some range checks
 *******************************************************************/
void GfxCropDialog::setBottom()
{
	int bottom = text_bottom->getNumber();
	if (rb_relative->GetValue())
		bottom += max_height;

	if (bottom > max_height)
		bottom = max_height;
	else if (bottom < crop_rect.tl.y)
		bottom = crop_rect.tl.y + 1;

	crop_rect.br.y = bottom;
	if (rb_relative->GetValue())
		bottom -= max_height;
	text_bottom->setNumber(bottom);
	updatePreview();
}


/*******************************************************************
 * GFXCROPDIALOG CLASS EVENTS
 *******************************************************************/

/* GfxCropDialog::onTextChanged
 * Called when a textbox value is changed
 *******************************************************************/
void GfxCropDialog::onTextChanged(wxCommandEvent& e)
{
	//updatePreview();
}

/* GfxCropDialog::onTextEnter
 * Called when enter is pressed in a text box
 *******************************************************************/
void GfxCropDialog::onTextEnter(wxCommandEvent& e)
{
	if (e.GetEventObject() == text_left)
		setLeft();
	else if (e.GetEventObject() == text_top)
		setTop();
	else if (e.GetEventObject() == text_right)
		setRight();
	else if (e.GetEventObject() == text_bottom)
		setBottom();
}

/* GfxCropDialog::onLeftTextFocus
 * Called when the left text box is (un)focused
 *******************************************************************/
void GfxCropDialog::onLeftTextFocus(wxFocusEvent& e)
{
	setLeft();
	e.Skip();
}

/* GfxCropDialog::onTopTextFocus
 * Called when the top text box is (un)focused
 *******************************************************************/
void GfxCropDialog::onTopTextFocus(wxFocusEvent& e)
{
	setTop();
	e.Skip();
}

/* GfxCropDialog::onRightTextFocus
 * Called when the right text box is (un)focused
 *******************************************************************/
void GfxCropDialog::onRightTextFocus(wxFocusEvent& e)
{
	setRight();
	e.Skip();
}

/* GfxCropDialog::onBottomTextFocus
 * Called when the bottom text box is (un)focused
 *******************************************************************/
void GfxCropDialog::onBottomTextFocus(wxFocusEvent& e)
{
	setBottom();
	e.Skip();
}

/* GfxCropDialog::onAbsoluteRelativeChanged
 * Called when the 'Absolute'/'Relative' radio buttons change
 *******************************************************************/
void GfxCropDialog::onAbsoluteRelativeChanged(wxCommandEvent& e)
{
	updateValues();
}
