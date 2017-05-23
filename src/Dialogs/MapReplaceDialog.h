
#ifndef __MAP_REPLACE_DIALOG_H__
#define __MAP_REPLACE_DIALOG_H__

#include "common.h"
#include "UI/STabCtrl.h"
#include "UI/WxBasicControls.h"


class Archive;
class ThingTypeReplacePanel : public wxPanel
{
private:
	wxSpinCtrl*	spin_from;
	wxSpinCtrl*	spin_to;
	wxButton*	btn_browse_from;
	wxButton*	btn_browse_to;

public:
	ThingTypeReplacePanel(wxWindow* parent);
	~ThingTypeReplacePanel();

	void	doReplace(Archive* archive);
};

class SpecialReplacePanel : public wxPanel
{
private:
	wxSpinCtrl* spin_from;
	wxSpinCtrl*	spin_to;
	wxCheckBox*	cb_line_specials;
	wxCheckBox*	cb_thing_specials;
	wxSpinCtrl*	spin_args_from[5];
	wxSpinCtrl*	spin_args_to[5];
	wxCheckBox*	cb_args[5];

public:
	SpecialReplacePanel(wxWindow* parent);
	~SpecialReplacePanel();

	void doReplace(Archive* archive);
};

class TextureReplacePanel : public wxPanel
{
private:
	wxTextCtrl*	text_from;
	wxTextCtrl*	text_to;
	wxCheckBox*	cb_floor;
	wxCheckBox*	cb_ceiling;
	wxCheckBox* cb_lower;
	wxCheckBox*	cb_middle;
	wxCheckBox*	cb_upper;

public:
	TextureReplacePanel(wxWindow* parent);
	~TextureReplacePanel();

	void doReplace(Archive* archive);
};

class MapReplaceDialog : public wxDialog
{
private:
	Archive*	archive;

	TabControl*				stc_tabs;
	ThingTypeReplacePanel*	panel_thing;
	SpecialReplacePanel*	panel_special;
	TextureReplacePanel*	panel_texture;
	wxButton*				btn_replace;
	wxButton*				btn_done;

public:
	MapReplaceDialog(wxWindow* parent = NULL, Archive* archive = NULL);
	~MapReplaceDialog();

	void	onBtnDone(wxCommandEvent& e);
	void	onBtnReplace(wxCommandEvent& e);
};

#endif//__MAP_REPLACE_DIALOG_H__
