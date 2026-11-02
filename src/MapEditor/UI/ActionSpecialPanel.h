#pragma once

namespace slade
{
class ActionSpecialTreeView;
class ArgsPanel;
class GenLineSpecialPanel;
class NumberTextCtrl;

class ActionSpecialPanel : public wxPanel
{
public:
	ActionSpecialPanel(wxWindow* parent, bool trigger = true);
	~ActionSpecialPanel() override = default;

	void setupSpecialPanel();
	void setArgsPanel(ArgsPanel* panel) { panel_args_ = panel; }
	void setSpecial(int special);
	void setTrigger(int index) const;
	void setTrigger(string_view trigger) const;
	void clearTrigger() const;
	int  selectedSpecial() const;
	void showGeneralised(bool show = true);
	void applyTo(const vector<MapObject*>& lines, bool apply_special) const;
	void openLines(const vector<MapObject*>& lines);
	void updateArgsPanel() const;

	void onRadioButtonChanged(wxCommandEvent& e);
	void onSpecialSelectionChanged(wxDataViewEvent& e);
	void onSpecialItemActivated(wxDataViewEvent& e);
	void onSpecialPresetClicked(wxCommandEvent& e);

private:
	ActionSpecialTreeView* tree_specials_        = nullptr;
	wxPanel*               panel_action_special_ = nullptr;
	GenLineSpecialPanel*   panel_gen_specials_   = nullptr;
	wxRadioButton*         rb_special_           = nullptr;
	wxRadioButton*         rb_generalised_       = nullptr;
	ArgsPanel*             panel_args_           = nullptr;
	wxChoice*              choice_trigger_       = nullptr;
	bool                   show_trigger_         = false;
	wxTextCtrl*            text_special_         = nullptr;
	wxButton*              btn_preset_           = nullptr;
	bool                   ignore_select_event_  = false;

	struct FlagHolder
	{
		wxCheckBox* check_box;
		int         index;
		string      udmf;
	};
	vector<FlagHolder> flags_;
};
} // namespace slade
