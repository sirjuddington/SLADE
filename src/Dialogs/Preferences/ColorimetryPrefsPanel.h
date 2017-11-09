#pragma once

#include "PrefsPanelBase.h"

class wxSpinCtrlDouble;

class ColorimetryPrefsPanel : public PrefsPanelBase
{
public:
	ColorimetryPrefsPanel(wxWindow* parent);
	~ColorimetryPrefsPanel();

	void	init() override;
	void	applyPreferences() override;

private:
	wxSpinCtrlDouble*	spin_grey_r_;
	wxSpinCtrlDouble*	spin_grey_g_;
	wxSpinCtrlDouble*	spin_grey_b_;
	wxSpinCtrlDouble*	spin_factor_r_;
	wxSpinCtrlDouble*	spin_factor_g_;
	wxSpinCtrlDouble*	spin_factor_b_;
	wxSpinCtrlDouble*	spin_factor_h_;
	wxSpinCtrlDouble*	spin_factor_s_;
	wxSpinCtrlDouble*	spin_factor_l_;
	wxSpinCtrlDouble*	spin_tristim_x_;
	wxSpinCtrlDouble*	spin_tristim_z_;
	wxSpinCtrlDouble*	spin_cie_kl_;
	wxSpinCtrlDouble*	spin_cie_k1_;
	wxSpinCtrlDouble*	spin_cie_k2_;
	wxSpinCtrlDouble*	spin_cie_kc_;
	wxSpinCtrlDouble*	spin_cie_kh_;
	wxChoice*			choice_colmatch_;
	wxChoice*			choice_presets_grey_;
	wxChoice*			choice_presets_tristim_;

	void	setupLayout();

	// Events
	void	onChoiceColormatchSelected(wxCommandEvent& e);
	void	onChoiceGreyscalePresetSelected(wxCommandEvent& e);
	void	onChoiceTristimPresetSelected(wxCommandEvent& e);
};
