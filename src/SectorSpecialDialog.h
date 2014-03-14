
#ifndef __SECTOR_SPECIAL_DIALOG_H__
#define __SECTOR_SPECIAL_DIALOG_H__

#include "SDialog.h"
#include "ListView.h"

class SectorSpecialDialog : public SDialog
{
private:
	ListView*	lv_specials;
	wxChoice*	choice_damage;
	wxCheckBox*	cb_secret;
	wxCheckBox*	cb_friction;
	wxCheckBox*	cb_pushpull;

public:
	SectorSpecialDialog(wxWindow* parent);
	~SectorSpecialDialog();

	void	setup(int special, int map_format);
	int		getSelectedSpecial(int map_format);

	// Events
	void	onSpecialsListViewItemActivated(wxListEvent& e);
};

#endif//__SECTOR_SPECIAL_DIALOG_H__
