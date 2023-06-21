#pragma once

#include "PrefsPanelBase.h"

class wxPropertyGrid;

namespace slade
{
class AdvancedPrefsPanel : public PrefsPanelBase
{
public:
	AdvancedPrefsPanel(wxWindow* parent);
	~AdvancedPrefsPanel() override = default;

	void refreshPropGrid() const;
	void init() override;
	void applyPreferences() override;

private:
	wxPropertyGrid* pg_cvars_ = nullptr;
};
} // namespace slade
