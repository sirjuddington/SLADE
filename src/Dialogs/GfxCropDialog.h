#pragma once

#include "OpenGL/GLTexture.h"
#include "UI/Canvas/OGLCanvas.h"

class SImage;
class NumberTextCtrl;
class GLTexture;

class CropCanvas : public OGLCanvas
{
public:
	CropCanvas(wxWindow* parent, SImage* image, Palette* palette);

	Recti cropRect() { return crop_rect_; }
	void   setCropRect(Recti& rect) { crop_rect_.set(rect); }

	void draw() override;

private:
	std::unique_ptr<GLTexture> texture_;
	Recti                     crop_rect_;
};

class GfxCropDialog : public wxDialog
{
public:
	GfxCropDialog(wxWindow* parent, SImage* image, Palette* palette);
	~GfxCropDialog() {}

	Recti cropRect() { return crop_rect_; }
	void   updatePreview();

private:
	CropCanvas*     canvas_preview_;
	NumberTextCtrl* text_left_;
	NumberTextCtrl* text_top_;
	NumberTextCtrl* text_right_;
	NumberTextCtrl* text_bottom_;
	wxRadioButton*  rb_absolute_;
	wxRadioButton*  rb_relative_;

	int    max_width_;
	int    max_height_;
	Recti crop_rect_;

	void updateValues();
	void setLeft();
	void setTop();
	void setRight();
	void setBottom();

	void onTextChanged(wxCommandEvent& e);
	void onTextEnter(wxCommandEvent& e);
	void onLeftTextFocus(wxFocusEvent& e);
	void onTopTextFocus(wxFocusEvent& e);
	void onRightTextFocus(wxFocusEvent& e);
	void onBottomTextFocus(wxFocusEvent& e);
	void onAbsoluteRelativeChanged(wxCommandEvent& e);
};
