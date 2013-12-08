
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
	wxListBox*		lb_errors;
	wxButton*		btn_check;
	wxStaticText*	label_status;

public:
	MapChecksPanel(wxWindow* parent, SLADEMap* map);
	~MapChecksPanel();

	void	updateStatusText(string text);
	void	checkMissingTextures();
	void	checkSpecialTags();
	void	checkIntersectingLines();
	void	checkOverlappingLines();

	// Events
	void	onBtnCheck(wxCommandEvent& e);
	void	onListBoxItem(wxCommandEvent& e);
};

#endif//__MAP_CHECKS_DIALOG_H__
