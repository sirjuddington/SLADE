#pragma once

#include "UI/Controls/STabCtrl.h"

class Archive;
class ThingTypeReplacePanel : public wxPanel
{
public:
	ThingTypeReplacePanel(wxWindow* parent);
	~ThingTypeReplacePanel();

	void doReplace(Archive* archive);

private:
	wxSpinCtrl* spin_from_;
	wxSpinCtrl* spin_to_;
	wxButton*   btn_browse_from_;
	wxButton*   btn_browse_to_;
};

class SpecialReplacePanel : public wxPanel
{
public:
	SpecialReplacePanel(wxWindow* parent);
	~SpecialReplacePanel();

	void doReplace(Archive* archive);

private:
	wxSpinCtrl* spin_from_;
	wxSpinCtrl* spin_to_;
	wxCheckBox* cb_line_specials_;
	wxCheckBox* cb_thing_specials_;
	wxSpinCtrl* spin_args_from_[5];
	wxSpinCtrl* spin_args_to_[5];
	wxCheckBox* cb_args_[5];
};

class TextureReplacePanel : public wxPanel
{
public:
	TextureReplacePanel(wxWindow* parent);
	~TextureReplacePanel();

	void doReplace(Archive* archive);

private:
	wxTextCtrl* text_from_;
	wxTextCtrl* text_to_;
	wxCheckBox* cb_floor_;
	wxCheckBox* cb_ceiling_;
	wxCheckBox* cb_lower_;
	wxCheckBox* cb_middle_;
	wxCheckBox* cb_upper_;
};

class MapReplaceDialog : public wxDialog
{
public:
	MapReplaceDialog(wxWindow* parent = nullptr, Archive* archive = nullptr);
	~MapReplaceDialog();

private:
	Archive* archive_;

	TabControl*            stc_tabs_;
	ThingTypeReplacePanel* panel_thing_;
	SpecialReplacePanel*   panel_special_;
	TextureReplacePanel*   panel_texture_;
	wxButton*              btn_replace_;
	wxButton*              btn_done_;

	void onBtnDone(wxCommandEvent& e);
	void onBtnReplace(wxCommandEvent& e);
};
