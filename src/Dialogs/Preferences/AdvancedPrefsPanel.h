#pragma once

#include "PrefsPanelBase.h"

class wxPropertyGrid;

class AdvancedPrefsPanel : public PrefsPanelBase
{
public:
	AdvancedPrefsPanel(wxWindow* parent);
	~AdvancedPrefsPanel() = default;

	void refreshPropGrid() const;
	void init() override;
	void applyPreferences() override;

private:
	wxPropertyGrid* pg_cvars_ = nullptr;
};
