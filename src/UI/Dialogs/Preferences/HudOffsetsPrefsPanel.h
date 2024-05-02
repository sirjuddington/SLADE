#pragma once

#include "PrefsPanelBase.h"

namespace slade
{
class HudOffsetsPrefsPanel : public PrefsPanelBase
{
public:
	HudOffsetsPrefsPanel(wxWindow* parent);
	~HudOffsetsPrefsPanel() override = default;

	void init() override;
	void applyPreferences() override;

private:
	wxCheckBox* cb_hud_bob_       = nullptr;
	wxCheckBox* cb_hud_center_    = nullptr;
	wxCheckBox* cb_hud_statusbar_ = nullptr;
	wxCheckBox* cb_hud_wide_      = nullptr;
};
} // namespace slade
