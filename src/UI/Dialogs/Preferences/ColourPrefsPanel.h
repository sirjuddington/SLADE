#pragma once

#include "PrefsPanelBase.h"

class wxPropertyGrid;

namespace slade
{
class ColourPrefsPanel : public PrefsPanelBase
{
public:
	ColourPrefsPanel(wxWindow* parent);
	~ColourPrefsPanel() = default;

	void   init() override;
	void   refreshPropGrid() const;
	void   applyPreferences() override;
	string pageTitle() override { return "Colours && Theme"; }

private:
	wxChoice*       choice_configs_ = nullptr;
	wxPropertyGrid* pg_colours_     = nullptr;
};
} // namespace slade
