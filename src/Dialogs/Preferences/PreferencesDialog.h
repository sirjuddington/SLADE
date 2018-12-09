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
	~PreferencesDialog() = default;

	void   showPage(const string& name, const string& subsection = "");
	string currentPage() const;
	void   initPages();
	void   applyPreferences();

	// Static functions
	static void openPreferences(wxWindow* parent, string initial_page = "", const string& subsection = "");

private:
	wxTreebook*                       tree_prefs_ = nullptr;
	std::map<string, PrefsPanelBase*> prefs_pages_;
	PrefsPanelBase*                   prefs_advanced_ = nullptr;

	// Base Resource Archive
	BaseResourceArchivesPanel* panel_bra_ = nullptr;

	// Static
	static string last_page_;
	static int    width_;
	static int    height_;

	void     addPrefsPage(PrefsPanelBase* page, const string& title, bool sub_page = false, bool select = false);
	wxPanel* setupBaseResourceArchivesPanel();
	wxPanel* setupAdvancedPanel();

	// Helper template function for addPrefsPage
	template<class T> void addPrefsPage(const string& title, bool sub_page = false, bool select = false)
	{
		addPrefsPage(new T(tree_prefs_), title, sub_page, select);
	}

	// Events
	void onButtonClicked(wxCommandEvent& e);
};
