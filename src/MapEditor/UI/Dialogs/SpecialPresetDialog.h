#pragma once

#include "UI/SDialog.h"

namespace slade
{
class SpecialPresetTreeView;
namespace game
{
	struct SpecialPreset;
}

class SpecialPresetDialog : public SDialog
{
public:
	SpecialPresetDialog(wxWindow* parent);

	game::SpecialPreset selectedPreset() const;

private:
	SpecialPresetTreeView* tree_presets_ = nullptr;
};
} // namespace slade
