
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ExceptionDialog.cpp
// Description: A dialog that displays an exception message and a scrollable,
//              multi-line textbox with a stack trace
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
#include "ExceptionDialog.h"
#include "App.h"
#include "UI/Layout.h"
#include "nlohmann/json.hpp"
#include <wx/artprov.h>
#include <wx/statbmp.h>
#include <wx/webrequest.h>

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// ExceptionDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ExceptionDialog class constructor
// -----------------------------------------------------------------------------
ExceptionDialog::ExceptionDialog(
	wxWindow*     parent,
	const string& message,
	const string& stacktrace_simple,
	const string& stacktrace_full) :
	wxDialog(parent, wxID_ANY, wxS("SLADE Exception"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE),
	message_{ message },
	stacktrace_simple_{ stacktrace_simple },
	stacktrace_full_{ stacktrace_full }
{
	LayoutHelper lh(this);

	// Ensure first character of the error message is capitalized
	if (!message_.empty())
		message_[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(message_[0])));

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
	auto st_error = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(message_));
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
		btn_copy->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { copyStackTrace(); });
	}
	hbox_buttons->AddStretchSpacer();

	// Send And Continue button
	if (!stacktrace_simple.empty())
	{
		btn_send_continue_ = new wxButton(this, wxID_ANY, wxS("Send And Continue"));
		hbox_buttons->Add(btn_send_continue_, lh.sfWithBorder(0, wxRIGHT).Expand());
		btn_send_continue_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { sendReport(); });
		Bind(wxEVT_WEBREQUEST_STATE, &ExceptionDialog::onWebRequestUpdate, this);
	}

	// Continue button
	btn_continue_ = new wxButton(this, wxID_OK, wxS("Continue"));
	hbox_buttons->Add(btn_continue_, wxSizerFlags().Expand());

	// Setup dialog
	wxDialog::Layout();
	SetInitialSize(GetSizer()->GetMinSize());
	st_error->Wrap(FromDIP(440));
	wxDialog::Layout(); // Need to do this twice so that the error message text wraps correctly
	SetInitialSize(GetSizer()->GetMinSize());
	CenterOnParent();
}

// -----------------------------------------------------------------------------
// Copies the stack trace to the system clipboard and notifies the user
// -----------------------------------------------------------------------------
void ExceptionDialog::copyStackTrace() const
{
	if (wxTheClipboard->Open())
	{
		wxTheClipboard->SetData(new wxTextDataObject(text_stacktrace_->GetValue()));
		wxTheClipboard->Flush();
		wxTheClipboard->Close();
		wxMessageBox(wxS("Stack trace successfully copied to clipboard"));
	}
	else
		wxMessageBox(
			wxS("Unable to access the system clipboard, please select+copy the text above manually"),
			wxS("Error"),
			wxICON_EXCLAMATION);
}

// -----------------------------------------------------------------------------
// Sends an exception report
// -----------------------------------------------------------------------------
void ExceptionDialog::sendReport()
{
	// Build report JSON
	nlohmann::json j_info;
	j_info["slade-version"] = global::sc_rev.empty()
								  ? app::version().toString()
								  : fmt::format("{} ({})", app::version().toString(), global::sc_rev);
	switch (app::platform())
	{
	case app::Platform::Windows: j_info["platform"] = "Windows"; break;
	case app::Platform::Linux:   j_info["platform"] = "Linux"; break;
	case app::Platform::MacOS:   j_info["platform"] = "MacOS"; break;
	default:                     j_info["platform"] = "Unknown"; break;
	}
	j_info["stack-trace"] = stacktrace_full_;
	j_info["message"]     = message_;
	j_info["type"]        = 1;
	// TODO: Do we want system info and logs? Maybe add later if needed

	// Send request
	auto request = wxWebSession::GetDefault().CreateRequest(
		this, wxS("https://slade-crash-report.sirjuddington.workers.dev/"));

	request.SetMethod(wxS("POST"));
	request.SetData(wxString::FromUTF8(j_info.dump()), wxS("application/json"));

	send_report_request_id_ = request.GetId();

	btn_send_continue_->SetLabel(wxS("Sending..."));
	btn_send_continue_->Enable(false);
	btn_continue_->Enable(false);

	request.Start();
}

// -----------------------------------------------------------------------------
// Called when a web request status is updated
// -----------------------------------------------------------------------------
void ExceptionDialog::onWebRequestUpdate(wxWebRequestEvent& e)
{
	// Check this is the exception report request
	if (e.GetId() != send_report_request_id_)
		return;

	// Ignore active/idle states
	if (e.GetState() == wxWebRequest::State::State_Active || e.GetState() == wxWebRequest::State::State_Idle)
		return;

	// Failed to send report - show error message
	if (e.GetState() == wxWebRequest::State::State_Failed || e.GetState() == wxWebRequest::State::State_Unauthorized
		|| e.GetState() == wxWebRequest::State::State_Cancelled)
	{
		wxMessageBox(
			WX_FMT("Failed to send exception report:\n{}", e.GetErrorDescription().utf8_string()),
			wxS("Report Failed"),
			wxICON_ERROR);
	}

	// Close dialog on success or failure
	EndModal(wxID_OK);
}
