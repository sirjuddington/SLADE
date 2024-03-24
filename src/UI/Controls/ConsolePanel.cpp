
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ConsolePanel.cpp
// Description: UI Frontend panel for the console
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
#include "ConsolePanel.h"
#include "App.h"
#include "General/Console.h"
#include "General/UI.h"
#include "TextEditor/TextStyle.h"
#include "UI/WxUtils.h"
#include "Utility/Colour.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// ConsolePanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ConsolePanel class constructor
// -----------------------------------------------------------------------------
ConsolePanel::ConsolePanel(wxWindow* parent, int id) : wxPanel(parent, id)
{
	// Setup layout
	initLayout();

	// Bind events
	text_command_->Bind(wxEVT_TEXT_ENTER, &ConsolePanel::onCommandEnter, this);
	text_command_->Bind(wxEVT_KEY_DOWN, &ConsolePanel::onCommandKeyDown, this);

	// Start update timer
	timer_update_.Bind(wxEVT_TIMER, [&](wxTimerEvent&) { update(); });
	timer_update_.Start(100);
}

// -----------------------------------------------------------------------------
// Sets up the panel layout
// -----------------------------------------------------------------------------
void ConsolePanel::initLayout()
{
	// Create and set the sizer for the panel
	auto vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(vbox);

	// Create and add the message log textbox
	text_log_ = new wxStyledTextCtrl(this, -1, wxDefaultPosition, wxDefaultSize);
	text_log_->SetEditable(false);
#ifdef __WXGTK__
	// workaround for an extremely convoluted wxGTK bug that causes a resource
	// leak that makes SLADE unusable on linux (!) -- see:
	// https://github.com/sirjuddington/SLADE/issues/1016
	// https://github.com/wxWidgets/wxWidgets/issues/23364
	text_log_->SetWrapMode(wxSTC_WRAP_NONE);
#else
	text_log_->SetWrapMode(wxSTC_WRAP_WORD);
#endif
	text_log_->SetSizeHints(wxSize(-1, 0));
	vbox->Add(text_log_, wxutil::sfWithBorder(1, wxLEFT | wxRIGHT | wxTOP).Expand());

	// Create and add the command entry textbox
	text_command_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	vbox->AddSpacer(ui::padMin());
	vbox->Add(text_command_, wxutil::sfWithBorder(0, wxBOTTOM | wxLEFT | wxRIGHT).Expand());

	Layout();

	// Set console font to default+monospace
	auto font = wxutil::monospaceFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
	text_command_->SetFont(font);

	setupTextArea();
}

// -----------------------------------------------------------------------------
// Sets up the log history text control
// -----------------------------------------------------------------------------
void ConsolePanel::setupTextArea() const
{
	// Style
	StyleSet::currentSet()->applyToWx(text_log_);

	// Margins
	text_log_->SetMarginWidth(0, text_log_->TextWidth(wxSTC_STYLE_DEFAULT, "00:00:00"));
	text_log_->SetMarginType(0, wxSTC_MARGIN_TEXT);
	text_log_->SetMarginWidth(1, 8);

	// Message type colours
	auto hsl = colour::rgbToHsl(StyleSet::currentSet()->styleForeground("default"));
	if (hsl.l > 0.8)
		hsl.l = 0.8;
	if (hsl.l < 0.2)
		hsl.l = 0.2;
	text_log_->StyleSetForeground(200, ColHSL(0.99, 1., hsl.l).asRGB());
	text_log_->StyleSetForeground(201, ColHSL(0.1, 1., hsl.l).asRGB());
	text_log_->StyleSetForeground(202, ColHSL(0.5, 0.8, hsl.l).asRGB());
	text_log_->StyleSetForeground(203, ColHSL(hsl.h, hsl.s, 0.5).asRGB());
}

// -----------------------------------------------------------------------------
// Update the log text with any new log messages
// -----------------------------------------------------------------------------
void ConsolePanel::update()
{
	setupTextArea();

	// Check if any new log messages were added since the last update
	auto& log = log::history();
	if (log.size() <= next_message_index_)
	{
		// None added, check again in 500ms
		timer_update_.Start(500);
		return;
	}

	// Add new log messages to log text area
	text_log_->SetEditable(true);
	int line_no = next_message_index_;
	for (auto a = next_message_index_; a < log.size(); ++a)
	{
		if (a > 0)
			text_log_->AppendText("\n");

		// Add message line + timestamp margin
		text_log_->AppendText(log[a].message);
		text_log_->MarginSetText(line_no, wxDateTime(log[a].timestamp).FormatISOTime());
		text_log_->MarginSetStyle(line_no, wxSTC_STYLE_LINENUMBER);

		// Set line colour depending on message type
#if !wxCHECK_VERSION(3, 1, 1)
		// Prior to wxWidgets 3.1.1 StartStyling took 2 arguments, no overload exists for compatibility
		text_log_->StartStyling(text_log_->GetLineEndPosition(line_no) - text_log_->GetLineLength(line_no), 0);
#else
		text_log_->StartStyling(text_log_->GetLineEndPosition(line_no) - text_log_->GetLineLength(line_no));
#endif

		switch (log[a].type)
		{
		case log::MessageType::Error:   text_log_->SetStyling(text_log_->GetLineLength(line_no), 200); break;
		case log::MessageType::Warning: text_log_->SetStyling(text_log_->GetLineLength(line_no), 201); break;
		case log::MessageType::Script:  text_log_->SetStyling(text_log_->GetLineLength(line_no), 202); break;
		case log::MessageType::Debug:   text_log_->SetStyling(text_log_->GetLineLength(line_no), 203); break;
		default:                        break;
		}

		++line_no;
	}
	text_log_->SetEditable(false);

	next_message_index_ = log::history().size();
	text_log_->ScrollToEnd();

	// Check again in 100ms
	timer_update_.Start(100);
}


// -----------------------------------------------------------------------------
//
// ConsolePanel Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the enter key is pressed in the command text box
// -----------------------------------------------------------------------------
void ConsolePanel::onCommandEnter(wxCommandEvent& e)
{
	app::console()->execute(wxutil::strToView(e.GetString()));
	update();
	text_command_->Clear();
	cmd_log_index_ = 0;
}

// -----------------------------------------------------------------------------
// Called when a key is pressed in the command text box
// -----------------------------------------------------------------------------
void ConsolePanel::onCommandKeyDown(wxKeyEvent& e)
{
	if (e.GetKeyCode() == WXK_UP)
	{
		text_command_->SetValue(app::console()->prevCommand(cmd_log_index_));
		text_command_->SetInsertionPointEnd();
		if (cmd_log_index_ < app::console()->numPrevCommands() - 1)
			cmd_log_index_++;
	}
	else if (e.GetKeyCode() == WXK_DOWN)
	{
		cmd_log_index_--;
		text_command_->SetValue(app::console()->prevCommand(cmd_log_index_));
		text_command_->SetInsertionPointEnd();
		if (cmd_log_index_ < 0)
			cmd_log_index_ = 0;
	}
	else
		e.Skip();
}
