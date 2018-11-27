#pragma once

#include "Game/SpecialPreset.h"
#include "UI/SDialog.h"

class SpecialPresetTreeView;

class SpecialPresetDialog : public SDialog
{
public:
	SpecialPresetDialog(wxWindow* parent);

	Game::SpecialPreset selectedPreset() const;

private:
	SpecialPresetTreeView* tree_presets_;
};
