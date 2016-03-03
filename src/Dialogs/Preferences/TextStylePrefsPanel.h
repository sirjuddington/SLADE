
#ifndef __TEXT_STYLE_PREFS_PANEL_H__
#define __TEXT_STYLE_PREFS_PANEL_H__

#include "PrefsPanelBase.h"
#include "UI/TextEditor/TextStyle.h"
#include <wx/clrpicker.h>
#include <wx/fontpicker.h>

class wxListBox;
class TextLanguage;
class TextStylePrefsPanel : public PrefsPanelBase
{
private:
	bool				init_done;
	wxChoice*			choice_styleset;
	wxButton*			btn_savestyleset;
	wxListBox*			list_styles;
	StyleSet			ss_current;
	TextStyle*			ts_current;
	wxCheckBox*			cb_font_override;
	wxFontPickerCtrl*	fp_font_override;

	wxColourPickerCtrl*	cp_foreground;
	wxColourPickerCtrl*	cp_background;
	wxFontPickerCtrl*	fp_font;
	wxCheckBox*			cb_override_font_face;
	wxCheckBox*			cb_override_font_size;
	wxCheckBox*			cb_override_font_bold;
	wxCheckBox*			cb_override_font_italic;
	wxCheckBox*			cb_override_font_underlined;
	wxCheckBox*			cb_override_foreground;
	wxCheckBox*			cb_override_background;

	TextLanguage*		language_preview;
	TextEditor*			te_preview;

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

	void	init();
	void	applyPreferences();

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

#endif//__TEXT_STYLE_PREFS_PANEL_H__
