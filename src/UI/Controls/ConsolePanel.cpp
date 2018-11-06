
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "App.h"
#include "ConsolePanel.h"
#include "General/Console/Console.h"
#include "General/Misc.h"
#include "TextEditor/TextStyle.h"
#include "UI/WxUtils.h"


// ----------------------------------------------------------------------------
//
// ConsolePanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ConsolePanel::ConsolePanel
//
// ConsolePanel class constructor
// ----------------------------------------------------------------------------
ConsolePanel::ConsolePanel(wxWindow* parent, int id)
	: wxPanel(parent, id)
{
	// Init variables
	cmd_log_index_ = 0;
	next_message_index_ = 0;

	// Setup layout
	initLayout();

	// Bind events
	text_command_->Bind(wxEVT_TEXT_ENTER, &ConsolePanel::onCommandEnter, this);
	text_command_->Bind(wxEVT_KEY_DOWN, &ConsolePanel::onCommandKeyDown, this);

	// Start update timer
	timer_update_.Bind(wxEVT_TIMER, [&](wxTimerEvent&){ update(); });
	timer_update_.Start(100);
}

// ----------------------------------------------------------------------------
// ConsolePanel::~ConsolePanel
//
// ConsolePanel class destructor
// ----------------------------------------------------------------------------
ConsolePanel::~ConsolePanel()
{
}

// ----------------------------------------------------------------------------
// ConsolePanel::initLayout
//
// Sets up the panel layout
// ----------------------------------------------------------------------------
void ConsolePanel::initLayout()
{
	// Create and set the sizer for the panel
	auto vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(vbox);

	// Create and add the message log textbox
	text_log_ = new wxStyledTextCtrl(this, -1, wxDefaultPosition, wxDefaultSize);
	text_log_->SetEditable(false);
	text_log_->SetWrapMode(wxSTC_WRAP_WORD);
	text_log_->SetSizeHints(wxSize(-1, 0));
	vbox->Add(text_log_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, UI::pad());

	// Create and add the command entry textbox
	text_command_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	vbox->AddSpacer(UI::px(UI::Size::PadMinimum));
	vbox->Add(text_command_, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, UI::pad());

	Layout();

	// Set console font to default+monospace
	auto font = WxUtils::getMonospaceFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
	text_command_->SetFont(font);

	setupTextArea();
}

// ----------------------------------------------------------------------------
// ConsolePanel::setupTextArea
//
// Sets up the log history text control
// ----------------------------------------------------------------------------
void ConsolePanel::setupTextArea() const
{
	// Style
	StyleSet::currentSet()->applyToWx(text_log_);

	// Margins
	text_log_->SetMarginWidth(0, text_log_->TextWidth(wxSTC_STYLE_DEFAULT, "00:00:00"));
	text_log_->SetMarginType(0, wxSTC_MARGIN_TEXT);
	text_log_->SetMarginWidth(1, 8);

	// Message type colours
	auto hsl = Misc::rgbToHsl(StyleSet::currentSet()->getStyleForeground("default"));
	if (hsl.l > 0.8) hsl.l = 0.8;
	if (hsl.l < 0.2) hsl.l = 0.2;
	text_log_->StyleSetForeground(200, WXCOL(Misc::hslToRgb(0.99, 1., hsl.l)));
	text_log_->StyleSetForeground(201, WXCOL(Misc::hslToRgb(0.1, 1., hsl.l)));
	text_log_->StyleSetForeground(202, WXCOL(Misc::hslToRgb(0.5, 0.8, hsl.l)));
	text_log_->StyleSetForeground(203, WXCOL(Misc::hslToRgb(hsl.h, hsl.s, 0.5)));
}

// ----------------------------------------------------------------------------
// ConsolePanel::update
//
// Update the log text with any new log messages
// ----------------------------------------------------------------------------
void ConsolePanel::update()
{
	setupTextArea();

	// Check if any new log messages were added since the last update
	auto& log = Log::history();
	if (log.size() <= next_message_index_)
	{
		// None added, check again in 500ms
		timer_update_.Start(500);
		return;
	}

	// Add new log messages to log text area
	text_log_->SetEditable(true);
	for (auto a = next_message_index_; a < log.size(); ++a)
	{
		if (a > 0)
			text_log_->AppendText("\n");

		// Add message line + timestamp margin
		text_log_->AppendText(log[a].message);
		text_log_->MarginSetText(a, wxDateTime(log[a].timestamp).FormatISOTime());
		text_log_->MarginSetStyle(a, wxSTC_STYLE_LINENUMBER);

		// Set line colour depending on message type
		text_log_->StartStyling(text_log_->GetLineEndPosition(a) - text_log_->GetLineLength(a), 0);
		switch (log[a].type)
		{
		case Log::MessageType::Error:
			text_log_->SetStyling(text_log_->GetLineLength(a), 200); break;
		case Log::MessageType::Warning:
			text_log_->SetStyling(text_log_->GetLineLength(a), 201); break;
		case Log::MessageType::Script:
			text_log_->SetStyling(text_log_->GetLineLength(a), 202); break;
		case Log::MessageType::Debug:
			text_log_->SetStyling(text_log_->GetLineLength(a), 203); break;
		default: break;
		}
	}
	text_log_->SetEditable(false);

	next_message_index_ = Log::history().size();
	text_log_->ScrollToEnd();

	// Check again in 100ms
	timer_update_.Start(100);
}


// ----------------------------------------------------------------------------
//
// ConsolePanel Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ConsolePanel::onCommandEnter
//
// Called when the enter key is pressed in the command text box
// ----------------------------------------------------------------------------
void ConsolePanel::onCommandEnter(wxCommandEvent& e)
{
	App::console()->execute(e.GetString());
	update();
	text_command_->Clear();
	cmd_log_index_ = 0;
}

// ----------------------------------------------------------------------------
// ConsolePanel::onCommandKeyDown
//
// Called when a key is pressed in the command text box
// ----------------------------------------------------------------------------
void ConsolePanel::onCommandKeyDown(wxKeyEvent& e)
{
	if (e.GetKeyCode() == WXK_UP)
	{
		text_command_->SetValue(App::console()->prevCommand(cmd_log_index_));
		text_command_->SetInsertionPointEnd();
		if (cmd_log_index_ < App::console()->numPrevCommands()-1)
			cmd_log_index_++;
	}
	else if (e.GetKeyCode() == WXK_DOWN)
	{
		cmd_log_index_--;
		text_command_->SetValue(App::console()->prevCommand(cmd_log_index_));
		text_command_->SetInsertionPointEnd();
		if (cmd_log_index_ < 0)
			cmd_log_index_ = 0;
	}
	else
		e.Skip();
}
