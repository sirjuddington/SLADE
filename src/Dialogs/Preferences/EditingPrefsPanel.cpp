
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    EditingPrefsPanel.cpp
 * Description: Panel containing editing preference controls
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "EditingPrefsPanel.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Executables.h"
#include "Graphics/Icons.h"
#include "UI/STabCtrl.h"
#include "Utility/SFileDialog.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, wad_force_uppercase)
EXTERN_CVAR(Int, autosave_entry_changes)
EXTERN_CVAR(Bool, percent_encoding)
EXTERN_CVAR(Bool, auto_entry_replace)
EXTERN_CVAR(Bool, save_archive_with_map)
EXTERN_CVAR(Bool, confirm_entry_delete)
EXTERN_CVAR(Bool, confirm_entry_revert)


/*******************************************************************
 * EXTERNALEDITORLIST CLASS
 *******************************************************************/
class ExternalEditorList : public VirtualListView
{
public:
	ExternalEditorList(wxWindow* parent)
		: VirtualListView(parent)
	{
		AppendColumn("Name");
		AppendColumn("Path");
	}

	~ExternalEditorList() {}

	void setCategory(string category)
	{
		exes = Executables::getExternalExes(category);
		SetItemCount(exes.size());
	}

	string getItemText(long item, long column, long index) const
	{
		if (item < 0 || item >= (long)exes.size())
			return "";

		if (column == 0)
			return exes[item].name;
		if (column == 1)
			return exes[item].path;

		return "";
	}

private:
	vector<Executables::external_exe_t> exes;
};


/*******************************************************************
 * EXTERNALEDITORDIALOG CLASS
 *******************************************************************/
class ExternalEditorDialog : public wxDialog
{
public:
	ExternalEditorDialog(wxWindow* parent, bool browse_on_open, string name = "", string path = "")
		: wxDialog(parent, -1, "External Editor"), browse_on_open(browse_on_open)
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		// Name
		wxGridBagSizer* gb_sizer = new wxGridBagSizer(8, 8);
		sizer->Add(gb_sizer, 1, wxEXPAND | wxALL, 10);
		gb_sizer->Add(
			new wxStaticText(this, -1, "Name:"),
			wxGBPosition(0, 0),
			wxDefaultSpan,
			wxALIGN_CENTER_VERTICAL);
		text_name = new wxTextCtrl(this, -1, name);
		gb_sizer->Add(text_name, wxGBPosition(0, 1), wxGBSpan(1, 2), wxEXPAND);

		// Path
		gb_sizer->Add(
			new wxStaticText(this, -1, "Path:"),
			wxGBPosition(1, 0),
			wxDefaultSpan,
			wxALIGN_CENTER_VERTICAL);
		text_path = new wxTextCtrl(this, -1, path, wxDefaultPosition, wxSize(300, -1));
		gb_sizer->Add(text_path, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
		btn_browse = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "open"));
		gb_sizer->Add(btn_browse, wxGBPosition(1, 2), wxDefaultSpan);

		// Ok/Cancel
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		gb_sizer->Add(hbox, wxGBPosition(2, 0), wxGBSpan(1, 3), wxEXPAND);
		hbox->AddStretchSpacer();
		btn_cancel = new wxButton(this, wxID_CANCEL, "Cancel");
		hbox->Add(btn_cancel, 0, wxEXPAND | wxRIGHT, 8);
		btn_ok = new wxButton(this, wxID_OK, "OK");
		hbox->Add(btn_ok, 0, wxEXPAND);

		gb_sizer->AddGrowableCol(1);

		btn_browse->Bind(wxEVT_BUTTON, &ExternalEditorDialog::onBtnBrowseClicked, this);
		Bind(wxEVT_SHOW, &ExternalEditorDialog::onShow, this);

		Fit();
		CenterOnParent();
	}

	~ExternalEditorDialog() {}

	string getName() { return text_name->GetValue(); }
	string getPath() { return text_path->GetValue(); }

private:
	wxTextCtrl*		text_name;
	wxTextCtrl*		text_path;
	wxBitmapButton*	btn_browse;
	wxButton*		btn_ok;
	wxButton*		btn_cancel;
	bool			browse_on_open;

	void browse()
	{
		SFileDialog::fd_info_t info;
#ifdef WIN32
		if (SFileDialog::openFile(info, "Browse for External Editor", "Executable Files (*.exe)|*.exe", this))
#else
		if (SFileDialog::openFile(info, "Browse for External Editor", wxFileSelectorDefaultWildcardStr, this))
#endif
		{
			text_path->SetValue(info.filenames[0]);

			if (text_name->GetValue().IsEmpty())
			{
				wxFileName fn(info.filenames[0]);
				text_name->SetValue(fn.GetName().Capitalize());
			}
		}
	}

	void onBtnBrowseClicked(wxCommandEvent& e)
	{
		browse();
	}

	void onShow(wxShowEvent& e)
	{
		if (e.IsShown() && browse_on_open && text_path->GetValue().IsEmpty())
			browse();
	}
};


/*******************************************************************
 * EDITINGPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* EditingPrefsPanel::EditingPrefsPanel
 * EditingPrefsPanel class constructor
 *******************************************************************/
EditingPrefsPanel::EditingPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Editing Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Tabs
	stc_tabs = STabCtrl::createControl(this);
	sizer->Add(stc_tabs, 1, wxEXPAND | wxALL, 8);

	// General
	stc_tabs->AddPage(setupGeneralTab(), "General", true);

	// External Editors
	stc_tabs->AddPage(setupExternalTab(), "External Editors", false);

	// Bind events
	choice_category->Bind(wxEVT_CHOICE, &EditingPrefsPanel::onChoiceCategoryChanged, this);
	btn_add_exe->Bind(wxEVT_BUTTON, &EditingPrefsPanel::onBtnAddClicked, this);
	btn_remove_exe->Bind(wxEVT_BUTTON, &EditingPrefsPanel::onBtnRemoveClicked, this);
	lv_ext_editors->Bind(wxEVT_LIST_ITEM_ACTIVATED, &EditingPrefsPanel::onExternalExeActivated, this);
}

/* EditingPrefsPanel::~EditingPrefsPanel
 * EditingPrefsPanel class destructor
 *******************************************************************/
EditingPrefsPanel::~EditingPrefsPanel()
{
}

/* EditingPrefsPanel::setupGeneralTab
 * Creates and returns the 'General' tab
 *******************************************************************/
wxPanel* EditingPrefsPanel::setupGeneralTab()
{
	wxPanel* panel = new wxPanel(stc_tabs, -1);
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(psizer);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND | wxALL, 4);

	// Force uppercase
	cb_wad_force_uppercase = new wxCheckBox(panel, -1, "Force uppercase entry names in Wad Archives");
	sizer->Add(cb_wad_force_uppercase, 0, wxEXPAND | wxALL, 4);

	// Percent encoding
	cb_zip_percent_encoding = new wxCheckBox(panel, -1, "Use percent encoding if needed outside of Wad Archives");
	sizer->Add(cb_zip_percent_encoding, 0, wxEXPAND | wxALL, 4);

	// Automatically replace entries
	cb_auto_entry_replace = new wxCheckBox(panel, -1, "Automatically replace entries with same name as drag-and-dropped files");
	sizer->Add(cb_auto_entry_replace, 0, wxEXPAND | wxALL, 4);

	// Save archive when saving map
	cb_save_archive_with_map = new wxCheckBox(panel, -1, "Save the map's parent archive when saving a map");
	sizer->Add(cb_save_archive_with_map, 0, wxEXPAND | wxALL, 4);

	// Confirm entry delete
	cb_confirm_entry_delete = new wxCheckBox(panel, -1, "Show confirmation dialog on deleting an entry");
	sizer->Add(cb_confirm_entry_delete, 0, wxEXPAND | wxALL, 4);

	// Confirm entry revert
	cb_confirm_entry_revert = new wxCheckBox(panel, -1, "Show confirmation dialog on reverting entry changes");
	sizer->Add(cb_confirm_entry_revert, 0, wxEXPAND | wxALL, 4);

	// Unsaved entry changes
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxALL, 4);
	string choices[] = { "Don't Save", "Save", "Ask" };
	choice_entry_mod = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, choices);
	hbox->Add(new wxStaticText(panel, -1, "Action on unsaved entry changes:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	hbox->Add(choice_entry_mod, 1, wxEXPAND, 0);

	return panel;
}

/* EditingPrefsPanel::setupExternalTab
 * Creates and returns the 'External Editors' tab
 *******************************************************************/
wxPanel* EditingPrefsPanel::setupExternalTab()
{
	wxPanel* panel = new wxPanel(stc_tabs, -1);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Category
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxALL, 8);
	vector<string> categories = EntryType::allCategories();
	choice_category = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, categories.size(), &categories[0]);
	hbox->Add(new wxStaticText(panel, -1, "Category: "), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	hbox->Add(choice_category, 1, wxEXPAND);

	// List
	lv_ext_editors = new ExternalEditorList(panel);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
	hbox->Add(lv_ext_editors, 1, wxEXPAND | wxRIGHT, 4);

	// Add button
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	hbox->Add(vbox, 0, wxEXPAND);
	btn_add_exe = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "plus"));
	btn_add_exe->SetToolTip("Add External Editor");
	vbox->Add(btn_add_exe, 0, wxBOTTOM, 4);

	// Remove button
	btn_remove_exe = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "minus"));
	btn_remove_exe->SetToolTip("Remove Selected External Editors");
	vbox->Add(btn_remove_exe, 0, wxBOTTOM, 4);

	return panel;
}

/* EditingPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void EditingPrefsPanel::init()
{
	cb_wad_force_uppercase->SetValue(wad_force_uppercase);
	cb_zip_percent_encoding->SetValue(percent_encoding);
	cb_auto_entry_replace->SetValue(auto_entry_replace);
	cb_save_archive_with_map->SetValue(save_archive_with_map);
	choice_entry_mod->SetSelection(autosave_entry_changes);
	cb_confirm_entry_delete->SetValue(confirm_entry_delete);
	cb_confirm_entry_revert->SetValue(confirm_entry_revert);

	choice_category->SetSelection(0);
	((ExternalEditorList*)lv_ext_editors)->setCategory(choice_category->GetStringSelection());
}

/* EditingPrefsPanel::applyPreferences
 * Applies preference values from the controls to CVARs
 *******************************************************************/
void EditingPrefsPanel::applyPreferences()
{
	wad_force_uppercase = cb_wad_force_uppercase->GetValue();
	percent_encoding = cb_zip_percent_encoding->GetValue();
	auto_entry_replace = cb_auto_entry_replace->GetValue();
	save_archive_with_map = cb_save_archive_with_map->GetValue();
	autosave_entry_changes = choice_entry_mod->GetSelection();
	confirm_entry_delete = cb_confirm_entry_delete->GetValue();
	confirm_entry_revert = cb_confirm_entry_revert->GetValue();
}

/* EditingPrefsPanel::showSubSection
 * Shows [subsection] on the prefs page
 *******************************************************************/
void EditingPrefsPanel::showSubSection(string subsection)
{
	// 'External' tab
	if (subsection == "external")
		stc_tabs->SetSelection(1);

	// Default to 'General' tab
	else
		stc_tabs->SetSelection(0);
}


/*******************************************************************
 * EDITINGPREFSPANEL CLASS EVENTS
 *******************************************************************/

/* EditingPrefsPanel::onChoiceCategoryChanged
 * Called when the external executables 'Category' dropdown changes
 *******************************************************************/
void EditingPrefsPanel::onChoiceCategoryChanged(wxCommandEvent& e)
{
	((ExternalEditorList*)lv_ext_editors)->setCategory(choice_category->GetStringSelection());
}

/* EditingPrefsPanel::onBtnAddClicked
 * Called when the 'Add' button is clicked
 *******************************************************************/
void EditingPrefsPanel::onBtnAddClicked(wxCommandEvent& e)
{
	ExternalEditorDialog dlg(this, true);
	while (dlg.ShowModal() == wxID_OK)
	{
		if (dlg.getName() == "")
			wxMessageBox("Please enter a name for the editor", "Name Required");
		else if (dlg.getPath() == "")
			wxMessageBox("Please enter or select an executable", "Path Required");
		else
		{
			// Add executable
			string category = choice_category->GetStringSelection();
			Executables::addExternalExe(dlg.getName(), dlg.getPath(), category);

			// Refresh list
			((ExternalEditorList*)lv_ext_editors)->setCategory(category);

			break;
		}
	}
}

/* EditingPrefsPanel::onBtnRemoveClicked
 * Called when the 'Remove' button is clicked
 *******************************************************************/
void EditingPrefsPanel::onBtnRemoveClicked(wxCommandEvent& e)
{
	vector<long> selection = lv_ext_editors->getSelection();
	string category = choice_category->GetStringSelection();

	// Remove selected editors
	for (unsigned a = 0; a < selection.size(); a++)
	{
		string name = lv_ext_editors->GetItemText(selection[a]);
		Executables::removeExternalExe(name, category);
	}

	// Refresh list
	((ExternalEditorList*)lv_ext_editors)->setCategory(category);
}

/* EditingPrefsPanel::onExternalExeActivated
 * Called when an item in the external editors list is activated
 *******************************************************************/
void EditingPrefsPanel::onExternalExeActivated(wxListEvent& e)
{
	string name = lv_ext_editors->GetItemText(e.GetIndex());
	string category = choice_category->GetStringSelection();
	Executables::external_exe_t exe = Executables::getExternalExe(name, category);

	ExternalEditorDialog dlg(this, false, name, exe.path);
	while (dlg.ShowModal() == wxID_OK)
	{
		if (dlg.getName() == "")
			wxMessageBox("Please enter a name for the editor", "Name Required");
		else if (dlg.getPath() == "")
			wxMessageBox("Please enter or select an executable", "Path Required");
		else
		{
			// Update executable
			Executables::setExternalExeName(name, dlg.getName(), category);
			Executables::setExternalExePath(dlg.getName(), dlg.getPath(), category);

			// Refresh list
			((ExternalEditorList*)lv_ext_editors)->setCategory(category);

			break;
		}
	}
}
