
#ifndef __HUD_OFFSETS_PREFS_PANEL_H__
#define __HUD_OFFSETS_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class HudOffsetsPrefsPanel : public PrefsPanelBase
{
private:
	wxCheckBox*	cb_hud_bob;
	wxCheckBox*	cb_hud_center;
	wxCheckBox*	cb_hud_statusbar;
	wxCheckBox*	cb_hud_wide;

public:
	HudOffsetsPrefsPanel(wxWindow* parent);
	~HudOffsetsPrefsPanel();

	void	init();
	void	applyPreferences();
};

#endif//__HUD_OFFSETS_PREFS_PANEL_H__
