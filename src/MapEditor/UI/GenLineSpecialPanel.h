#pragma once

class wxChoice;
class wxStaticText;
class wxGridBagSizer;

namespace slade
{
class GenLineSpecialPanel : public wxPanel
{
public:
	GenLineSpecialPanel(wxWindow* parent);
	~GenLineSpecialPanel() = default;

	void setupForType(int type);
	void setProp(int prop, int value);
	bool loadSpecial(int special);
	int  special() const;

private:
	wxChoice*       choice_type_     = nullptr;
	wxChoice*       choice_props_[7] = {};
	wxStaticText*   label_props_[7]  = {};
	wxGridBagSizer* gb_sizer_        = nullptr;

	// Events
	void onChoiceTypeChanged(wxCommandEvent& e);
	void onChoicePropertyChanged(wxCommandEvent& e);
};
} // namespace slade
