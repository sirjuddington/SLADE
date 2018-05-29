#pragma once

#include "PrefsPanelBase.h"

class FileLocationPanel;

class PNGPrefsPanel : public PrefsPanelBase
{
public:
	PNGPrefsPanel(wxWindow* parent);
	~PNGPrefsPanel();

	void	init() override;
	void	applyPreferences() override;

	string pageTitle() override { return "PNG Optimization Tools"; }

private:
	FileLocationPanel*	flp_pngout_;
	FileLocationPanel*	flp_pngcrush_;
	FileLocationPanel*	flp_deflopt_;
};
