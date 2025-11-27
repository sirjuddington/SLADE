
#include "Main.h"
#include "ExceptionDialog.h"
#include "UI/Layout.h"
#include "Utility/StringUtils.h"
#include <wx/artprov.h>
#include <wx/statbmp.h>

using namespace slade;
using namespace ui;

ExceptionDialog::ExceptionDialog(
	wxWindow*     parent,
	const string& message,
	const string& stacktrace_simple,
	const string& stacktrace_full) :
	wxDialog(parent, wxID_ANY, wxS("SLADE Exception"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	LayoutHelper lh(this);

	// Ensure first character of the error message is capitalized
	string message_capitalized = message;
	message_capitalized[0]     = static_cast<char>(std::toupper(static_cast<unsigned char>(message_capitalized[0])));

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto hbox_top = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox_top, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxTOP).Expand());

	// Error icon
	auto bmp_icon = new wxStaticBitmap(
		this,
		wxID_ANY,
		wxArtProvider::GetBitmapBundle(wxASCII_STR(wxART_WARNING), wxASCII_STR(wxART_MESSAGE_BOX), wxSize(32, 32)));
	hbox_top->Add(bmp_icon, lh.sfWithLargeBorder(0, wxRIGHT).CenterVertical());

	// Error message
	auto vbox_error = new wxBoxSizer(wxVERTICAL);
	hbox_top->Add(vbox_error, wxSizerFlags().CenterVertical());
	vbox_error->Add(
		new wxStaticText(this, wxID_ANY, wxS("SLADE has encountered an exception:")),
		lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	auto st_error = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(message_capitalized));
	st_error->SetFont(st_error->GetFont().Bold());
	vbox_error->Add(st_error, wxSizerFlags().Expand());

	// Stack trace text area
	sizer->Add(
		new wxStaticText(this, wxID_ANY, wxS("Stack Trace:")),
		lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxTOP).Expand());
	sizer->AddSpacer(lh.padSmall());
	text_stacktrace_ = new wxTextCtrl(
		this,
		wxID_ANY,
		stacktrace_simple.empty() ? wxString::FromUTF8("No stack trace available")
								  : wxString::FromUTF8(stacktrace_simple),
		wxDefaultPosition,
		lh.size(500, stacktrace_simple.empty() ? 100 : 400),
		wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
	text_stacktrace_->SetFont(wxFont(9, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	sizer->Add(text_stacktrace_, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Buttons
	auto hbox_buttons = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox_buttons, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Copy Stack Trace button
	if (!stacktrace_simple.empty())
	{
		auto btn_copy = new wxButton(this, wxID_ANY, wxS("Copy Stack Trace"));
		hbox_buttons->Add(btn_copy, wxSizerFlags().Expand());

		btn_copy->Bind(
			wxEVT_BUTTON,
			[this, stacktrace_simple](wxCommandEvent&)
			{
				if (wxTheClipboard->Open())
				{
					wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(stacktrace_simple)));
					wxTheClipboard->Flush();
					wxTheClipboard->Close();
					wxMessageBox(wxS("Stack trace successfully copied to clipboard"));
				}
				else
					wxMessageBox(
						wxS("Unable to access the system clipboard, please select+copy the text above manually"),
						wxS("Error"),
						wxICON_EXCLAMATION);
			});
	}
	hbox_buttons->AddStretchSpacer();

	// TODO: Send Report button

	// Continue button
	auto btn_continue = new wxButton(this, wxID_OK, wxS("Continue"));
	hbox_buttons->Add(btn_continue, wxSizerFlags().Expand());

	// Setup dialog
	wxDialog::Layout();
	SetInitialSize(GetSizer()->GetMinSize());
	st_error->Wrap(FromDIP(440));
	wxDialog::Layout(); // Need to do this twice so that the error message text wraps correctly
	SetInitialSize(GetSizer()->GetMinSize());
	CenterOnParent();
}
