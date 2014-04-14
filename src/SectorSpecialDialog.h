
#ifndef __SECTOR_SPECIAL_DIALOG_H__
#define __SECTOR_SPECIAL_DIALOG_H__

#include "SDialog.h"
#include "ListView.h"

class SectorSpecialPanel : public wxPanel
{
private:
	ListView*		lv_specials;
	wxChoice*		choice_damage;
	wxCheckBox*		cb_secret;
	wxCheckBox*		cb_friction;
	wxCheckBox*		cb_pushpull;

public:
	SectorSpecialPanel(wxWindow* parent);
	~SectorSpecialPanel();

	ListView*	getSpecialsList() { return lv_specials; }

	void	setup(int special, int map_format);
	int		getSelectedSpecial(int map_format);
};

class SectorSpecialDialog : public SDialog
{
private:
	SectorSpecialPanel*	panel_special;

public:
	SectorSpecialDialog(wxWindow* parent);
	~SectorSpecialDialog();

	void	setup(int special, int map_format);
	int		getSelectedSpecial(int map_format);

	// Events
	void	onSpecialsListViewItemActivated(wxListEvent& e);
};

#endif//__SECTOR_SPECIAL_DIALOG_H__
