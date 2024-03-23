#pragma once

#include "TextureEditorPanel.h"

class wxSpinCtrlDouble;

namespace slade
{
class ColourBox;

class ZTextureEditorPanel : public TextureEditorPanel
{
public:
	ZTextureEditorPanel(wxWindow* parent, TextureXEditor* tx_editor) : TextureEditorPanel{ parent, tx_editor } {}
	~ZTextureEditorPanel() override = default;

	// UI Stuff
	wxPanel* createTextureControls(wxWindow* parent) override;
	void     updateTextureControls() override;
	wxPanel* createPatchControls(wxWindow* parent) override;
	void     updatePatchControls() override;

	// Editing
	void addPatch() override;
	void replacePatch() override;

	void enableTranslationControls(bool enable) const;
	void enableBlendControls(bool enable, bool tint = false) const;

private:
	// Texture properties
	wxSpinCtrlDouble* spin_tex_scalex_  = nullptr;
	wxSpinCtrlDouble* spin_tex_scaley_  = nullptr;
	wxSpinCtrl*       spin_tex_offsetx_ = nullptr;
	wxSpinCtrl*       spin_tex_offsety_ = nullptr;
	wxChoice*         choice_type_      = nullptr;
	wxCheckBox*       cb_optional_      = nullptr;
	wxCheckBox*       cb_worldpanning_  = nullptr;
	wxCheckBox*       cb_nodecals_      = nullptr;
	wxCheckBox*       cb_nulltexture_   = nullptr;

	// Patch properties
	wxCheckBox*       cb_flipx_        = nullptr;
	wxCheckBox*       cb_flipy_        = nullptr;
	wxCheckBox*       cb_useofs_       = nullptr;
	wxChoice*         choice_rotation_ = nullptr;
	wxSpinCtrlDouble* spin_alpha_      = nullptr;
	wxChoice*         choice_style_    = nullptr;

	wxRadioButton*    rb_pc_normal_         = nullptr;
	wxRadioButton*    rb_pc_translation_    = nullptr;
	wxRadioButton*    rb_pc_blend_          = nullptr;
	wxRadioButton*    rb_pc_tint_           = nullptr;
	ColourBox*        cb_blend_col_         = nullptr;
	wxSpinCtrlDouble* spin_tint_amount_     = nullptr;
	wxTextCtrl*       text_translation_     = nullptr;
	wxButton*         btn_edit_translation_ = nullptr;

	// Events
	void onCBBlendRGBAChanged(wxCommandEvent& e);
	void onChoiceViewTypeSelected(wxCommandEvent& e);
	void onTexScaleXChanged(wxCommandEvent& e) override;
	void onTexScaleYChanged(wxCommandEvent& e) override;
	void onTexOffsetXChanged(wxCommandEvent& e);
	void onTexOffsetYChanged(wxCommandEvent& e);
	void onTexTypeChanged(wxCommandEvent& e);
	void onTexOptionalChanged(wxCommandEvent& e);
	void onTexWorldPanningChanged(wxCommandEvent& e) override;
	void onTexNoDecalsChanged(wxCommandEvent& e);
	void onTexNullTextureChanged(wxCommandEvent& e);
	void onPatchFlipXChanged(wxCommandEvent& e);
	void onPatchFlipYChanged(wxCommandEvent& e);
	void onPatchUseOfsChanged(wxCommandEvent& e);
	void onPatchRotationChanged(wxCommandEvent& e);
	void onPatchAlphaChanged(wxCommandEvent& e);
	void onPatchAlphaStyleChanged(wxCommandEvent& e);
	void onPCNormalSelected(wxCommandEvent& e);
	void onPCBlendSelected(wxCommandEvent& e);
	void onPCTintSelected(wxCommandEvent& e);
	void onPCTranslationSelected(wxCommandEvent& e);
	void onPatchColourChanged(wxEvent& e);
	void onPatchTintAmountChanged(wxCommandEvent& e);
	void onBtnEditTranslation(wxCommandEvent& e);
	void onTextTranslationEnter(wxCommandEvent& e);
};
} // namespace slade
