
#include "Main.h"
#include "NewTextureDialog.h"
#include "MainEditor/UI/TextureXEditor/PatchBrowser.h"
#include "UI/Browser/BrowserItem.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace texeditor;

NewTextureDialog::NewTextureDialog(wxWindow* parent, PatchBrowser* patch_browser) :
	SDialog(parent, "New Texture", "new_texture"),
	patch_browser_(patch_browser)
{
	auto lh = ui::LayoutHelper(this);

	wxutil::setWindowIcon(this, "tex_new");

	SetSizer(new wxBoxSizer(wxVERTICAL));
	auto* sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	GetSizer()->Add(sizer, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxTOP).Expand());

	// Name
	text_name_ = new wxTextCtrl(this, -1);
	sizer->Add(new wxStaticText(this, -1, wxS("Name:")), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(text_name_, { 0, 1 }, { 1, 4 }, wxEXPAND);

	// Blank
	rb_blank_ = new wxRadioButton(this, -1, wxS("Blank"));
	rb_blank_->SetValue(true);
	spin_width_  = wxutil::createSpinCtrl(this, 64, 0, 4096);
	spin_height_ = wxutil::createSpinCtrl(this, 128, 0, 4096);
	sizer->Add(rb_blank_, { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(new wxStaticText(this, -1, wxS("Size:")), { 1, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
	sizer->Add(spin_width_, { 1, 2 }, { 1, 1 }, wxEXPAND);
	sizer->Add(new wxStaticText(this, -1, wxS("x")), { 1, 3 }, { 1, 1 }, wxALIGN_CENTER);
	sizer->Add(spin_height_, { 1, 4 }, { 1, 1 }, wxEXPAND);

	// From Patch
	rb_patch_         = new wxRadioButton(this, -1, wxS("From Patch"));
	text_patch_       = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	btn_browse_patch_ = new wxButton(this, -1, wxS("Browse"));
	btn_browse_patch_->Enable(false);
	sizer->Add(rb_patch_, { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(text_patch_, { 2, 1 }, { 1, 3 }, wxEXPAND);
	sizer->Add(btn_browse_patch_, { 2, 4 }, { 1, 1 }, wxEXPAND);

	sizer->AddGrowableCol(1, 1);

	// Separator
	GetSizer()->Add(
		new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxHORIZONTAL),
		lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxTOP).Expand());

	// Dialog buttons
	auto* btn_create = new wxButton(this, -1, wxS("Create"));
	auto* btn_cancel = new wxButton(this, -1, wxS("Cancel"));
	btn_create->SetDefault();
	auto* hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->AddStretchSpacer(1);
	hbox->Add(btn_create, lh.sfWithBorder(0, wxRIGHT).Expand());
	hbox->Add(btn_cancel, wxSizerFlags().Expand());
	GetSizer()->Add(hbox, lh.sfWithLargeBorder().Expand());

	// Bind events
	rb_blank_->Bind(
		wxEVT_RADIOBUTTON,
		[this](wxCommandEvent&)
		{
			spin_width_->Enable(rb_blank_->GetValue());
			spin_height_->Enable(rb_blank_->GetValue());
			btn_browse_patch_->Enable(!rb_blank_->GetValue());
		});
	rb_patch_->Bind(
		wxEVT_RADIOBUTTON,
		[this](wxCommandEvent&)
		{
			spin_width_->Enable(rb_blank_->GetValue());
			spin_height_->Enable(rb_blank_->GetValue());
			btn_browse_patch_->Enable(!rb_blank_->GetValue());
		});
	btn_browse_patch_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { browsePatch(); });
	btn_cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });
	btn_create->Bind(
		wxEVT_BUTTON,
		[this](wxCommandEvent&)
		{
			if (checkValues())
				EndModal(wxID_OK);
		});

	// Setup dialog size
	setSavedSize(FromDIP(400), -1);
	// SetInitialSize({ FromDIP(400), -1 });
	// wxTopLevelWindowBase::Layout();
	// wxWindowBase::Fit();
	// wxTopLevelWindowBase::SetMinSize(GetBestSize());
	CenterOnParent();
}

bool NewTextureDialog::checkValues() const
{
	// Name
	if (text_name_->GetValue().Trim().IsEmpty())
	{
		wxMessageBox(wxS("Please enter a texture name"), wxS("New Texture"));
		return false;
	}

	// Patch
	if (rb_patch_->GetValue() && text_patch_->GetValue().Trim().IsEmpty())
	{
		wxMessageBox(wxS("Please select a patch"), wxS("New Texture"));
		return false;
	}

	return true;
}

void NewTextureDialog::browsePatch() const
{
	// Select initial patch if one already entered
	if (!text_patch_->GetValue().Trim().IsEmpty())
		patch_browser_->selectPatch(text_patch_->GetValue().utf8_string());

	// Open browser and return selected patch name (empty if cancelled)
	string patch;
	if (patch_browser_->ShowModal() == wxID_OK && patch_browser_->selectedItem())
		patch = patch_browser_->selectedItem()->name();

	if (!patch.empty())
		text_patch_->SetValue(wxString::FromUTF8(patch));
}
