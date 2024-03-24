#pragma once

#include "UI/Controls/STabCtrl.h"

namespace slade
{
class SpecialReplacePanel;
class TextureReplacePanel;
class ThingTypeReplacePanel;

class MapReplaceDialog : public wxDialog
{
public:
	MapReplaceDialog(wxWindow* parent = nullptr, Archive* archive = nullptr);
	~MapReplaceDialog() override = default;

private:
	Archive* archive_ = nullptr;

	TabControl*            stc_tabs_      = nullptr;
	ThingTypeReplacePanel* panel_thing_   = nullptr;
	SpecialReplacePanel*   panel_special_ = nullptr;
	TextureReplacePanel*   panel_texture_ = nullptr;
	wxButton*              btn_replace_   = nullptr;
	wxButton*              btn_done_      = nullptr;

	void onBtnReplace(wxCommandEvent& e);
};
} // namespace slade
