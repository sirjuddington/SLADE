
#ifndef __ZTEXTURE_EDITOR_PANEL_H__
#define __ZTEXTURE_EDITOR_PANEL_H__

#include "common.h"
#include "TextureEditorPanel.h"
#include "UI/ColourBox.h"

class wxSpinCtrlDouble;
class ZTextureEditorPanel : public TextureEditorPanel
{
private:
	// Texture properties
	wxSpinCtrlDouble*	spin_tex_scalex;
	wxSpinCtrlDouble*	spin_tex_scaley;
	wxSpinCtrl*			spin_tex_offsetx;
	wxSpinCtrl*			spin_tex_offsety;
	wxChoice*			choice_type;
	wxCheckBox*			cb_optional;
	wxCheckBox*			cb_worldpanning;
	wxCheckBox*			cb_nodecals;
	wxCheckBox*			cb_nulltexture;

	// Patch properties
	wxCheckBox*			cb_flipx;
	wxCheckBox*			cb_flipy;
	wxCheckBox*			cb_useofs;
	wxChoice*			choice_rotation;
	wxSpinCtrlDouble*	spin_alpha;
	wxChoice*			choice_style;

	wxRadioButton*		rb_pc_normal;
	wxRadioButton*		rb_pc_translation;
	wxRadioButton*		rb_pc_blend;
	wxRadioButton*		rb_pc_tint;
	ColourBox*			cb_blend_col;
	wxSpinCtrlDouble*	spin_tint_amount;
	wxTextCtrl*			text_translation;
	wxButton*			btn_edit_translation;

public:
	ZTextureEditorPanel(wxWindow* parent, TextureXEditor* tx_editor);
	~ZTextureEditorPanel();

	// UI Stuff
	wxPanel*	createTextureControls(wxWindow* parent);
	void		updateTextureControls();
	wxPanel*	createPatchControls(wxWindow* parent);
	//void		populatePatchList();
	void		updatePatchControls();

	// Editing
	void	addPatch();
	void	replacePatch();

	void	enableTranslationControls(bool enable);
	void	enableBlendControls(bool enable, bool tint = false);

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

#endif//__ZTEXTURE_EDITOR_PANEL_H__
