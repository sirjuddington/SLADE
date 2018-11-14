#pragma once

#include "PrefsPanelBase.h"

class wxPropertyGrid;

class AdvancedPrefsPanel : public PrefsPanelBase
{
public:
	AdvancedPrefsPanel(wxWindow* parent);
	~AdvancedPrefsPanel();

	void refreshPropGrid();
	void init() override;
	void applyPreferences() override;

private:
	wxPropertyGrid* pg_cvars_;
};
