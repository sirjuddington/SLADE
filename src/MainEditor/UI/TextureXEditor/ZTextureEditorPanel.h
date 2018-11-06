#pragma once

#include "common.h"
#include "TextureEditorPanel.h"
#include "UI/Controls/ColourBox.h"

class wxSpinCtrlDouble;

class ZTextureEditorPanel : public TextureEditorPanel
{
public:
	ZTextureEditorPanel(wxWindow* parent, TextureXEditor* tx_editor) : TextureEditorPanel{ parent, tx_editor } {}
	~ZTextureEditorPanel() {}

	// UI Stuff
	wxPanel*	createTextureControls(wxWindow* parent) override;
	void		updateTextureControls() override;
	wxPanel*	createPatchControls(wxWindow* parent) override;
	void		updatePatchControls() override;

	// Editing
	void	addPatch() override;
	void	replacePatch() override;

	void	enableTranslationControls(bool enable);
	void	enableBlendControls(bool enable, bool tint = false);

private:
	// Texture properties
	wxSpinCtrlDouble*	spin_tex_scalex_;
	wxSpinCtrlDouble*	spin_tex_scaley_;
	wxSpinCtrl*			spin_tex_offsetx_;
	wxSpinCtrl*			spin_tex_offsety_;
	wxChoice*			choice_type_;
	wxCheckBox*			cb_optional_;
	wxCheckBox*			cb_worldpanning_;
	wxCheckBox*			cb_nodecals_;
	wxCheckBox*			cb_nulltexture_;

	// Patch properties
	wxCheckBox*			cb_flipx_;
	wxCheckBox*			cb_flipy_;
	wxCheckBox*			cb_useofs_;
	wxChoice*			choice_rotation_;
	wxSpinCtrlDouble*	spin_alpha_;
	wxChoice*			choice_style_;

	wxRadioButton*		rb_pc_normal_;
	wxRadioButton*		rb_pc_translation_;
	wxRadioButton*		rb_pc_blend_;
	wxRadioButton*		rb_pc_tint_;
	ColourBox*			cb_blend_col_;
	wxSpinCtrlDouble*	spin_tint_amount_;
	wxTextCtrl*			text_translation_;
	wxButton*			btn_edit_translation_;

	// Events
	void	onCBBlendRGBAChanged(wxCommandEvent& e);
	void	onChoiceViewTypeSelected(wxCommandEvent& e);
	void	onTexScaleXChanged(wxCommandEvent& e);
	void	onTexScaleYChanged(wxCommandEvent& e);
	void	onTexOffsetXChanged(wxCommandEvent& e);
	void	onTexOffsetYChanged(wxCommandEvent& e);
	void	onTexTypeChanged(wxCommandEvent& e);
	void	onTexOptionalChanged(wxCommandEvent& e);
	void	onTexWorldPanningChanged(wxCommandEvent& e);
	void	onTexNoDecalsChanged(wxCommandEvent& e);
	void	onTexNullTextureChanged(wxCommandEvent& e);
	void	onPatchFlipXChanged(wxCommandEvent& e);
	void	onPatchFlipYChanged(wxCommandEvent& e);
	void	onPatchUseOfsChanged(wxCommandEvent& e);
	void	onPatchRotationChanged(wxCommandEvent& e);
	void	onPatchAlphaChanged(wxCommandEvent& e);
	void	onPatchAlphaStyleChanged(wxCommandEvent& e);
	void	onPCNormalSelected(wxCommandEvent& e);
	void	onPCBlendSelected(wxCommandEvent& e);
	void	onPCTintSelected(wxCommandEvent& e);
	void	onPCTranslationSelected(wxCommandEvent& e);
	void	onPatchColourChanged(wxEvent& e);
	void	onPatchTintAmountChanged(wxCommandEvent& e);
	void	onBtnEditTranslation(wxCommandEvent& e);
	void	onTextTranslationEnter(wxCommandEvent& e);
};
