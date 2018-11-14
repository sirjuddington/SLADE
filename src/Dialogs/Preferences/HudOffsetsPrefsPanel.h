#pragma once

#include "PrefsPanelBase.h"

class HudOffsetsPrefsPanel : public PrefsPanelBase
{
public:
	HudOffsetsPrefsPanel(wxWindow* parent);
	~HudOffsetsPrefsPanel();

	void init() override;
	void applyPreferences() override;

private:
	wxCheckBox* cb_hud_bob_;
	wxCheckBox* cb_hud_center_;
	wxCheckBox* cb_hud_statusbar_;
	wxCheckBox* cb_hud_wide_;
};
