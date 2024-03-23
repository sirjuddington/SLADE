#pragma once

#include "PrefsPanelBase.h"

namespace slade
{
class FileLocationPanel;

class PNGPrefsPanel : public PrefsPanelBase
{
public:
	PNGPrefsPanel(wxWindow* parent);
	~PNGPrefsPanel() override = default;

	void init() override;
	void applyPreferences() override;

	wxString pageTitle() override { return "PNG Optimization Tools"; }

private:
	FileLocationPanel* flp_pngout_   = nullptr;
	FileLocationPanel* flp_pngcrush_ = nullptr;
	FileLocationPanel* flp_deflopt_  = nullptr;
};
} // namespace slade
