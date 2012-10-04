
#ifndef __PREFERENCES_DIALOG_H__
#define __PREFERENCES_DIALOG_H__

#include <wx/treebook.h>

class PrefsPanelBase;
class BaseResourceArchivesPanel;
class PreferencesDialog : public wxDialog {
private:
	wxTreebook*				tree_prefs;
	vector<PrefsPanelBase*>	prefs_pages;
	PrefsPanelBase*			prefs_advanced;

	// Base Resource Archive
	BaseResourceArchivesPanel*	panel_bra;
	wxButton*					btn_bra_open;

	// Static
	static string	last_page;
	static int		width;
	static int		height;

public:
	PreferencesDialog(wxWindow* parent);
	~PreferencesDialog();

	wxPanel*	setupEditingPrefsPanel();
	wxPanel*	setupBaseResourceArchivesPanel();

	void	showPage(string name);
	string	currentPage();
	void	initPages();
	void	applyPreferences();

	// Events
	void	onBtnBRAOpenClicked(wxCommandEvent& e);
	void	onButtonClicked(wxCommandEvent& e);

	// Static functions
	static void	openPreferences(wxWindow* parent, string initial_page = "");
};

#endif//__PREFERENCES_DIALOG_H__
