
#include "Main.h"
#include "EditingSettingsPanel.h"
#include "Archive/EntryType/EntryType.h"
#include "ExternalEditorsSettingsPanel.h"
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

	ext_editors_panel_->loadSettings();
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

	ext_editors_panel_->applySettings();
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

	// Layout
	panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	panel->GetSizer()->Add(
		ext_editors_panel_ = new ExternalEditorsSettingsPanel(panel), lh.sfWithLargeBorder(1).Expand());
	ext_editors_panel_->Show();

	return panel;
}
