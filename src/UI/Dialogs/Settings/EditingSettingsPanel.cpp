
#include "Main.h"
#include "EditingSettingsPanel.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Executables.h"
#include "UI/Controls/RadioButtonPanel.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Layout.h"
#include "UI/Lists/VirtualListView.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, wad_force_uppercase)
EXTERN_CVAR(Int, autosave_entry_changes)
EXTERN_CVAR(Bool, percent_encoding)
EXTERN_CVAR(Bool, auto_entry_replace)
EXTERN_CVAR(Bool, elist_filter_dirs)
EXTERN_CVAR(Bool, save_archive_with_map) // move
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
		AppendColumn("Name");
		AppendColumn("Path");
	}

	~ExternalEditorList() override = default;

	void setCategory(string_view category)
	{
		exes_ = executables::externalExes(category);
		SetItemCount(exes_.size());
	}

	wxString itemText(long item, long column, long index) const override
	{
		if (item < 0 || item >= static_cast<long>(exes_.size()))
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
	ExternalEditorDialog(wxWindow* parent, bool browse_on_open, const wxString& name = "", const wxString& path = "") :
		wxDialog(parent, -1, "External Editor"),
		browse_on_open_(browse_on_open)
	{
		auto lh    = ui::LayoutHelper(this);
		auto sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		// Name
		auto gb_sizer = new wxGridBagSizer(lh.pad(), lh.pad());
		sizer->Add(gb_sizer, lh.sfWithLargeBorder(1).Expand());
		gb_sizer->Add(new wxStaticText(this, -1, "Name:"), { 0, 0 }, wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		text_name_ = new wxTextCtrl(this, -1, name);
		gb_sizer->Add(text_name_, { 0, 1 }, { 1, 2 }, wxEXPAND);

		// Path
		gb_sizer->Add(new wxStaticText(this, -1, "Path:"), { 1, 0 }, wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		text_path_ = new wxTextCtrl(this, -1, path, wxDefaultPosition, lh.size(300, -1));
		gb_sizer->Add(text_path_, { 1, 1 }, wxDefaultSpan, wxEXPAND);
		btn_browse_ = new SIconButton(this, icons::General, "open");
		gb_sizer->Add(btn_browse_, { 1, 2 }, wxDefaultSpan);

		// Ok/Cancel
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		gb_sizer->Add(hbox, { 2, 0 }, { 1, 3 }, wxEXPAND);
		hbox->AddStretchSpacer();
		btn_cancel_ = new wxButton(this, wxID_CANCEL, "Cancel");
		hbox->Add(btn_cancel_, lh.sfWithBorder(0, wxRIGHT).Expand());
		btn_ok_ = new wxButton(this, wxID_OK, "OK");
		hbox->Add(btn_ok_, wxSizerFlags().Expand());

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

	wxString getName() const { return text_name_->GetValue(); }
	wxString getPath() const { return text_path_->GetValue(); }

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
			text_path_->SetValue(info.filenames[0]);

			if (text_name_->GetValue().IsEmpty())
			{
				wxFileName fn(info.filenames[0]);
				text_name_->SetValue(fn.GetName().Capitalize());
			}
		}
	}
};




EditingSettingsPanel::EditingSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto tabs = STabCtrl::createControl(this);
	tabs->AddPage(createArchiveEditorPanel(tabs), "Archive Editor");
	tabs->AddPage(createExternalEditorsPanel(tabs), "External Editors");
	sizer->Add(tabs, wxSizerFlags(1).Expand());
}

void EditingSettingsPanel::loadSettings()
{
	cb_wad_force_uppercase_->SetValue(wad_force_uppercase);
	cb_zip_percent_encoding_->SetValue(percent_encoding);
	cb_auto_entry_replace_->SetValue(auto_entry_replace);
	cb_filter_dirs_->SetValue(elist_filter_dirs);
	cb_confirm_entry_delete_->SetValue(confirm_entry_delete);
	cb_confirm_entry_revert_->SetValue(confirm_entry_revert);
	rbp_entry_mod_->setSelection(autosave_entry_changes);
	rbp_dir_mod_->setSelection(dir_archive_change_action);

	choice_category_->SetSelection(0);
	dynamic_cast<ExternalEditorList*>(lv_ext_editors_)
		->setCategory(wxutil::strToView(choice_category_->GetStringSelection()));
}

void EditingSettingsPanel::applySettings()
{
	wad_force_uppercase       = cb_wad_force_uppercase_->GetValue();
	percent_encoding          = cb_zip_percent_encoding_->GetValue();
	auto_entry_replace        = cb_auto_entry_replace_->GetValue();
	elist_filter_dirs         = cb_filter_dirs_->GetValue();
	confirm_entry_delete      = cb_confirm_entry_delete_->GetValue();
	confirm_entry_revert      = cb_confirm_entry_revert_->GetValue();
	autosave_entry_changes    = rbp_entry_mod_->getSelection();
	dir_archive_change_action = rbp_dir_mod_->getSelection();
}

wxPanel* EditingSettingsPanel::createArchiveEditorPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = LayoutHelper(panel);

	// Create controls
	cb_wad_force_uppercase_  = new wxCheckBox(panel, wxID_ANY, "Force uppercase entry names in Wad Archives");
	cb_zip_percent_encoding_ = new wxCheckBox(
		panel, wxID_ANY, "Use percent encoding if needed outside of Wad Archives");
	cb_auto_entry_replace_ = new wxCheckBox(
		panel, -1, "Automatically replace entries with same name as drag-and-dropped files");
	cb_filter_dirs_          = new wxCheckBox(panel, -1, "Ignore directories when filtering by name");
	cb_confirm_entry_delete_ = new wxCheckBox(panel, -1, "Show confirmation dialog on deleting an entry");
	cb_confirm_entry_revert_ = new wxCheckBox(panel, -1, "Show confirmation dialog on reverting entry changes");
	rbp_entry_mod_ = new RadioButtonPanel(panel, { "Don't Save", "Save", "Ask" }, "Action on unsaved entry changes:");
	rbp_dir_mod_   = new RadioButtonPanel(
        panel, { "Ignore Changes", "Apply Changes", "Ask" }, "Action on external directory changes");

	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, lh.sfWithLargeBorder(0).Expand());

	// Editor Behaviour
	vbox->Add(wxutil::createSectionSeparator(panel, "Behaviour"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	vbox->Add(
		lh.layoutVertically({ cb_auto_entry_replace_,
							  cb_filter_dirs_,
							  cb_confirm_entry_delete_,
							  cb_confirm_entry_revert_,
							  rbp_entry_mod_,
							  rbp_dir_mod_ }),
		lh.sfWithBorder(0, wxLEFT));

	// Entry naming
	vbox->AddSpacer(lh.padXLarge());
	vbox->Add(wxutil::createSectionSeparator(panel, "Entry Naming"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	vbox->Add(lh.layoutVertically({ cb_wad_force_uppercase_, cb_zip_percent_encoding_ }), lh.sfWithBorder(0, wxLEFT));

	return panel;
}

wxPanel* EditingSettingsPanel::createExternalEditorsPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent, -1);
	auto lh    = LayoutHelper(panel);

	// Create controls
	auto categories  = wxutil::arrayStringStd(EntryType::allCategories());
	choice_category_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, categories);
	lv_ext_editors_  = new ExternalEditorList(panel);
	btn_add_exe_     = new SIconButton(panel, icons::General, "plus");
	btn_add_exe_->SetToolTip("Add External Editor");
	btn_remove_exe_ = new SIconButton(panel, icons::General, "minus");
	btn_remove_exe_->SetToolTip("Remove Selected External Editors");

	// Layout
	panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	auto sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	panel->GetSizer()->Add(sizer, lh.sfWithLargeBorder(1).Expand());

	sizer->Add(new wxStaticText(panel, -1, "Category: "), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_category_, { 0, 1 }, { 1, 2 }, wxEXPAND);
	sizer->Add(lv_ext_editors_, { 1, 0 }, { 3, 2 }, wxEXPAND);
	sizer->Add(btn_add_exe_, { 1, 2 }, { 1, 1 });
	sizer->Add(btn_remove_exe_, { 2, 2 }, { 1, 1 });

	sizer->AddGrowableRow(3, 1);
	sizer->AddGrowableCol(1, 1);

	// Bind events
	choice_category_->Bind(
		wxEVT_CHOICE,
		[&](wxCommandEvent&)
		{
			dynamic_cast<ExternalEditorList*>(lv_ext_editors_)
				->setCategory(wxutil::strToView(choice_category_->GetStringSelection()));
		});
	btn_add_exe_->Bind(wxEVT_BUTTON, &EditingSettingsPanel::onBtnAddClicked, this);
	btn_remove_exe_->Bind(wxEVT_BUTTON, &EditingSettingsPanel::onBtnRemoveClicked, this);
	lv_ext_editors_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &EditingSettingsPanel::onExternalExeActivated, this);

	return panel;
}


// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the 'Add' button is clicked
// -----------------------------------------------------------------------------
void EditingSettingsPanel::onBtnAddClicked(wxCommandEvent& e)
{
	ExternalEditorDialog dlg(this, true);
	while (dlg.ShowModal() == wxID_OK)
	{
		if (dlg.getName().empty())
			wxMessageBox("Please enter a name for the editor", "Name Required");
		else if (dlg.getPath().empty())
			wxMessageBox("Please enter or select an executable", "Path Required");
		else
		{
			// Add executable
			auto category = choice_category_->GetStringSelection().ToStdString();
			executables::addExternalExe(dlg.getName().ToStdString(), dlg.getPath().ToStdString(), category);

			// Refresh list
			dynamic_cast<ExternalEditorList*>(lv_ext_editors_)->setCategory(category);

			break;
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Remove' button is clicked
// -----------------------------------------------------------------------------
void EditingSettingsPanel::onBtnRemoveClicked(wxCommandEvent& e)
{
	auto selection = lv_ext_editors_->selection();
	auto category  = choice_category_->GetStringSelection().ToStdString();

	// Remove selected editors
	for (long item : selection)
	{
		wxString name = lv_ext_editors_->GetItemText(item);
		executables::removeExternalExe(wxutil::strToView(name), category);
	}

	// Refresh list
	dynamic_cast<ExternalEditorList*>(lv_ext_editors_)->setCategory(category);
}

// -----------------------------------------------------------------------------
// Called when an item in the external editors list is activated
// -----------------------------------------------------------------------------
void EditingSettingsPanel::onExternalExeActivated(wxListEvent& e)
{
	auto name     = lv_ext_editors_->GetItemText(e.GetIndex()).ToStdString();
	auto category = choice_category_->GetStringSelection().ToStdString();
	auto exe      = executables::externalExe(name, category);

	ExternalEditorDialog dlg(this, false, name, exe.path);
	while (dlg.ShowModal() == wxID_OK)
	{
		if (dlg.getName().empty())
			wxMessageBox("Please enter a name for the editor", "Name Required");
		else if (dlg.getPath().empty())
			wxMessageBox("Please enter or select an executable", "Path Required");
		else
		{
			// Update executable
			executables::setExternalExeName(name, dlg.getName().ToStdString(), category);
			executables::setExternalExePath(dlg.getName().ToStdString(), dlg.getPath().ToStdString(), category);

			// Refresh list
			dynamic_cast<ExternalEditorList*>(lv_ext_editors_)->setCategory(category);

			break;
		}
	}
}
