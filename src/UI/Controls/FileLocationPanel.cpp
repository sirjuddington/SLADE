
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    FileLocationPanel.cpp
// Description: A simple panel with a text box and button to browse for a file
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
#include "FileLocationPanel.h"
#include "SIconButton.h"
#include "UI/WxUtils.h"

using namespace slade;

wxDEFINE_EVENT(wxEVT_COMMAND_FLP_LOCATION_CHANGED, wxCommandEvent);

// -----------------------------------------------------------------------------
//
// FileLocationPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// FileLocationPanel class constructor
// -----------------------------------------------------------------------------
FileLocationPanel::FileLocationPanel(
	wxWindow*     parent,
	const string& path,
	bool          editable,
	const string& browse_caption,
	const string& browse_extensions,
	const string& browse_default_filename) :
	wxPanel(parent, -1),
	browse_caption_{ browse_caption },
	browse_extensions_{ browse_extensions },
	browse_default_filename_{ browse_default_filename }
{
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);
	text_path_ = new wxTextCtrl(
		this, -1, wxString::FromUTF8(path), wxDefaultPosition, wxDefaultSize, editable ? 0 : wxTE_READONLY);
	sizer->Add(text_path_, wxSizerFlags(1).Expand());

	btn_browse_ = new SIconButton(this, "open", browse_caption);
	sizer->Add(btn_browse_, wxSizerFlags(0).Expand().Border(wxLEFT, ui::px(ui::Size::PadMinimum)));

	btn_browse_->Bind(
		wxEVT_BUTTON,
		[&](wxCommandEvent& e)
		{
			filedialog::FDInfo inf;
			if (filedialog::openFile(inf, browse_caption_, browse_extensions_, this, browse_default_filename_))
			{
				auto fn_wx = wxString::FromUTF8(inf.filenames[0]);
				text_path_->SetValue(fn_wx);
				wxCommandEvent event(wxEVT_COMMAND_FLP_LOCATION_CHANGED, GetId());
				event.SetEventObject(this);
				event.SetString(fn_wx);
				GetEventHandler()->ProcessEvent(event);
			}
		});

	text_path_->Bind(
		wxEVT_TEXT,
		[&](wxCommandEvent& e)
		{
			wxCommandEvent event(wxEVT_COMMAND_FLP_LOCATION_CHANGED, GetId());
			event.SetEventObject(this);
			event.SetString(text_path_->GetValue());
			GetEventHandler()->ProcessEvent(event);
		});
}

// -----------------------------------------------------------------------------
// Returns the currently selected file path
// -----------------------------------------------------------------------------
string FileLocationPanel::location() const
{
	return text_path_->GetValue().utf8_string();
}

// -----------------------------------------------------------------------------
// Sets the selected file [path]
// -----------------------------------------------------------------------------
void FileLocationPanel::setLocation(const string& path) const
{
	text_path_->SetValue(wxString::FromUTF8(path));
}
