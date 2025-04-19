
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "EditingPrefsPanel.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Executables.h"
#include "General/UI.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, wad_force_uppercase)
EXTERN_CVAR(Int, autosave_entry_changes)
EXTERN_CVAR(Bool, percent_encoding)
EXTERN_CVAR(Bool, auto_entry_replace)
EXTERN_CVAR(Bool, save_archive_with_map)
EXTERN_CVAR(Bool, confirm_entry_delete)
EXTERN_CVAR(Bool, confirm_entry_revert)
EXTERN_CVAR(Int, dir_archive_change_action)


// -----------------------------------------------------------------------------
//
// ExternalEditorList Class
//
// -----------------------------------------------------------------------------
class ExternalEditorList : public VirtualListView
{
public:
	ExternalEditorList(wxWindow* parent) : VirtualListView(parent)
	{
		AppendColumn(wxS("Name"));
		AppendColumn(wxS("Path"));
	}

	~ExternalEditorList() override = default;

	void setCategory(string_view category)
	{
		exes_ = executables::externalExes(category);
		SetItemCount(exes_.size());
	}

	string itemText(long item, long column, long index) const override
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
	vector<executables::ExternalExe> exes_;
};


// -----------------------------------------------------------------------------
//
// ExternalEditorDialog Class
//
// -----------------------------------------------------------------------------
class ExternalEditorDialog : public wxDialog
{
public:
	ExternalEditorDialog(wxWindow* parent, bool browse_on_open, const string& name = "", const string& path = "") :
		wxDialog(parent, -1, wxS("External Editor")),
		browse_on_open_(browse_on_open)
	{
		auto sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		// Name
		auto gb_sizer = new wxGridBagSizer(ui::pad(), ui::pad());
		sizer->Add(gb_sizer, 1, wxEXPAND | wxALL, ui::padLarge());
		gb_sizer->Add(new wxStaticText(this, -1, wxS("Name:")), { 0, 0 }, wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		text_name_ = new wxTextCtrl(this, -1, wxString::FromUTF8(name));
		gb_sizer->Add(text_name_, { 0, 1 }, { 1, 2 }, wxEXPAND);

		// Path
		gb_sizer->Add(new wxStaticText(this, -1, wxS("Path:")), { 1, 0 }, wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		text_path_ = new wxTextCtrl(
			this, -1, wxString::FromUTF8(path), wxDefaultPosition, wxSize(ui::scalePx(300), -1));
		gb_sizer->Add(text_path_, { 1, 1 }, wxDefaultSpan, wxEXPAND);
		btn_browse_ = new SIconButton(this, icons::General, "open");
		gb_sizer->Add(btn_browse_, { 1, 2 }, wxDefaultSpan);

		// Ok/Cancel
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		gb_sizer->Add(hbox, { 2, 0 }, { 1, 3 }, wxEXPAND);
		hbox->AddStretchSpacer();
		btn_cancel_ = new wxButton(this, wxID_CANCEL, wxS("Cancel"));
		hbox->Add(btn_cancel_, 0, wxEXPAND | wxRIGHT, ui::pad());
		btn_ok_ = new wxButton(this, wxID_OK, wxS("OK"));
		hbox->Add(btn_ok_, 0, wxEXPAND);

		gb_sizer->AddGrowableCol(1);

		// Browse button click
		btn_browse_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { browse(); });

		// Show event
		Bind(
			wxEVT_SHOW,
			[&](wxShowEvent& e)
			{
				if (e.IsShown() && browse_on_open_ && text_path_->GetValue().IsEmpty())
					browse();
			});

		wxWindowBase::Fit();
		CenterOnParent();
	}

	~ExternalEditorDialog() override = default;

	string getName() const { return text_name_->GetValue().utf8_string(); }
	string getPath() const { return text_path_->GetValue().utf8_string(); }

private:
	wxTextCtrl*     text_name_;
	wxTextCtrl*     text_path_;
	wxBitmapButton* btn_browse_;
	wxButton*       btn_ok_;
	wxButton*       btn_cancel_;
	bool            browse_on_open_;

	void browse()
	{
		filedialog::FDInfo info;
#ifdef WIN32
		if (filedialog::openFile(info, "Browse for External Editor", "Executable Files (*.exe)|*.exe", this))
#else
		if (filedialog::openFile(info, "Browse for External Editor", wxFileSelectorDefaultWildcardStr, this))
#endif
		{
			text_path_->SetValue(wxString::FromUTF8(info.filenames[0]));

			if (text_name_->GetValue().IsEmpty())
			{
				wxFileName fn(wxString::FromUTF8(info.filenames[0]));
				text_name_->SetValue(fn.GetName().Capitalize());
			}
		}
	}
};


// -----------------------------------------------------------------------------
//
// EditingPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// EditingPrefsPanel class constructor
// -----------------------------------------------------------------------------
EditingPrefsPanel::EditingPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Tabs
	stc_tabs_ = STabCtrl::createControl(this);
	sizer->Add(stc_tabs_, 1, wxEXPAND);

	// General
	stc_tabs_->AddPage(setupGeneralTab(), wxS("General"), true);

	// External Editors
	stc_tabs_->AddPage(setupExternalTab(), wxS("External Editors"), false);

	// Bind events
	choice_category_->Bind(
		wxEVT_CHOICE,
		[&](wxCommandEvent&)
		{ ((ExternalEditorList*)lv_ext_editors_)->setCategory(choice_category_->GetStringSelection().utf8_string()); });
	btn_add_exe_->Bind(wxEVT_BUTTON, &EditingPrefsPanel::onBtnAddClicked, this);
	btn_remove_exe_->Bind(wxEVT_BUTTON, &EditingPrefsPanel::onBtnRemoveClicked, this);
	lv_ext_editors_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &EditingPrefsPanel::onExternalExeActivated, this);
}

// -----------------------------------------------------------------------------
// Creates and returns the 'General' tab
// -----------------------------------------------------------------------------
wxPanel* EditingPrefsPanel::setupGeneralTab()
{
	auto panel = new wxPanel(stc_tabs_, -1);

	// Create controls
	cb_wad_force_uppercase_  = new wxCheckBox(panel, -1, wxS("Force uppercase entry names in Wad Archives"));
	cb_zip_percent_encoding_ = new wxCheckBox(panel, -1, wxS("Use percent encoding if needed outside of Wad Archives"));
	cb_auto_entry_replace_   = new wxCheckBox(
        panel, -1, wxS("Automatically replace entries with same name as drag-and-dropped files"));
	cb_save_archive_with_map_ = new wxCheckBox(panel, -1, wxS("Save the map's parent archive when saving a map"));
	cb_confirm_entry_delete_  = new wxCheckBox(panel, -1, wxS("Show confirmation dialog on deleting an entry"));
	cb_confirm_entry_revert_  = new wxCheckBox(panel, -1, wxS("Show confirmation dialog on reverting entry changes"));
	choice_entry_mod_         = new wxChoice(panel, -1);
	choice_dir_mod_           = new wxChoice(panel, -1);

	// Populate dropdowns
	choice_entry_mod_->Set(wxutil::arrayStringStd({ "Don't Save", "Save", "Ask" }));
	choice_dir_mod_->Set(wxutil::arrayStringStd({ "Ignore Changes", "Apply Changes", "Ask" }));

	// Layout
	panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	wxutil::layoutVertically(
		panel->GetSizer(),
		vector<wxObject*>{ cb_wad_force_uppercase_,
						   cb_zip_percent_encoding_,
						   cb_auto_entry_replace_,
						   cb_save_archive_with_map_,
						   cb_confirm_entry_delete_,
						   cb_confirm_entry_revert_,
						   wxutil::createLabelHBox(panel, "Action on unsaved entry changes:", choice_entry_mod_),
						   wxutil::createLabelHBox(panel, "Action on external directory changes:", choice_dir_mod_) },
		wxSizerFlags(0).Expand().Border(wxALL, ui::padLarge()));

	return panel;
}

// -----------------------------------------------------------------------------
// Creates and returns the 'External Editors' tab
// -----------------------------------------------------------------------------
wxPanel* EditingPrefsPanel::setupExternalTab()
{
	auto panel = new wxPanel(stc_tabs_, -1);

	// Create controls
	auto categories  = wxutil::arrayStringStd(EntryType::allCategories());
	choice_category_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, categories);
	lv_ext_editors_  = new ExternalEditorList(panel);
	btn_add_exe_     = new SIconButton(panel, icons::General, "plus");
	btn_add_exe_->SetToolTip(wxS("Add External Editor"));
	btn_remove_exe_ = new SIconButton(panel, icons::General, "minus");
	btn_remove_exe_->SetToolTip(wxS("Remove Selected External Editors"));

	// Layout
	panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	auto sizer = new wxGridBagSizer(ui::pad(), ui::pad());
	panel->GetSizer()->Add(sizer, 1, wxEXPAND | wxALL, ui::padLarge());

	sizer->Add(new wxStaticText(panel, -1, wxS("Category: ")), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_category_, { 0, 1 }, { 1, 2 }, wxEXPAND);
	sizer->Add(lv_ext_editors_, { 1, 0 }, { 3, 2 }, wxEXPAND);
	sizer->Add(btn_add_exe_, { 1, 2 }, { 1, 1 });
	sizer->Add(btn_remove_exe_, { 2, 2 }, { 1, 1 });

	sizer->AddGrowableRow(3, 1);
	sizer->AddGrowableCol(1, 1);

	return panel;
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
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
	((ExternalEditorList*)lv_ext_editors_)->setCategory(choice_category_->GetStringSelection().utf8_string());
}

// -----------------------------------------------------------------------------
// Applies preference values from the controls to CVARs
// -----------------------------------------------------------------------------
void EditingPrefsPanel::applyPreferences()
{
	wad_force_uppercase       = cb_wad_force_uppercase_->GetValue();
	percent_encoding          = cb_zip_percent_encoding_->GetValue();
	auto_entry_replace        = cb_auto_entry_replace_->GetValue();
	save_archive_with_map     = cb_save_archive_with_map_->GetValue();
	autosave_entry_changes    = choice_entry_mod_->GetSelection();
	confirm_entry_delete      = cb_confirm_entry_delete_->GetValue();
	confirm_entry_revert      = cb_confirm_entry_revert_->GetValue();
	dir_archive_change_action = choice_dir_mod_->GetSelection();
}

// -----------------------------------------------------------------------------
// Shows [subsection] on the prefs page
// -----------------------------------------------------------------------------
void EditingPrefsPanel::showSubSection(const string& subsection)
{
	// 'External' tab
	if (subsection == "external")
		stc_tabs_->SetSelection(1);

	// Default to 'General' tab
	else
		stc_tabs_->SetSelection(0);
}


// -----------------------------------------------------------------------------
//
// EditingPrefsPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Add' button is clicked
// -----------------------------------------------------------------------------
void EditingPrefsPanel::onBtnAddClicked(wxCommandEvent& e)
{
	ExternalEditorDialog dlg(this, true);
	while (dlg.ShowModal() == wxID_OK)
	{
		if (dlg.getName().empty())
			wxMessageBox(wxS("Please enter a name for the editor"), wxS("Name Required"));
		else if (dlg.getPath().empty())
			wxMessageBox(wxS("Please enter or select an executable"), wxS("Path Required"));
		else
		{
			// Add executable
			auto category = choice_category_->GetStringSelection().utf8_string();
			executables::addExternalExe(dlg.getName(), dlg.getPath(), category);

			// Refresh list
			((ExternalEditorList*)lv_ext_editors_)->setCategory(category);

			break;
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Remove' button is clicked
// -----------------------------------------------------------------------------
void EditingPrefsPanel::onBtnRemoveClicked(wxCommandEvent& e)
{
	auto selection = lv_ext_editors_->selection();
	auto category  = choice_category_->GetStringSelection().utf8_string();

	// Remove selected editors
	for (long item : selection)
	{
		auto name = lv_ext_editors_->GetItemText(item);
		executables::removeExternalExe(name.utf8_string(), category);
	}

	// Refresh list
	static_cast<ExternalEditorList*>(lv_ext_editors_)->setCategory(category);
}

// -----------------------------------------------------------------------------
// Called when an item in the external editors list is activated
// -----------------------------------------------------------------------------
void EditingPrefsPanel::onExternalExeActivated(wxListEvent& e)
{
	auto name     = lv_ext_editors_->GetItemText(e.GetIndex()).utf8_string();
	auto category = choice_category_->GetStringSelection().utf8_string();
	auto exe      = executables::externalExe(name, category);

	ExternalEditorDialog dlg(this, false, name, exe.path);
	while (dlg.ShowModal() == wxID_OK)
	{
		if (dlg.getName().empty())
			wxMessageBox(wxS("Please enter a name for the editor"), wxS("Name Required"));
		else if (dlg.getPath().empty())
			wxMessageBox(wxS("Please enter or select an executable"), wxS("Path Required"));
		else
		{
			// Update executable
			executables::setExternalExeName(name, dlg.getName(), category);
			executables::setExternalExePath(dlg.getName(), dlg.getPath(), category);

			// Refresh list
			((ExternalEditorList*)lv_ext_editors_)->setCategory(category);

			break;
		}
	}
}
