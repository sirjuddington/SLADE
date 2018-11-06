
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    EditingPrefsPanel.cpp
// Description: Panel containing editing preference controls
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "EditingPrefsPanel.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Executables.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "UI/Controls/STabCtrl.h"
#include "Utility/SFileDialog.h"
#include "UI/WxUtils.h"


// ----------------------------------------------------------------------------
//
// External Variables
//
// ----------------------------------------------------------------------------
EXTERN_CVAR(Bool, wad_force_uppercase)
EXTERN_CVAR(Int, autosave_entry_changes)
EXTERN_CVAR(Bool, percent_encoding)
EXTERN_CVAR(Bool, auto_entry_replace)
EXTERN_CVAR(Bool, save_archive_with_map)
EXTERN_CVAR(Bool, confirm_entry_delete)
EXTERN_CVAR(Bool, confirm_entry_revert)
EXTERN_CVAR(Int, dir_archive_change_action)


// ----------------------------------------------------------------------------
//
// ExternalEditorList Class
//
// ----------------------------------------------------------------------------
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
		exes_ = Executables::getExternalExes(category);
		SetItemCount(exes_.size());
	}

	string getItemText(long item, long column, long index) const override
	{
		if (item < 0 || item >= (long)exes_.size())
			return "";

		if (column == 0)
			return exes_[item].name;
		if (column == 1)
			return exes_[item].path;

		return "";
	}

private:
	vector<Executables::external_exe_t> exes_;
};


// ----------------------------------------------------------------------------
//
// ExternalEditorDialog Class
//
// ----------------------------------------------------------------------------
class ExternalEditorDialog : public wxDialog
{
public:
	ExternalEditorDialog(wxWindow* parent, bool browse_on_open, string name = "", string path = "")
		: wxDialog(parent, -1, "External Editor"), browse_on_open_(browse_on_open)
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		// Name
		wxGridBagSizer* gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
		sizer->Add(gb_sizer, 1, wxEXPAND | wxALL, UI::padLarge());
		gb_sizer->Add(
			new wxStaticText(this, -1, "Name:"),
			wxGBPosition(0, 0),
			wxDefaultSpan,
			wxALIGN_CENTER_VERTICAL);
		text_name_ = new wxTextCtrl(this, -1, name);
		gb_sizer->Add(text_name_, wxGBPosition(0, 1), wxGBSpan(1, 2), wxEXPAND);

		// Path
		gb_sizer->Add(
			new wxStaticText(this, -1, "Path:"),
			wxGBPosition(1, 0),
			wxDefaultSpan,
			wxALIGN_CENTER_VERTICAL);
		text_path_ = new wxTextCtrl(this, -1, path, wxDefaultPosition, wxSize(UI::scalePx(300), -1));
		gb_sizer->Add(text_path_, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
		btn_browse_ = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "open"));
		gb_sizer->Add(btn_browse_, wxGBPosition(1, 2), wxDefaultSpan);

		// Ok/Cancel
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		gb_sizer->Add(hbox, wxGBPosition(2, 0), wxGBSpan(1, 3), wxEXPAND);
		hbox->AddStretchSpacer();
		btn_cancel_ = new wxButton(this, wxID_CANCEL, "Cancel");
		hbox->Add(btn_cancel_, 0, wxEXPAND | wxRIGHT, UI::pad());
		btn_ok_ = new wxButton(this, wxID_OK, "OK");
		hbox->Add(btn_ok_, 0, wxEXPAND);

		gb_sizer->AddGrowableCol(1);

		btn_browse_->Bind(wxEVT_BUTTON, &ExternalEditorDialog::onBtnBrowseClicked, this);
		Bind(wxEVT_SHOW, &ExternalEditorDialog::onShow, this);

		Fit();
		CenterOnParent();
	}

	~ExternalEditorDialog() {}

	string getName() const { return text_name_->GetValue(); }
	string getPath() const { return text_path_->GetValue(); }

private:
	wxTextCtrl*		text_name_;
	wxTextCtrl*		text_path_;
	wxBitmapButton*	btn_browse_;
	wxButton*		btn_ok_;
	wxButton*		btn_cancel_;
	bool			browse_on_open_;

	void browse()
	{
		SFileDialog::fd_info_t info;
#ifdef WIN32
		if (SFileDialog::openFile(info, "Browse for External Editor", "Executable Files (*.exe)|*.exe", this))
#else
		if (SFileDialog::openFile(info, "Browse for External Editor", wxFileSelectorDefaultWildcardStr, this))
#endif
		{
			text_path_->SetValue(info.filenames[0]);

			if (text_name_->GetValue().IsEmpty())
			{
				wxFileName fn(info.filenames[0]);
				text_name_->SetValue(fn.GetName().Capitalize());
			}
		}
	}

	void onBtnBrowseClicked(wxCommandEvent& e)
	{
		browse();
	}

	void onShow(wxShowEvent& e)
	{
		if (e.IsShown() && browse_on_open_ && text_path_->GetValue().IsEmpty())
			browse();
	}
};


// ----------------------------------------------------------------------------
//
// EditingPrefsPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// EditingPrefsPanel::EditingPrefsPanel
//
// EditingPrefsPanel class constructor
// ----------------------------------------------------------------------------
EditingPrefsPanel::EditingPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Tabs
	stc_tabs_ = STabCtrl::createControl(this);
	sizer->Add(stc_tabs_, 1, wxEXPAND);

	// General
	stc_tabs_->AddPage(setupGeneralTab(), "General", true);

	// External Editors
	stc_tabs_->AddPage(setupExternalTab(), "External Editors", false);

	// Bind events
	choice_category_->Bind(wxEVT_CHOICE, &EditingPrefsPanel::onChoiceCategoryChanged, this);
	btn_add_exe_->Bind(wxEVT_BUTTON, &EditingPrefsPanel::onBtnAddClicked, this);
	btn_remove_exe_->Bind(wxEVT_BUTTON, &EditingPrefsPanel::onBtnRemoveClicked, this);
	lv_ext_editors_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &EditingPrefsPanel::onExternalExeActivated, this);
}

// ----------------------------------------------------------------------------
// EditingPrefsPanel::~EditingPrefsPanel
//
// EditingPrefsPanel class destructor
// ----------------------------------------------------------------------------
EditingPrefsPanel::~EditingPrefsPanel()
{
}

// ----------------------------------------------------------------------------
// EditingPrefsPanel::setupGeneralTab
//
// Creates and returns the 'General' tab
// ----------------------------------------------------------------------------
wxPanel* EditingPrefsPanel::setupGeneralTab()
{
	wxPanel* panel = new wxPanel(stc_tabs_, -1);

	// Create controls
	cb_wad_force_uppercase_ = new wxCheckBox(panel, -1, "Force uppercase entry names in Wad Archives");
	cb_zip_percent_encoding_ = new wxCheckBox(
		panel,
		-1,
		"Use percent encoding if needed outside of Wad Archives"
	);
	cb_auto_entry_replace_ = new wxCheckBox(
		panel,
		-1,
		"Automatically replace entries with same name as drag-and-dropped files"
	);
	cb_save_archive_with_map_ = new wxCheckBox(panel, -1, "Save the map's parent archive when saving a map");
	cb_confirm_entry_delete_ = new wxCheckBox(panel, -1, "Show confirmation dialog on deleting an entry");
	cb_confirm_entry_revert_ = new wxCheckBox(panel, -1, "Show confirmation dialog on reverting entry changes");
	choice_entry_mod_ = new wxChoice(panel, -1);
	choice_dir_mod_ = new wxChoice(panel, -1);

	// Populate dropdowns
	choice_entry_mod_->Set(WxUtils::arrayString({ "Don't Save", "Save", "Ask" }));
	choice_dir_mod_->Set(WxUtils::arrayString({ "Ignore Changes", "Apply Changes", "Ask"}));

	// Layout
	panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	WxUtils::layoutVertically(
		panel->GetSizer(),
		vector<wxObject*>{
			cb_wad_force_uppercase_,
			cb_zip_percent_encoding_,
			cb_auto_entry_replace_,
			cb_save_archive_with_map_,
			cb_confirm_entry_delete_,
			cb_confirm_entry_revert_,
			WxUtils::createLabelHBox(
				panel,
				"Action on unsaved entry changes:",
				choice_entry_mod_
			),
			WxUtils::createLabelHBox(
				panel,
				"Action on external directory changes:",
				choice_dir_mod_
			)
		},
		wxSizerFlags(0).Expand().Border(wxALL, UI::padLarge())
	);

	return panel;
}

// ----------------------------------------------------------------------------
// EditingPrefsPanel::setupExternalTab
//
// Creates and returns the 'External Editors' tab
// ----------------------------------------------------------------------------
wxPanel* EditingPrefsPanel::setupExternalTab()
{
	wxPanel* panel = new wxPanel(stc_tabs_, -1);

	// Create controls
	auto categories = EntryType::allCategories();
	choice_category_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, categories.size(), &categories[0]);
	lv_ext_editors_ = new ExternalEditorList(panel);
	btn_add_exe_ = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "plus"));
	btn_add_exe_->SetToolTip("Add External Editor");
	btn_remove_exe_ = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "minus"));
	btn_remove_exe_->SetToolTip("Remove Selected External Editors");

	// Layout
	panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	auto sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	panel->GetSizer()->Add(sizer, 1, wxEXPAND|wxALL, UI::padLarge());

	sizer->Add(new wxStaticText(panel, -1, "Category: "), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_category_, { 0, 1 }, { 1, 2 }, wxEXPAND);
	sizer->Add(lv_ext_editors_, { 1, 0 }, { 3, 2 }, wxEXPAND);
	sizer->Add(btn_add_exe_, { 1, 2 }, { 1, 1 });
	sizer->Add(btn_remove_exe_, { 2, 2 }, { 1, 1 });

	sizer->AddGrowableRow(3, 1);
	sizer->AddGrowableCol(1, 1);

	return panel;
}

// ----------------------------------------------------------------------------
// EditingPrefsPanel::init
//
// Initialises panel controls
// ----------------------------------------------------------------------------
void EditingPrefsPanel::init()
{
	cb_wad_force_uppercase_->SetValue(wad_force_uppercase);
	cb_zip_percent_encoding_->SetValue(percent_encoding);
	cb_auto_entry_replace_->SetValue(auto_entry_replace);
	cb_save_archive_with_map_->SetValue(save_archive_with_map);
	choice_entry_mod_->SetSelection(autosave_entry_changes);
	cb_confirm_entry_delete_->SetValue(confirm_entry_delete);
	cb_confirm_entry_revert_->SetValue(confirm_entry_revert);
	choice_dir_mod_->SetSelection(dir_archive_change_action);

	choice_category_->SetSelection(0);
	((ExternalEditorList*)lv_ext_editors_)->setCategory(choice_category_->GetStringSelection());
}

// ----------------------------------------------------------------------------
// EditingPrefsPanel::applyPreferences
//
// Applies preference values from the controls to CVARs
// ----------------------------------------------------------------------------
void EditingPrefsPanel::applyPreferences()
{
	wad_force_uppercase = cb_wad_force_uppercase_->GetValue();
	percent_encoding = cb_zip_percent_encoding_->GetValue();
	auto_entry_replace = cb_auto_entry_replace_->GetValue();
	save_archive_with_map = cb_save_archive_with_map_->GetValue();
	autosave_entry_changes = choice_entry_mod_->GetSelection();
	confirm_entry_delete = cb_confirm_entry_delete_->GetValue();
	confirm_entry_revert = cb_confirm_entry_revert_->GetValue();
	dir_archive_change_action = choice_dir_mod_->GetSelection();
}

// ----------------------------------------------------------------------------
// EditingPrefsPanel::showSubSection
//
// Shows [subsection] on the prefs page
// ----------------------------------------------------------------------------
void EditingPrefsPanel::showSubSection(string subsection)
{
	// 'External' tab
	if (subsection == "external")
		stc_tabs_->SetSelection(1);

	// Default to 'General' tab
	else
		stc_tabs_->SetSelection(0);
}


// ----------------------------------------------------------------------------
//
// EditingPrefsPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// EditingPrefsPanel::onChoiceCategoryChanged
//
// Called when the external executables 'Category' dropdown changes
// ----------------------------------------------------------------------------
void EditingPrefsPanel::onChoiceCategoryChanged(wxCommandEvent& e)
{
	((ExternalEditorList*)lv_ext_editors_)->setCategory(choice_category_->GetStringSelection());
}

// ----------------------------------------------------------------------------
// EditingPrefsPanel::onBtnAddClicked
//
// Called when the 'Add' button is clicked
// ----------------------------------------------------------------------------
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
			string category = choice_category_->GetStringSelection();
			Executables::addExternalExe(dlg.getName(), dlg.getPath(), category);

			// Refresh list
			((ExternalEditorList*)lv_ext_editors_)->setCategory(category);

			break;
		}
	}
}

// ----------------------------------------------------------------------------
// EditingPrefsPanel::onBtnRemoveClicked
//
// Called when the 'Remove' button is clicked
// ----------------------------------------------------------------------------
void EditingPrefsPanel::onBtnRemoveClicked(wxCommandEvent& e)
{
	vector<long> selection = lv_ext_editors_->getSelection();
	string category = choice_category_->GetStringSelection();

	// Remove selected editors
	for (unsigned a = 0; a < selection.size(); a++)
	{
		string name = lv_ext_editors_->GetItemText(selection[a]);
		Executables::removeExternalExe(name, category);
	}

	// Refresh list
	((ExternalEditorList*)lv_ext_editors_)->setCategory(category);
}

// ----------------------------------------------------------------------------
// EditingPrefsPanel::onExternalExeActivated
//
// Called when an item in the external editors list is activated
// ----------------------------------------------------------------------------
void EditingPrefsPanel::onExternalExeActivated(wxListEvent& e)
{
	string name = lv_ext_editors_->GetItemText(e.GetIndex());
	string category = choice_category_->GetStringSelection();
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
			((ExternalEditorList*)lv_ext_editors_)->setCategory(category);

			break;
		}
	}
}
