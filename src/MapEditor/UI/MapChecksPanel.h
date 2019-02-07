#pragma once

#include "UI/Controls/DockPanel.h"

class SLADEMap;
class MapCheck;
class wxListBox;

class MapChecksPanel : public DockPanel
{
public:
	MapChecksPanel(wxWindow* parent, SLADEMap* map);
	~MapChecksPanel();

	void updateStatusText(const wxString& text);
	void showCheckItem(unsigned index);
	void refreshList();
	void reset();

	// DockPanel overrides
	void layoutNormal() override { layoutHorizontal(); }
	void layoutVertical() override;
	void layoutHorizontal() override;

private:
	SLADEMap*                         map_ = nullptr;
	vector<std::unique_ptr<MapCheck>> active_checks_;

	wxCheckListBox* clb_active_checks_ = nullptr;
	wxListBox*      lb_errors_         = nullptr;
	wxButton*       btn_check_         = nullptr;
	wxStaticText*   label_status_      = nullptr;
	wxButton*       btn_fix1_          = nullptr;
	wxButton*       btn_fix2_          = nullptr;
	wxButton*       btn_edit_object_   = nullptr;
	wxButton*       btn_export_        = nullptr;

	struct CheckItem
	{
		MapCheck* check;
		unsigned  index;
		CheckItem(MapCheck* check, unsigned index) : check{ check }, index{ index } {}
	};
	vector<CheckItem> check_items_;

	// Events
	void onBtnCheck(wxCommandEvent& e);
	void onListBoxItem(wxCommandEvent& e);
	void onBtnFix1(wxCommandEvent& e);
	void onBtnFix2(wxCommandEvent& e);
	void onBtnEditObject(wxCommandEvent& e);
	void onBtnExport(wxCommandEvent& e);
};
