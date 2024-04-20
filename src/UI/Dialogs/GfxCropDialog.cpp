
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Graphics/SImage/SImage.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "UI/Canvas/GL/GLCanvas.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// CropCanvas Class
//
// -----------------------------------------------------------------------------
namespace slade
{
class CropCanvas : public GLCanvas
{
public:
	CropCanvas(wxWindow* parent, SImage& image) : GLCanvas(parent, BGStyle::Checkered), image_{ &image }
	{
		view_.setCentered(true);
		view_.setInterpolated(false);

		crop_rect_.set(0, 0, image.width(), image.height());

		int size = ui::scalePx(220);
		SetInitialSize(wxSize(size, size));
	}

	~CropCanvas() override { gl::Texture::clear(texture_); }

	const Recti& cropRect() const { return crop_rect_; }
	void         setCropRect(const Recti& rect) { crop_rect_.set(rect); }

	// Draw the canvas contents
	void draw() override
	{
		// Create texture if needed
		if (!texture_)
		{
			if (image_ && image_->isValid())
			{
				texture_ = gl::Texture::createFromImage(*image_, palette_.get(), gl::TexFilter::Nearest, false);
				crop_rect_.set(0, 0, image_->width(), image_->height());
			}
			else
				return;
		}

		gl::draw2d::Context dc(&view_);

		// Determine graphic position & scale

		// Get image dimensions
		auto& tex_info = gl::Texture::info(texture_);
		auto  x_dim    = static_cast<double>(tex_info.size.x);
		auto  y_dim    = static_cast<double>(tex_info.size.y);

		// Get max scale for x and y (including padding)
		auto x_scale = (static_cast<double>(view_.size().x) - ui::scalePx(24)) / x_dim;
		auto y_scale = (static_cast<double>(view_.size().y) - ui::scalePx(24)) / y_dim;

		// Set scale to smallest of the 2 (so that none of the texture will be clipped)
		view_.setScale(std::min<double>(x_scale, y_scale));

		// Draw graphic
		Rectf tex_rect;
		if (texture_)
		{
			tex_rect.set(x_dim * -0.5, y_dim * -0.5, x_dim * 0.5, y_dim * 0.5);
			dc.texture = texture_;
			dc.drawRect(tex_rect);
		}

		// Draw cropping rectangle
		dc.colour.set(0, 0, 0, 255);
		dc.texture = 0;
		dc.translate(tex_rect.tl.x, tex_rect.tl.y); // Translate to top-left of graphic
		dc.drawLines(vector<Rectf>{
			{ crop_rect_.tl.x, -1000, crop_rect_.tl.x, 1000 }, // Left
			{ -1000, crop_rect_.tl.y, 1000, crop_rect_.tl.y }, // Top
			{ crop_rect_.br.x, -1000, crop_rect_.br.x, 1000 }, // Right
			{ -1000, crop_rect_.br.y, 1000, crop_rect_.br.y }  // Bottom
		});

		// Shade cropped-out area
		dc.colour.set(0, 0, 0, 100);
		dc.drawRect({ -1000, -1000, crop_rect_.tl.x, 1000 });                      // Left
		dc.drawRect({ crop_rect_.br.x, -1000, 1000, 1000 });                       // Right
		dc.drawRect({ crop_rect_.tl.x, -1000, crop_rect_.br.x, crop_rect_.tl.y }); // Top
		dc.drawRect({ crop_rect_.tl.x, crop_rect_.br.y, crop_rect_.br.x, 1000 });  // Bottom
	}

private:
	SImage*  image_   = nullptr;
	unsigned texture_ = 0;
	Recti    crop_rect_;
};
} // namespace slade


// -----------------------------------------------------------------------------
//
// GfxCropDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GfxCropDialog class constructor
// -----------------------------------------------------------------------------
GfxCropDialog::GfxCropDialog(wxWindow* parent, SImage& image, const Palette* palette) :
	wxDialog(parent, -1, "Crop", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	namespace wx = wxutil;

	// Set max crop size
	max_width_  = image.width();
	max_height_ = image.height();
	crop_rect_.set(0, 0, max_width_, max_height_);

	// Set dialog icon
	wx::setWindowIcon(this, "crop");

	// Setup main sizer
	auto msizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(msizer);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	msizer->Add(sizer, wx::sfWithLargeBorder(1).Expand());

	// Add preview
	canvas_preview_ = new CropCanvas(this, image);
	canvas_preview_->setPalette(palette);
	sizer->Add(canvas_preview_, wx::sfWithBorder(1, wxBOTTOM).Expand());

	// Add crop controls
	auto frame      = new wxStaticBox(this, -1, "Crop Borders");
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wx::sfWithLargeBorder(0, wxBOTTOM).Expand());

	// Absolute
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	framesizer->Add(hbox, wx::sfWithBorder().Expand());
	rb_absolute_ = new wxRadioButton(frame, -1, "Absolute");
	rb_absolute_->SetValue(true);
	hbox->Add(rb_absolute_, wx::sfWithBorder(0, wxRIGHT).Expand());

	// Relative
	rb_relative_ = new wxRadioButton(frame, -1, "Relative");
	hbox->Add(rb_relative_, wxSizerFlags().Expand());

	auto gb_sizer = new wxGridBagSizer(ui::pad(), ui::pad());
	framesizer->Add(gb_sizer, wx::sfWithBorder(1).Expand());

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
	text_right_->setNumber(image.width());
	gb_sizer->Add(text_right_, wxGBPosition(2, 1), wxDefaultSpan, wxEXPAND);

	// Bottom
	gb_sizer->Add(new wxStaticText(frame, -1, "Bottom:"), wxGBPosition(3, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	text_bottom_ = new NumberTextCtrl(frame);
	text_bottom_->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
	text_bottom_->setNumber(image.height());
	gb_sizer->Add(text_bottom_, wxGBPosition(3, 1), wxDefaultSpan, wxEXPAND);

	gb_sizer->AddGrowableCol(1);

	// Add buttons
	sizer->Add(wx::createDialogButtonBox(this, "Crop", "Cancel"), wxSizerFlags().Expand());

	bindEvents();

	// Setup dialog size
	SetInitialSize(wxSize(-1, -1));
	const wxSize size = GetSize() * wxWindowBase::GetContentScaleFactor();
	wxTopLevelWindowBase::SetMinSize(size);
	CenterOnParent();
}

void GfxCropDialog::bindEvents()
{
	// Left text box
	text_left_->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_left_->Bind(
		wxEVT_KILL_FOCUS,
		[&](wxFocusEvent& e)
		{
			setLeft();
			e.Skip();
		});

	// Top text box
	text_top_->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_top_->Bind(
		wxEVT_KILL_FOCUS,
		[&](wxFocusEvent& e)
		{
			setTop();
			e.Skip();
		});

	// Right text box
	text_right_->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_right_->Bind(
		wxEVT_KILL_FOCUS,
		[&](wxFocusEvent& e)
		{
			setRight();
			e.Skip();
		});

	// Bottom text box
	text_bottom_->Bind(wxEVT_TEXT_ENTER, &GfxCropDialog::onTextEnter, this);
	text_bottom_->Bind(
		wxEVT_KILL_FOCUS,
		[&](wxFocusEvent& e)
		{
			setBottom();
			e.Skip();
		});

	// Absolute/Relative radio buttons
	rb_absolute_->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) { updateValues(); });
	rb_relative_->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) { updateValues(); });
}

// -----------------------------------------------------------------------------
// Update the preview canvas with the current crop settings
// -----------------------------------------------------------------------------
void GfxCropDialog::updatePreview() const
{
	canvas_preview_->setCropRect(crop_rect_);
	canvas_preview_->Refresh();
}

// -----------------------------------------------------------------------------
// Update the number text box values
// -----------------------------------------------------------------------------
void GfxCropDialog::updateValues() const
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

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

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
