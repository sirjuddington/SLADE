
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ExtMessageDialog.cpp
 * Description: A simple message dialog that displays a short message
 *              and a scrollable extended text area, used to present
 *              potentially lengthy text (error logs, etc)
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
#include "WxStuff.h"
#include "ExtMessageDialog.h"


/*******************************************************************
 * EXTMESSAGEDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* ExtMessageDialog::ExtMessageDialog
 * ExtMessageDialog class constructor
 *******************************************************************/
ExtMessageDialog::ExtMessageDialog(wxWindow* parent, string caption) : wxDialog(parent, -1, caption, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	// Create and set sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add message label
	label_message = new wxStaticText(this, -1, "", wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	sizer->Add(label_message, 0, wxEXPAND|wxALL, 10);

	// Add extended text box
	text_ext = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY);
	text_ext->SetFont(wxFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	sizer->Add(text_ext, 1, wxEXPAND|wxALL, 10);

	// Add buttons
	sizer->Add(CreateStdDialogButtonSizer(wxOK), 0, wxEXPAND|wxALL, 10);

	SetInitialSize(wxSize(500, 500));

	// Bind events
	Bind(wxEVT_SIZE, &ExtMessageDialog::onSize, this);
}

/* ExtMessageDialog::~ExtMessageDialog
 * ExtMessageDialog class destructor
 *******************************************************************/
ExtMessageDialog::~ExtMessageDialog()
{
}

/* ExtMessageDialog::setMessage
 * Sets the dialog short message
 *******************************************************************/
void ExtMessageDialog::setMessage(string message)
{
	label_message->SetLabel(message);
}

/* ExtMessageDialog::setExt
 * Sets the dialog extended text
 *******************************************************************/
void ExtMessageDialog::setExt(string text)
{
	text_ext->SetValue(text);
}


/*******************************************************************
 * EXTMESSAGEDIALOG CLASS EVENTS
 *******************************************************************/

/* ExtMessageDialog::onSize
 * Called when the dialog is resized
 *******************************************************************/
void ExtMessageDialog::onSize(wxSizeEvent& e)
{
	Layout();
	label_message->Wrap(label_message->GetSize().GetWidth());
	Layout();
}
