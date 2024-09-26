#pragma once

#include "UI/Controls/STabCtrl.h"
#include "UI/SDialog.h"

class wxFlexGridSizer;

namespace slade
{
class ActionSpecialPanel;
class ActionSpecialTreeView;
class ArgsControl;
class ArgsPanel;
class GenLineSpecialPanel;
class NumberTextCtrl;
namespace game
{
	class ActionSpecial;
	struct ArgSpec;
}

class ActionSpecialDialog : public SDialog
{
public:
	ActionSpecialDialog(wxWindow* parent, bool show_args = false);
	~ActionSpecialDialog() override = default;

	void setSpecial(int special) const;
	void setArgs(int args[5]) const;
	int  selectedSpecial() const;
	int  argValue(int index) const;
	void applyTo(const vector<MapObject*>& lines, bool apply_special) const;
	void openLines(const vector<MapObject*>& lines) const;

private:
	ActionSpecialPanel* panel_special_ = nullptr;
	ArgsPanel*          panel_args_    = nullptr;
	TabControl*         stc_tabs_      = nullptr;
};
} // namespace slade
