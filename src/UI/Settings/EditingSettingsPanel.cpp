
#include "Main.h"
#include "EditingSettingsPanel.h"
#include "ExternalEditorsSettingsPanel.h"
#include "UI/Controls/RadioButtonPanel.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Layout.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

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
EXTERN_CVAR(Bool, confirm_entry_delete)
EXTERN_CVAR(Bool, confirm_entry_revert)
EXTERN_CVAR(Int, dir_archive_change_action)


EditingSettingsPanel::EditingSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	ext_editors_panel_ = new ExternalEditorsSettingsPanel(this);

	auto tabs = STabCtrl::createControl(this);
	tabs->AddPage(createArchiveEditorPanel(tabs), "Archive Editor");
	tabs->AddPage(wxutil::createPadPanel(tabs, ext_editors_panel_, padLarge()), "External Editors");
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
