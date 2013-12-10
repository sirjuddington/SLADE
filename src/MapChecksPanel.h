
#ifndef __MAP_CHECKS_DIALOG_H__
#define __MAP_CHECKS_DIALOG_H__

class SLADEMap;
class MapCheck;
class MapChecksPanel : public wxPanel
{
private:
	SLADEMap*					map;
	vector<MapCheck*>	active_checks;

	wxCheckBox*		cb_missing_tex;
	wxCheckBox*		cb_special_tags;
	wxCheckBox*		cb_intersecting;
	wxCheckBox*		cb_overlapping;
	wxCheckBox*		cb_unknown_tex;
	wxCheckBox*		cb_unknown_flats;
	wxCheckBox*		cb_unknown_things;
	wxCheckBox*		cb_overlapping_things;
	wxListBox*		lb_errors;
	wxButton*		btn_check;
	wxStaticText*	label_status;
	wxButton*		btn_fix1;
	wxButton*		btn_fix2;
	wxButton*		btn_edit_object;

	struct check_item_t
	{
		MapCheck* check;
		unsigned index;
		check_item_t(MapCheck* check, unsigned index)
		{
			this->check = check;
			this->index = index;
		}
	};
	vector<check_item_t>	check_items;

public:
	MapChecksPanel(wxWindow* parent, SLADEMap* map);
	~MapChecksPanel();

	void	updateStatusText(string text);
	void	showCheckItem(unsigned index);
	void	refreshList();

	// Events
	void	onBtnCheck(wxCommandEvent& e);
	void	onListBoxItem(wxCommandEvent& e);
	void	onBtnFix1(wxCommandEvent& e);
	void	onBtnFix2(wxCommandEvent& e);
	void	onBtnEditObject(wxCommandEvent& e);
};

#endif//__MAP_CHECKS_DIALOG_H__
