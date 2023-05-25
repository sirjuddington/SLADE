#pragma once

#include "UI/Lists/ListView.h"
#include "UI/SDialog.h"

namespace slade
{
class SectorSpecialPanel : public wxPanel
{
public:
	SectorSpecialPanel(wxWindow* parent);
	~SectorSpecialPanel() override = default;

	ListView* getSpecialsList() const { return lv_specials_; }

	void setup(int special) const;
	int  selectedSpecial() const;

private:
	ListView*   lv_specials_      = nullptr;
	wxChoice*   choice_damage_    = nullptr;
	wxCheckBox* cb_secret_        = nullptr;
	wxCheckBox* cb_friction_      = nullptr;
	wxCheckBox* cb_pushpull_      = nullptr;
	wxCheckBox* cb_alt_damage_    = nullptr;
	wxCheckBox* cb_kill_grounded_ = nullptr;

	void updateDamageDropdown() const;
};

class SectorSpecialDialog : public SDialog
{
public:
	SectorSpecialDialog(wxWindow* parent);
	~SectorSpecialDialog() override = default;

	void setup(int special) const { panel_special_->setup(special); }
	int  getSelectedSpecial() const { return panel_special_->selectedSpecial(); }

private:
	SectorSpecialPanel* panel_special_ = nullptr;
};
} // namespace slade
