
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ConsolePanel.cpp
 * Description: UI Frontend panel for the console
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "ConsolePanel.h"
#include "General/Console/Console.h"
#include "UI/WxStuff.h"
#include "App.h"


/*******************************************************************
 * CONSOLEPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ConsolePanel::ConsolePanel
 * ConsolePanel class constructor
 *******************************************************************/
ConsolePanel::ConsolePanel(wxWindow* parent, int id)
	: wxPanel(parent, id)
{
	// Init variables
	cmd_log_index = 0;
	next_message_index = 0;

	// Setup layout
	initLayout();

	// Bind events
	text_command->Bind(wxEVT_TEXT_ENTER, &ConsolePanel::onCommandEnter, this);
	text_command->Bind(wxEVT_KEY_DOWN, &ConsolePanel::onCommandKeyDown, this);

	// Start update timer
	timer_update.Bind(wxEVT_TIMER, [&](wxTimerEvent&){ update(); });
	timer_update.Start(100);
}

/* ConsolePanel::~ConsolePanel
 * ConsolePanel class destructor
 *******************************************************************/
ConsolePanel::~ConsolePanel()
{
}

/* ConsolePanel::initLayout
 * Sets up the panel layout
 *******************************************************************/
void ConsolePanel::initLayout()
{
	// Create and set the sizer for the panel
	auto vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(vbox);

	// Create and add the message log textbox
	text_log = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	text_log->SetSizeHints(wxSize(-1, 0));
	vbox->Add(text_log, 1, wxEXPAND | wxALL, 4);

	// Create and add the command entry textbox
	text_command = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	vbox->Add(text_command, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 4);

	Layout();

	// Set console font to default+monospace
	auto font = getMonospaceFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
	text_log->SetFont(font);
	text_command->SetFont(font);
}

/* ConsolePanel::update
 * Update the log text with any new log messages
 *******************************************************************/
void ConsolePanel::update()
{
	// Check if any new log messages were added since the last update
	auto& log = Log::history();
	if (log.size() <= next_message_index)
	{
		// None added, check again in 500ms
		timer_update.Start(500);
		return;
	}

	// Get combined string of new messages
	string new_logs = (next_message_index == 0) ? "" : "\n";
	for (auto a = next_message_index; a < log.size(); a++)
		new_logs += log[a].formattedMessageLine() + "\n";
	new_logs.RemoveLast(1); // Remove ending newline

	// Append to text box
	text_log->AppendText(new_logs);
	next_message_index = Log::history().size();

	// Check again in 100ms
	timer_update.Start(100);
}


/*******************************************************************
 * CONSOLEPANEL EVENTS
 *******************************************************************/

/* ConsolePanel::onCommandEnter
 * Called when the enter key is pressed in the command text box
 *******************************************************************/
void ConsolePanel::onCommandEnter(wxCommandEvent& e)
{
	App::console()->execute(e.GetString());
	update();
	text_command->Clear();
	cmd_log_index = 0;
}

/* ConsolePanel::onCommandKeyDown
 * Called when a key is pressed in the command text box
 *******************************************************************/
void ConsolePanel::onCommandKeyDown(wxKeyEvent& e)
{
	if (e.GetKeyCode() == WXK_UP)
	{
		text_command->SetValue(App::console()->prevCommand(cmd_log_index));
		text_command->SetInsertionPointEnd();
		if (cmd_log_index < App::console()->numPrevCommands()-1)
			cmd_log_index++;
	}
	else if (e.GetKeyCode() == WXK_DOWN)
	{
		cmd_log_index--;
		text_command->SetValue(App::console()->prevCommand(cmd_log_index));
		text_command->SetInsertionPointEnd();
		if (cmd_log_index < 0)
			cmd_log_index = 0;
	}
	else
		e.Skip();
}
