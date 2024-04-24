#pragma once

#include "Graphics/SImage/SImage.h"
#include "Graphics/Translation.h"
#include "UI/Controls/ColourBox.h"

class wxListBox;
class wxPanel;

namespace slade
{
class ColourBox;
class GfxCanvasBase;
class PaletteCanvas;
class ArchiveEntry;

class GradientBox : public wxPanel
{
public:
	GradientBox(wxWindow* parent, int steps = -1);
	~GradientBox() override = default;

	void setStartCol(ColRGBA col) { col_start_.set(col.r, col.g, col.b, 255); }
	void setEndCol(ColRGBA col) { col_end_.set(col.r, col.g, col.b, 255); }
	void setSteps(int steps) { steps_ = steps; }

private:
	ColRGBA col_start_ = ColRGBA::BLACK;
	ColRGBA col_end_   = ColRGBA::WHITE;
	int     steps_     = 0;
};

class TranslationEditorDialog : public wxDialog
{
public:
	TranslationEditorDialog(
		wxWindow*       parent,
		const Palette&  pal,
		const wxString& title         = "Edit Translation",
		const SImage*   preview_image = nullptr);
	~TranslationEditorDialog() override;

	Translation& getTranslation() const { return *translation_; }
	bool         getTruecolor() const;

	void openTranslation(const Translation& trans);
	void openRange(int index);
	void updateListItem(int index) const;
	void setStartColour(const ColRGBA& col) const;
	void setEndColour(const ColRGBA& col) const;
	void setTintColour(const ColRGBA& col) const;
	void setTintAmount(int amount) const;
	void showPaletteTarget();
	void showGradientTarget();
	void showTintTarget(bool tint);
	void updatePreviews() const;

private:
	unique_ptr<Palette>     palette_;
	unique_ptr<Translation> translation_;
	unique_ptr<SImage>      image_preview_;

	PaletteCanvas* pal_canvas_original_ = nullptr;
	wxListBox*     list_translations_   = nullptr;
	wxButton*      btn_add_             = nullptr;
	wxButton*      btn_remove_          = nullptr;
	wxButton*      btn_up_              = nullptr;
	wxButton*      btn_down_            = nullptr;
	wxButton*      btn_load_            = nullptr;
	wxButton*      btn_save_            = nullptr;
	wxRadioButton* rb_type_palette_     = nullptr;
	wxRadioButton* rb_type_colour_      = nullptr;
	wxRadioButton* rb_type_desaturate_  = nullptr;
	wxRadioButton* rb_type_colourise_   = nullptr;
	wxRadioButton* rb_type_tint_        = nullptr;
	wxTextCtrl*    text_string_         = nullptr;

	// Paletted target range
	wxPanel*       panel_target_palette_ = nullptr;
	PaletteCanvas* pal_canvas_target_    = nullptr;
	wxCheckBox*    cb_target_reverse_    = nullptr;

	// Colour gradient target range
	wxPanel*     panel_target_gradient_ = nullptr;
	ColourBox*   cb_range_begin_        = nullptr;
	ColourBox*   cb_range_end_          = nullptr;
	GradientBox* gb_gradient_           = nullptr;

	// Colourise/tint target range
	wxPanel*      panel_target_tint_ = nullptr;
	ColourBox*    cb_target_tint_    = nullptr;
	wxSlider*     slider_tint_       = nullptr;
	wxStaticText* label_tint_        = nullptr;
	wxStaticText* label_amount_      = nullptr;

	// Preview
	PaletteCanvas* pal_canvas_preview_ = nullptr;
	GfxCanvasBase* gfx_preview_        = nullptr;

	// Truecolor
	wxCheckBox* cb_truecolor_   = nullptr;
	wxCheckBox* cb_paletteonly_ = nullptr;


	// Events
	void onSize(wxSizeEvent& e);
	void onTranslationListItemSelected(wxCommandEvent& e);
	void onRBPaletteSelected(wxCommandEvent& e);
	void onRBColourSelected(wxCommandEvent& e);
	void onRBDesaturateSelected(wxCommandEvent& e);
	void onRBColouriseSelected(wxCommandEvent& e);
	void onRBTintSelected(wxCommandEvent& e);
	void onBeginColourChanged(wxEvent& e);
	void onEndColourChanged(wxEvent& e);
	void onTintColourChanged(wxEvent& e);
	void onPalOriginLeftUp(wxMouseEvent& e);
	void onPalTargetLeftUp(wxMouseEvent& e);
	void onTintAmountChanged(wxCommandEvent& e);
	void onBtnRemove(wxCommandEvent& e);
	void onBtnAdd(wxCommandEvent& e);
	void onBtnUp(wxCommandEvent& e);
	void onBtnDown(wxCommandEvent& e);
	void onBtnLoad(wxCommandEvent& e);
	void onBtnSave(wxCommandEvent& e);
	void onGfxPreviewMouseMotion(wxMouseEvent& e);
	void onCBTargetReverse(wxCommandEvent& e);
	void onCBTruecolor(wxCommandEvent& e);
	void onCBPaletteOnly(wxCommandEvent& e);
};
} // namespace slade
