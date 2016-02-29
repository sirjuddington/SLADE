
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
#include <wx/textctrl.h>
#include <wx/settings.h>
#include <wx/sizer.h>


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

	// Setup layout
	initLayout();

	// Listen to the console
	listenTo(theConsole);

	// Bind events
	text_command->Bind(wxEVT_TEXT_ENTER, &ConsolePanel::onCommandEnter, this);
	text_command->Bind(wxEVT_KEY_DOWN, &ConsolePanel::onCommandKeyDown, this);

	// Load the current contents of the console log
	text_log->AppendText(theConsole->dumpLog());
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
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
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
	wxFont font = getMonospaceFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
	text_log->SetFont(font);
	text_command->SetFont(font);
}

/* ConsolePanel::onAnnouncement
 * Handles any announcement events
 *******************************************************************/
void ConsolePanel::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	// New console log message added
	if (event_name == "console_logmessage")
	{
		text_log->AppendText(theConsole->lastLogLine());
	}
}


/*******************************************************************
 * CONSOLEPANEL EVENTS
 *******************************************************************/

/* ConsolePanel::onCommandEnter
 * Called when the enter key is pressed in the command text box
 *******************************************************************/
void ConsolePanel::onCommandEnter(wxCommandEvent& e)
{
	theConsole->execute(e.GetString());
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
		text_command->SetValue(theConsole->prevCommand(cmd_log_index));
		text_command->SetInsertionPointEnd();
		if (cmd_log_index < theConsole->numPrevCommands()-1)
			cmd_log_index++;
	}
	else if (e.GetKeyCode() == WXK_DOWN)
	{
		cmd_log_index--;
		text_command->SetValue(theConsole->prevCommand(cmd_log_index));
		text_command->SetInsertionPointEnd();
		if (cmd_log_index < 0)
			cmd_log_index = 0;
	}
	else
		e.Skip();
}
