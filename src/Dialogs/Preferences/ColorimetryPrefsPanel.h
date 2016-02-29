
#ifndef COLORIMETRY_PREFS_PANEL_H
#define COLORIMETRY_PREFS_PANEL_H

#include "PrefsPanelBase.h"

class wxSpinCtrlDouble;
class ColorimetryPrefsPanel : public PrefsPanelBase
{
private:
	wxSpinCtrlDouble*	spin_grey_r;
	wxSpinCtrlDouble*	spin_grey_g;
	wxSpinCtrlDouble*	spin_grey_b;
	wxSpinCtrlDouble*	spin_factor_r;
	wxSpinCtrlDouble*	spin_factor_g;
	wxSpinCtrlDouble*	spin_factor_b;
	wxSpinCtrlDouble*	spin_factor_h;
	wxSpinCtrlDouble*	spin_factor_s;
	wxSpinCtrlDouble*	spin_factor_l;
	wxSpinCtrlDouble*	spin_tristim_x;
	wxSpinCtrlDouble*	spin_tristim_z;
	wxSpinCtrlDouble*	spin_cie_kl;
	wxSpinCtrlDouble*	spin_cie_k1;
	wxSpinCtrlDouble*	spin_cie_k2;
	wxSpinCtrlDouble*	spin_cie_kc;
	wxSpinCtrlDouble*	spin_cie_kh;
	wxChoice*			choice_colmatch;
	wxChoice*			choice_presets_grey;
	wxChoice*			choice_presets_tristim;

public:
	ColorimetryPrefsPanel(wxWindow* parent);
	~ColorimetryPrefsPanel();

	void	init();
	void	applyPreferences();

	// Events
	void	onChoiceColormatchSelected(wxCommandEvent& e);
	void	onChoiceGreyscalePresetSelected(wxCommandEvent& e);
	void	onChoiceTristimPresetSelected(wxCommandEvent& e);
};

#endif// COLORIMETRY_PREFS_PANEL_H
