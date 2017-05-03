
#ifndef __TRANSLATION_EDITOR_DIALOG_H__
#define __TRANSLATION_EDITOR_DIALOG_H__

#include "Graphics/Translation.h"
#include "Graphics/SImage/SImage.h"
#include "UI/Canvas/OGLCanvas.h"
#include "UI/WxBasicControls.h"
#include "UI/ColourBox.h"

class GfxCanvas;
class PaletteCanvas;
class Palette8bit;
class GradientBox : public OGLCanvas
{
private:
	rgba_t	col_start;
	rgba_t	col_end;
	int		steps;

public:
	GradientBox(wxWindow* parent, int steps = -1);
	~GradientBox();

	void	setStartCol(rgba_t col) { col_start.set(col.r, col.g, col.b, 255); }
	void	setEndCol(rgba_t col) { col_end.set(col.r, col.g, col.b, 255); }
	void	setSteps(int steps) { this->steps = steps; }

	void	draw();
};

class ArchiveEntry;
class wxListBox;
class wxPanel;
class TranslationEditorDialog : public wxDialog
{
private:
	Palette8bit*	palette;
	Translation		translation;
	SImage			image_preview;

	PaletteCanvas*	pal_canvas_original;
	wxListBox*		list_translations;
	wxButton*		btn_add;
	wxButton*		btn_remove;
	wxButton*		btn_up;
	wxButton*		btn_down;
	wxButton*		btn_load;
	wxButton*		btn_save;
	wxRadioButton*	rb_type_palette;
	wxRadioButton*	rb_type_colour;
	wxRadioButton*	rb_type_desaturate;
	wxRadioButton*	rb_type_colourise;
	wxRadioButton*	rb_type_tint;
	wxTextCtrl*		text_string;

	// Paletted target range
	wxPanel*		panel_target_palette;
	PaletteCanvas*	pal_canvas_target;
	wxCheckBox*		cb_target_reverse;

	// Colour gradient target range
	wxPanel*		panel_target_gradient;
	ColourBox*		cb_range_begin;
	ColourBox*		cb_range_end;
	GradientBox*	gb_gradient;

	// Colourise/tint target range
	wxPanel*		panel_target_tint;
	ColourBox*		cb_target_tint;
	wxSlider*		slider_tint;
	wxStaticText*	label_tint;
	wxStaticText*	label_amount;

	// Preview
	PaletteCanvas*	pal_canvas_preview;
	GfxCanvas*		gfx_preview;

	// Truecolor
	wxCheckBox*		cb_truecolor;
	wxCheckBox*		cb_paletteonly;

public:
	TranslationEditorDialog(wxWindow* parent, Palette8bit* pal, string title = "Edit Translation", SImage* preview_image = NULL);
	~TranslationEditorDialog();

	Translation&	getTranslation() { return translation; }
	bool			getTruecolor();

	void	openTranslation(Translation& trans);
	void	openRange(int index);
	void	updateListItem(int index);
	void	setStartColour(rgba_t col);
	void	setEndColour(rgba_t col);
	void	setTintColour(rgba_t col);
	void	setTintAmount(int amount);
	void	showPaletteTarget();
	void	showGradientTarget();
	void	showTintTarget(bool tint);
	void	updatePreviews();

	// Events
	void	onSize(wxSizeEvent& e);
	void	onTranslationListItemSelected(wxCommandEvent& e);
	void	onRBPaletteSelected(wxCommandEvent& e);
	void	onRBColourSelected(wxCommandEvent& e);
	void	onRBDesaturateSelected(wxCommandEvent& e);
	void	onRBColouriseSelected(wxCommandEvent& e);
	void	onRBTintSelected(wxCommandEvent& e);
	void	onBeginColourChanged(wxEvent& e);
	void	onEndColourChanged(wxEvent& e);
	void	onTintColourChanged(wxEvent& e);
	void	onPalOriginLeftUp(wxMouseEvent& e);
	void	onPalTargetLeftUp(wxMouseEvent& e);
	void	onTintAmountChanged(wxCommandEvent& e);
	void	onBtnRemove(wxCommandEvent& e);
	void	onBtnAdd(wxCommandEvent& e);
	void	onBtnUp(wxCommandEvent& e);
	void	onBtnDown(wxCommandEvent& e);
	void	onBtnLoad(wxCommandEvent& e);
	void	onBtnSave(wxCommandEvent& e);
	void	onGfxPreviewMouseMotion(wxMouseEvent& e);
	void	onCBTargetReverse(wxCommandEvent& e);
	void	onCBTruecolor(wxCommandEvent& e);
	void	onCBPaletteOnly(wxCommandEvent& e);
};


/*******************************************************************
 * GFXCOLOURISEDIALOG CLASS
 *******************************************************************
 A simple dialog for the 'Colourise' function, allows the user to
 select a colour and shows a preview of the colourised image
 */
class GfxColouriseDialog : public wxDialog
{
private:
	GfxCanvas*			gfx_preview;
	ArchiveEntry*		entry;
	Palette8bit*		palette;
	ColourBox*			cb_colour;

public:
	GfxColouriseDialog(wxWindow* parent, ArchiveEntry* entry, Palette8bit* pal);
	rgba_t getColour();
	void setColour(string col);
	void onColourChanged(wxEvent& e);
	void onResize(wxSizeEvent& e);
};


/*******************************************************************
 * GFXTINTDIALOG CLASS
 *******************************************************************
 A simple dialog for the 'Tint' function, allows the user to select
 tint colour+amount and shows a preview of the tinted image
 */
class GfxTintDialog : public wxDialog
{
private:
	GfxCanvas*			gfx_preview;
	ArchiveEntry*		entry;
	Palette8bit*		palette;
	ColourBox*			cb_colour;
	wxSlider*			slider_amount;
	wxStaticText*		label_amount;

public:
	GfxTintDialog(wxWindow* parent, ArchiveEntry* entry, Palette8bit* pal);
	rgba_t getColour();
	float getAmount();
	void setValues(string col, int val);
	void onColourChanged(wxEvent& e);
	void onAmountChanged(wxCommandEvent& e);
	void onResize(wxSizeEvent& e);
};



#endif//__TRANSLATION_EDITOR_DIALOG_H__
