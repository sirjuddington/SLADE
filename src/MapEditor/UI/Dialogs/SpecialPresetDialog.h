#pragma once

#include "Game/SpecialPreset.h"
#include "UI/SDialog.h"

namespace slade
{
class SpecialPresetTreeView;

class SpecialPresetDialog : public SDialog
{
public:
	SpecialPresetDialog(wxWindow* parent);

	game::SpecialPreset selectedPreset() const;

private:
	SpecialPresetTreeView* tree_presets_ = nullptr;
};
} // namespace slade
