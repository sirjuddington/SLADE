
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SLADECrashDialog.cpp
// Description: A dialog that displays a crash message and a scrollable,
//              multi-line textbox with a stack trace (plus some other useful
//              info). Also handles sending crash reports
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
#include "CrashReportDialog.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "General/SAction.h"
#include "OpenGL/OpenGL.h"
#include "Utility/JsonUtils.h"
#include <cpptrace/formatting.hpp>
#include <wx/statbmp.h>
#include <wx/url.h>
#include <wx/webrequest.h>

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// CrashReportDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CrashReportDialog class constructor
// -----------------------------------------------------------------------------
CrashReportDialog::CrashReportDialog(wxWindow* parent) :
	wxDialog(parent, -1, wxS("SLADE Application Crash")),
	j_info_{ new Json() }
{
	auto px10 = FromDIP(10);
	auto px6  = FromDIP(6);
	auto px4  = FromDIP(4);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND);

	// Add dead doomguy picture
	app::archiveManager()
		.programResourceArchive()
		->entryAtPath("images/STFDEAD0.png")
		->exportFile(app::path("STFDEAD0.png", app::Dir::Temp));
	wxImage img;
	img.LoadFile(wxString::FromUTF8(app::path("STFDEAD0.png", app::Dir::Temp)));
	img.Rescale(img.GetWidth() * 2, img.GetHeight() * 2, wxIMAGE_QUALITY_NEAREST);
	auto picture = new wxStaticBitmap(this, -1, wxBitmap(img));
	hbox->Add(picture, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM, px10);

	// Add general crash message
	wxString message = wxS(
		"SLADE has crashed unexpectedly. To help fix the problem that caused this crash, "
		"please click 'Send and Exit' to send the crash report. If the issue is recurring often, "
		"please click 'Create GitHub Issue' below and complete the issue details on GitHub.");
	auto label = new wxStaticText(this, -1, message);
	hbox->Add(label, 1, wxEXPAND | wxALL, px10);

	// Add stack trace text area
	text_stack_ = new wxTextCtrl(
		this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
	text_stack_->SetFont(wxFont(9, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	sizer->Add(new wxStaticText(this, -1, wxS("Crash Information:")), 0, wxLEFT | wxRIGHT, px10);
	sizer->AddSpacer(2);
	sizer->Add(text_stack_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, px10);

	// Add 'Copy Stack Trace' button
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, px6);
	btn_copy_trace_ = new wxButton(this, -1, wxS("Copy Stack Trace"));
	hbox->AddStretchSpacer();
	hbox->Add(btn_copy_trace_, 0, wxLEFT | wxRIGHT | wxBOTTOM, px4);
	btn_copy_trace_->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CrashReportDialog::onBtnCopyTrace, this);

	// Add 'Create GitHub Issue' button
	btn_github_issue_ = new wxButton(this, -1, wxS("Create GitHub Issue"));
	hbox->Add(btn_github_issue_, 0, wxLEFT | wxRIGHT | wxBOTTOM, px4);
	btn_github_issue_->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CrashReportDialog::onBtnPostReport, this);

	// Add 'Exit Without Sending' button
	btn_exit_ = new wxButton(this, -1, wxS("Exit Without Sending"));
	hbox->Add(btn_exit_, 0, wxLEFT | wxRIGHT | wxBOTTOM, px4);
	btn_exit_->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CrashReportDialog::onBtnExit, this);

	// Add 'Send and Exit' button
	btn_send_exit_ = new wxButton(this, -1, wxS("Send and Exit"));
	hbox->Add(btn_send_exit_, 0, wxLEFT | wxRIGHT | wxBOTTOM, px4);
	btn_send_exit_->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CrashReportDialog::onBtnSendAndExit, this);

	Bind(wxEVT_WEBREQUEST_STATE, &CrashReportDialog::onWebRequestUpdate, this);

	// Setup layout
	wxDialog::Layout();
	auto width = hbox->CalcMin().GetWidth() + FromDIP(30);
	label->Wrap(width - FromDIP(50) - picture->GetSize().x);
	SetInitialSize(wxSize(width, FromDIP(600)));
	CenterOnParent();
	wxDialog::Show(false);
}

// -----------------------------------------------------------------------------
// Loads crash report data from a cpptrace stacktrace
// -----------------------------------------------------------------------------
void CrashReportDialog::loadFromCpptrace(const cpptrace::stacktrace& trace)
{
	// SLADE info
	if (global::sc_rev.empty())
		trace_ = fmt::format("Version: {}", app::version().toString());
	else
		trace_ = fmt::format("Version: {} ({})", app::version().toString(), global::sc_rev);
	if (app::platform() == app::Platform::Windows)
		trace_ += fmt::format(" ({})\n", app::isWin64Build() ? "x64" : "x86");
	else
		trace_ += "\n";
	auto current_action = SAction::current();
	if (current_action.empty())
		trace_ += "No current action\n";
	else
		trace_ += fmt::format("Current action: {}", current_action);
	trace_ += "\n";

	auto& j_info            = *j_info_;
	j_info["type"]          = 0;
	j_info["slade-version"] = global::sc_rev.empty()
								  ? app::version().toString()
								  : fmt::format("{} ({})", app::version().toString(), global::sc_rev);

	// System info
	gl::Info gl_info = gl::sysInfo();
	string   sys_info;
	sys_info += fmt::format("Operating System: {}\n", wxGetOsDescription().utf8_string());
	sys_info += fmt::format("Graphics Vendor: {}\n", gl_info.vendor);
	sys_info += fmt::format("Graphics Hardware: {}\n", gl_info.renderer);
	sys_info += fmt::format("OpenGL Version: {}\n", gl_info.version);
	trace_ += sys_info;

	j_info["system-info"] = sys_info;
	switch (app::platform())
	{
	case app::Platform::Windows: j_info["platform"] = "Windows"; break;
	case app::Platform::Linux:   j_info["platform"] = "Linux"; break;
	case app::Platform::MacOS:   j_info["platform"] = "MacOS"; break;
	default:                     j_info["platform"] = "Unknown"; break;
	}

	// Stack trace (via cpptrace)
	auto formatter_short = cpptrace::formatter{}
							   .header("Stack Trace:")
							   .addresses(cpptrace::formatter::address_mode::none)
							   .paths(cpptrace::formatter::path_mode::basename)
							   .symbols(cpptrace::formatter::symbol_mode::pruned);
	trace_ += "\n";
	trace_ += formatter_short.format(trace);
	trace_ += "\n";

	// Detailed stack trace for report
	auto formatter_detailed = cpptrace::formatter{}
								  .header("")
								  .addresses(cpptrace::formatter::address_mode::object)
								  .paths(cpptrace::formatter::path_mode::full)
								  .symbols(cpptrace::formatter::symbol_mode::pretty)
								  .snippets(true)
								  .snippet_context(2);
	j_info["stack-trace"] = formatter_detailed.format(trace);

	// Last 10 log lines
	trace_ += "\nLast Log Messages:\n";
	for (auto l : log::last(10))
		trace_ += l->message + "\n";

	// Last 500 log lines for report
	string log_long;
	for (auto l : log::last(500))
		log_long += l->message + "\n";
	j_info["log"] = log_long;

	// Last 5 actions (all for report)
	auto last_actions = SAction::lastPerformed(5);
	if (!last_actions.empty())
	{
		trace_ += "\nLast Actions:\n";
		for (auto& a : last_actions)
			trace_ += a + "\n";

		// Full action history for report
		string action_history;
		for (auto& a : SAction::history())
			action_history += a + "\n";
		j_info["action-log"] = action_history;
	}

	// Set stack trace text
	text_stack_->SetValue(wxString::FromUTF8(trace_));

	// Dump crash details to a file (just in case)
	wxFile file(wxString::FromUTF8(app::path("slade3_crash.log", app::Dir::User)), wxFile::write);
	file.Write(wxString::FromUTF8(trace_));
	file.Close();

	// Print trace to console
	trace.print(std::cerr);
}


// -----------------------------------------------------------------------------
//
// CrashReportDialog Class Event Handlers
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Copy Stack Trace' button is pressed
// -----------------------------------------------------------------------------
void CrashReportDialog::onBtnCopyTrace(wxCommandEvent& e)
{
	if (wxTheClipboard->Open())
	{
		wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(trace_)));
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
// Called when the 'Create GitHub Issue' button is pressed
// -----------------------------------------------------------------------------
void CrashReportDialog::onBtnPostReport(wxCommandEvent& e)
{
	auto url_base = "https://github.com/sirjuddington/SLADE/issues/new?labels=crash+bug&template=crash.yml";
	auto version  = global::sc_rev.empty() ? app::version().toString()
										   : app::version().toString() + " " + global::sc_rev;

	// Build URL
	auto url_str = WX_FMT("{}&version={}&crashinfo={}", url_base, version, trace_);
	url_str.Replace(wxS("#"), wxS("%23")); // Encode '#' characters otherwise the stack trace is cut off

	// Open in browser
	wxLaunchDefaultBrowser(wxURL(url_str).BuildURI());
}

// -----------------------------------------------------------------------------
// Called when the 'Send and Exit' button is pressed
// -----------------------------------------------------------------------------
void CrashReportDialog::onBtnSendAndExit(wxCommandEvent& e)
{
	auto request = wxWebSession::GetDefault().CreateRequest(
		this, wxS("https://slade-crash-report.sirjuddington.workers.dev/"));

	request.SetMethod(wxS("POST"));
	request.SetData(wxString::FromUTF8(j_info_->dump()), wxS("application/json"));

	send_report_request_id_ = request.GetId();

	btn_send_exit_->SetLabel(wxS("Sending..."));
	btn_send_exit_->Enable(false);
	btn_exit_->Enable(false);

	request.Start();
}

// -----------------------------------------------------------------------------
// Called when a web request status is updated
// -----------------------------------------------------------------------------
void CrashReportDialog::onWebRequestUpdate(wxWebRequestEvent& e)
{
	// Check this is the crash report request
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
			WX_FMT("Failed to send crash report:\n{}\n\nSLADE will now exit.", e.GetErrorDescription().utf8_string()),
			wxS("Report Failed"),
			wxICON_ERROR);
	}

	// Close dialog on success or failure
	EndModal(wxID_OK);
}
