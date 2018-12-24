#pragma once

#include "UI/Lists/ListView.h"
#include "UI/SDialog.h"

class SectorSpecialPanel : public wxPanel
{
public:
	SectorSpecialPanel(wxWindow* parent);
	~SectorSpecialPanel() = default;

	ListView* getSpecialsList() const { return lv_specials_; }

	void setup(int special) const;
	int  selectedSpecial() const;

private:
	ListView*   lv_specials_   = nullptr;
	wxChoice*   choice_damage_ = nullptr;
	wxCheckBox* cb_secret_     = nullptr;
	wxCheckBox* cb_friction_   = nullptr;
	wxCheckBox* cb_pushpull_   = nullptr;
};

class SectorSpecialDialog : public SDialog
{
public:
	SectorSpecialDialog(wxWindow* parent);
	~SectorSpecialDialog() = default;

	void setup(int special) const { panel_special_->setup(special); }
	int  getSelectedSpecial() const { return panel_special_->selectedSpecial(); }

private:
	SectorSpecialPanel* panel_special_ = nullptr;
};
