#pragma once

#include "UI/Canvas/OGLCanvas.h"
#include "OpenGL/GLTexture.h"

class SImage;
class NumberTextCtrl;
class GLTexture;

class CropCanvas : public OGLCanvas
{
public:
	CropCanvas(wxWindow* parent, SImage* image, Palette8bit* palette);

	rect_t	cropRect() { return crop_rect; }
	void	setCropRect(rect_t& rect) { crop_rect.set(rect); }

	void	draw() override;

private:
	std::unique_ptr<GLTexture>	texture;
	rect_t						crop_rect;
};

class GfxCropDialog : public wxDialog
{
public:
	GfxCropDialog(wxWindow* parent, SImage* image, Palette8bit* palette);
	~GfxCropDialog() {}

	rect_t	getCropRect() { return crop_rect; }
	void	updatePreview();

private:
	CropCanvas*		canvas_preview;
	NumberTextCtrl*	text_left;
	NumberTextCtrl*	text_top;
	NumberTextCtrl*	text_right;
	NumberTextCtrl*	text_bottom;
	wxRadioButton*	rb_absolute;
	wxRadioButton*	rb_relative;

	int		max_width;
	int		max_height;
	rect_t	crop_rect;

	void	updateValues();
	void	setLeft();
	void	setTop();
	void	setRight();
	void	setBottom();

	void	onTextChanged(wxCommandEvent& e);
	void	onTextEnter(wxCommandEvent& e);
	void	onLeftTextFocus(wxFocusEvent& e);
	void	onTopTextFocus(wxFocusEvent& e);
	void	onRightTextFocus(wxFocusEvent& e);
	void	onBottomTextFocus(wxFocusEvent& e);
	void	onAbsoluteRelativeChanged(wxCommandEvent& e);
};
