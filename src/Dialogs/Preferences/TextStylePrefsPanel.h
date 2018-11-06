#pragma once

#include "common.h"
#include "PrefsPanelBase.h"
#include "TextEditor/TextStyle.h"

class wxListBox;
class TextLanguage;

class TextStylePrefsPanel : public PrefsPanelBase
{
public:
	TextStylePrefsPanel(wxWindow* parent);
	~TextStylePrefsPanel();

	void	updateStyleControls();

	void	updateFontFace();
	void	updateFontSize();
	void	updateFontBold();
	void	updateFontItalic();
	void	updateFontUnderlined();
	void	updateForeground();
	void	updateBackground();
	void	updatePreview();

	void	init() override;
	void	applyPreferences() override;

	string pageTitle() override { return "Text Editor Fonts && Colours"; }

private:
	bool				init_done_;
	wxChoice*			choice_styleset_;
	wxButton*			btn_savestyleset_;
	wxListBox*			list_styles_;
	StyleSet			ss_current_;
	TextStyle*			ts_current_;
	wxCheckBox*			cb_font_override_;
	wxFontPickerCtrl*	fp_font_override_;

	wxColourPickerCtrl*	cp_foreground_;
	wxColourPickerCtrl*	cp_background_;
	wxFontPickerCtrl*	fp_font_;
	wxCheckBox*			cb_override_font_face_;
	wxCheckBox*			cb_override_font_size_;
	wxCheckBox*			cb_override_font_bold_;
	wxCheckBox*			cb_override_font_italic_;
	wxCheckBox*			cb_override_font_underlined_;
	wxCheckBox*			cb_override_foreground_;
	wxCheckBox*			cb_override_background_;

	TextLanguage*		language_preview_;
	TextEditorCtrl*		te_preview_;

	wxPanel*	createStylePanel();

	// Events
	void	onStyleSelected(wxCommandEvent& e);
	void	onCBOverrideFontFace(wxCommandEvent& e);
	void	onCBOverrideFontSize(wxCommandEvent& e);
	void	onCBOverrideFontBold(wxCommandEvent& e);
	void	onCBOverrideFontItalic(wxCommandEvent& e);
	void	onCBOverrideFontUnderlined(wxCommandEvent& e);
	void	onCBOverrideForeground(wxCommandEvent& e);
	void	onCBOverrideBackground(wxCommandEvent& e);
	void	onFontChanged(wxFontPickerEvent& e);
	void	onForegroundChanged(wxColourPickerEvent& e);
	void	onBackgroundChanged(wxColourPickerEvent& e);
	void	onBtnSaveStyleSet(wxCommandEvent& e);
	void	onStyleSetSelected(wxCommandEvent& e);
	void	onCBOverrideFont(wxCommandEvent& e);
	void	onFontOverrideChanged(wxFontPickerEvent& e);
};
