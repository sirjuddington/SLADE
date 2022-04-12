#pragma once

#include "UI/SDialog.h"

class wxPanel;
class wxTreebook;

namespace slade
{
class PrefsPanelBase;
class BaseResourceArchivesPanel;

class PreferencesDialog : public SDialog
{
public:
	PreferencesDialog(wxWindow* parent);
	~PreferencesDialog() override = default;

	void     showPage(const wxString& name, const wxString& subsection = "");
	wxString currentPage() const;
	void     initPages() const;
	void     applyPreferences() const;

	// Static functions
	static void openPreferences(wxWindow* parent, wxString initial_page = "", const wxString& subsection = "");

private:
	wxTreebook*                         tree_prefs_ = nullptr;
	std::map<wxString, PrefsPanelBase*> prefs_pages_;
	PrefsPanelBase*                     prefs_advanced_ = nullptr;

	// Base Resource Archive
	BaseResourceArchivesPanel* panel_bra_ = nullptr;

	// Static
	static wxString last_page_;
	static int      width_;
	static int      height_;

	void     addPrefsPage(PrefsPanelBase* page, const wxString& title, bool sub_page = false, bool select = false);
	wxPanel* setupBaseResourceArchivesPanel();
	wxPanel* setupAdvancedPanel();

	// Helper template function for addPrefsPage
	template<class T> void addPrefsPage(const wxString& title, bool sub_page = false, bool select = false)
	{
		addPrefsPage(new T(tree_prefs_), title, sub_page, select);
	}

	// Events
	void onButtonClicked(wxCommandEvent& e);
};
} // namespace slade
