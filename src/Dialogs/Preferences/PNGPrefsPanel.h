#pragma once

#include "PrefsPanelBase.h"

class FileLocationPanel;

class PNGPrefsPanel : public PrefsPanelBase
{
public:
	PNGPrefsPanel(wxWindow* parent);
	~PNGPrefsPanel() = default;

	void init() override;
	void applyPreferences() override;

	string pageTitle() override { return "PNG Optimization Tools"; }

private:
	FileLocationPanel* flp_pngout_   = nullptr;
	FileLocationPanel* flp_pngcrush_ = nullptr;
	FileLocationPanel* flp_deflopt_  = nullptr;
};
