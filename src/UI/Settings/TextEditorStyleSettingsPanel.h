#pragma once

#include "SettingsPanel.h"

class wxListBox;

namespace slade
{
class TextLanguage;
class StyleSet;
class TextEditorCtrl;
class TextStyle;

namespace ui
{
	class TextEditorStyleSettingsPanel : public SettingsPanel
	{
	public:
		TextEditorStyleSettingsPanel(wxWindow* parent);
		~TextEditorStyleSettingsPanel() override = default;

		string title() const override { return "Text Editor Fonts & Colours"; }

		void loadSettings() override;
		void applySettings() override;

	private:
		bool                 init_done_        = false;
		wxChoice*            choice_styleset_  = nullptr;
		wxButton*            btn_savestyleset_ = nullptr;
		wxListBox*           list_styles_      = nullptr;
		unique_ptr<StyleSet> ss_current_;
		TextStyle*           ts_current_       = nullptr;
		wxCheckBox*          cb_font_override_ = nullptr;
		wxFontPickerCtrl*    fp_font_override_ = nullptr;

		wxColourPickerCtrl* cp_foreground_               = nullptr;
		wxColourPickerCtrl* cp_background_               = nullptr;
		wxFontPickerCtrl*   fp_font_                     = nullptr;
		wxCheckBox*         cb_override_font_face_       = nullptr;
		wxCheckBox*         cb_override_font_size_       = nullptr;
		wxCheckBox*         cb_override_font_bold_       = nullptr;
		wxCheckBox*         cb_override_font_italic_     = nullptr;
		wxCheckBox*         cb_override_font_underlined_ = nullptr;
		wxCheckBox*         cb_override_foreground_      = nullptr;
		wxCheckBox*         cb_override_background_      = nullptr;

		unique_ptr<TextLanguage> language_preview_;
		TextEditorCtrl*          te_preview_ = nullptr;

		wxPanel* createStylePanel();

		void updateStyleControls() const;
		void updateFontFace() const;
		void updateFontSize() const;
		void updateFontBold() const;
		void updateFontItalic() const;
		void updateFontUnderlined() const;
		void updateForeground() const;
		void updateBackground() const;
		void updatePreview() const;

		// Events
		void onStyleSelected(wxCommandEvent& e);
		void onCBOverrideFontFace(wxCommandEvent& e);
		void onCBOverrideFontSize(wxCommandEvent& e);
		void onCBOverrideFontBold(wxCommandEvent& e);
		void onCBOverrideFontItalic(wxCommandEvent& e);
		void onCBOverrideFontUnderlined(wxCommandEvent& e);
		void onCBOverrideForeground(wxCommandEvent& e);
		void onCBOverrideBackground(wxCommandEvent& e);
		void onFontChanged(wxFontPickerEvent& e);
		void onForegroundChanged(wxColourPickerEvent& e);
		void onBackgroundChanged(wxColourPickerEvent& e);
		void onBtnSaveStyleSet(wxCommandEvent& e);
		void onStyleSetSelected(wxCommandEvent& e);
		void onCBOverrideFont(wxCommandEvent& e);
		void onFontOverrideChanged(wxFontPickerEvent& e);
	};
} // namespace ui
} // namespace slade
