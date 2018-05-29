#pragma once

#include "UI/SDialog.h"

class PrefsPanelBase;
class BaseResourceArchivesPanel;
class wxPanel;
class wxTreebook;

class PreferencesDialog : public SDialog
{
public:
	PreferencesDialog(wxWindow* parent);
	~PreferencesDialog();

	void	showPage(string name, string subsection = "");
	string	currentPage();
	void	initPages();
	void	applyPreferences();

	// Static functions
	static void	openPreferences(wxWindow* parent, string initial_page = "", string subsection = "");

private:
	wxTreebook*							tree_prefs_;
	std::map<string, PrefsPanelBase*>	prefs_pages_;
	PrefsPanelBase*						prefs_advanced_;

	// Base Resource Archive
	BaseResourceArchivesPanel*	panel_bra_;
	wxButton*					btn_bra_open_;

	// Static
	static string	last_page_;
	static int		width_;
	static int		height_;

	void		addPrefsPage(PrefsPanelBase* page, const string& title, bool sub_page = false, bool select = false);
	wxPanel*	setupBaseResourceArchivesPanel();
	wxPanel*	setupAdvancedPanel();

	// Helper template function for addPrefsPage
	template<class T> void addPrefsPage(const string& title, bool sub_page = false, bool select = false)
	{
		addPrefsPage(new T(tree_prefs_), title, sub_page, select);
	}

	// Events
	void	onBtnBRAOpenClicked(wxCommandEvent& e);
	void	onButtonClicked(wxCommandEvent& e);
};
