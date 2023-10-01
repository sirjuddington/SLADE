#pragma once

#include "UI/Canvas/GLCanvas.h"

namespace slade
{
class SImage;
class NumberTextCtrl;

class CropCanvas : public GLCanvas
{
public:
	CropCanvas(wxWindow* parent, SImage& image);
	~CropCanvas() override;

	const Recti& cropRect() const { return crop_rect_; }
	void         setCropRect(const Recti& rect) { crop_rect_.set(rect); }

	void draw() override;

private:
	SImage*  image_   = nullptr;
	unsigned texture_ = 0;
	Recti    crop_rect_;
};

class GfxCropDialog : public wxDialog
{
public:
	GfxCropDialog(wxWindow* parent, SImage& image, const Palette* palette);
	~GfxCropDialog() override = default;

	const Recti& cropRect() const { return crop_rect_; }
	void         updatePreview() const;

private:
	CropCanvas*     canvas_preview_ = nullptr;
	NumberTextCtrl* text_left_      = nullptr;
	NumberTextCtrl* text_top_       = nullptr;
	NumberTextCtrl* text_right_     = nullptr;
	NumberTextCtrl* text_bottom_    = nullptr;
	wxRadioButton*  rb_absolute_    = nullptr;
	wxRadioButton*  rb_relative_    = nullptr;

	int   max_width_  = 0;
	int   max_height_ = 0;
	Recti crop_rect_;

	void bindEvents();

	void updateValues() const;
	void setLeft();
	void setTop();
	void setRight();
	void setBottom();

	// Events
	void onTextEnter(wxCommandEvent& e);
};
} // namespace slade
