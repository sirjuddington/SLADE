#pragma once

namespace slade
{
class ListView;

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
} // namespace slade
