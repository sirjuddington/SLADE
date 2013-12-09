
#ifndef __MAP_CHECKS_DIALOG_H__
#define __MAP_CHECKS_DIALOG_H__

class SLADEMap;
class MapObject;
class MapChecksPanel : public wxPanel
{
private:
	SLADEMap*			map;
	vector<MapObject*>	objects;
	vector<fpoint2_t>	positions;

	wxCheckBox*		cb_missing_tex;
	wxCheckBox*		cb_special_tags;
	wxCheckBox*		cb_intersecting;
	wxCheckBox*		cb_overlapping;
	wxCheckBox*		cb_unknown_tex;
	wxCheckBox*		cb_unknown_flats;
	wxCheckBox*		cb_overlapping_things;
	wxListBox*		lb_errors;
	wxButton*		btn_check;
	wxStaticText*	label_status;
	wxButton*		btn_fix1;
	wxButton*		btn_fix2;
	wxButton*		btn_edit_object;

public:
	MapChecksPanel(wxWindow* parent, SLADEMap* map);
	~MapChecksPanel();

	void	updateStatusText(string text);
	void	checkMissingTextures();
	void	checkSpecialTags();
	void	checkIntersectingLines();
	void	checkOverlappingLines();
	void	checkUnknownTextures();
	void	checkUnknownFlats();
	void	checkOverlappingThings();

	// Events
	void	onBtnCheck(wxCommandEvent& e);
	void	onListBoxItem(wxCommandEvent& e);
	void	onBtnFix1(wxCommandEvent& e);
	void	onBtnFix2(wxCommandEvent& e);
	void	onBtnEditObject(wxCommandEvent& e);
};

#endif//__MAP_CHECKS_DIALOG_H__
