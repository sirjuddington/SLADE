#pragma once

#include "UI/Controls/STabCtrl.h"

namespace slade
{
class Archive;

class ThingTypeReplacePanel : public wxPanel
{
public:
	ThingTypeReplacePanel(wxWindow* parent);
	~ThingTypeReplacePanel() override = default;

	void doReplace(Archive* archive) const;

private:
	wxSpinCtrl* spin_from_ = nullptr;
	wxSpinCtrl* spin_to_   = nullptr;
};

class SpecialReplacePanel : public wxPanel
{
public:
	SpecialReplacePanel(wxWindow* parent);
	~SpecialReplacePanel() override = default;

	void doReplace(Archive* archive) const;

private:
	wxSpinCtrl* spin_from_         = nullptr;
	wxSpinCtrl* spin_to_           = nullptr;
	wxCheckBox* cb_line_specials_  = nullptr;
	wxCheckBox* cb_thing_specials_ = nullptr;
	wxSpinCtrl* spin_args_from_[5] = {};
	wxSpinCtrl* spin_args_to_[5]   = {};
	wxCheckBox* cb_args_[5]        = {};
};

class TextureReplacePanel : public wxPanel
{
public:
	TextureReplacePanel(wxWindow* parent);
	~TextureReplacePanel() override = default;

	void doReplace(Archive* archive) const;

private:
	wxTextCtrl* text_from_  = nullptr;
	wxTextCtrl* text_to_    = nullptr;
	wxCheckBox* cb_floor_   = nullptr;
	wxCheckBox* cb_ceiling_ = nullptr;
	wxCheckBox* cb_lower_   = nullptr;
	wxCheckBox* cb_middle_  = nullptr;
	wxCheckBox* cb_upper_   = nullptr;
};

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
