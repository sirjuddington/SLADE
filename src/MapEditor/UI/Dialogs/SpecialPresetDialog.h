#pragma once

#include "UI/SDialog.h"
#include "Game/SpecialPreset.h"

class SpecialPresetTreeView;

class SpecialPresetDialog : public SDialog
{
public:
	SpecialPresetDialog(wxWindow* parent);

	Game::SpecialPreset	selectedPreset() const;

private:
	SpecialPresetTreeView*	tree_presets_;
};
