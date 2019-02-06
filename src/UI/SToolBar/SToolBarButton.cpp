
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SToolBarButton.cpp
// Description: SToolBarButton class - a simple toolbar button for use on an
//              SToolBar, can be displayed as an icon or icon + text
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
#include "SToolBarButton.h"
#include "Graphics/Icons.h"
#include "MainEditor/UI/MainWindow.h"
#include "OpenGL/Drawing.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, toolbar_button_flat, true, CVar::Flag::Save)
wxDEFINE_EVENT(wxEVT_STOOLBAR_BUTTON_CLICKED, wxCommandEvent);


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, toolbar_size);


// -----------------------------------------------------------------------------
//
// SToolBarButton Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SToolBarButton class constructor
// -----------------------------------------------------------------------------
SToolBarButton::SToolBarButton(wxWindow* parent, const wxString& action, const wxString& icon, bool show_name) :
	wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, wxDefaultValidator, "stbutton"),
	action_{ SAction::fromId(action) },
	show_name_{ show_name },
	action_id_{ action_->id() },
	action_name_{ action_->text() },
	help_text_{ action_->helpText() },
	pad_outer_{ UI::scalePx(1) },
	pad_inner_{ UI::scalePx(2) },
	icon_size_{ UI::scalePx(toolbar_size) }
{
	// Determine width of name text if shown
	if (show_name)
	{
		wxString name = action_->text();
		name.Replace("&", "");
		text_width_ = GetTextExtent(name).GetWidth() + pad_inner_ * 2;
	}

	// Set size
	int height = pad_outer_ * 2 + pad_inner_ * 2 + icon_size_;
	int width  = height + text_width_;
	wxWindowBase::SetSizeHints(width, height, width, height);
	wxWindowBase::SetMinSize(wxSize(width, height));
	SetSize(width, height);

	// Load icon
	if (icon.IsEmpty())
		icon_ = Icons::getIcon(Icons::General, action_->iconName(), icon_size_ > 16);
	else
		icon_ = Icons::getIcon(Icons::General, icon, icon_size_ > 16);

	// Add shortcut to help text if it exists
	wxString sc = action_->shortcutText();
	if (!sc.IsEmpty())
		help_text_ += wxString::Format(" (Shortcut: %s)", sc);

	// Set tooltip
	if (!show_name)
	{
		wxString tip = action_->text();
		tip.Replace("&", "");
		if (!sc.IsEmpty())
			tip += wxString::Format(" (Shortcut: %s)", sc);
		SetToolTip(tip);
	}
	else if (!sc.IsEmpty())
		SetToolTip(wxString::Format("Shortcut: %s", sc));

	// Bind events
	Bind(wxEVT_PAINT, &SToolBarButton::onPaint, this);
	Bind(wxEVT_ENTER_WINDOW, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_LEAVE_WINDOW, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_LEFT_DOWN, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_LEFT_UP, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_KILL_FOCUS, &SToolBarButton::onFocus, this);
	Bind(wxEVT_LEFT_DCLICK, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_ERASE_BACKGROUND, &SToolBarButton::onEraseBackground, this);
}

// -----------------------------------------------------------------------------
// SToolBarButton class constructor
// -----------------------------------------------------------------------------
SToolBarButton::SToolBarButton(
	wxWindow*       parent,
	const wxString& action_id,
	const wxString& action_name,
	const wxString& icon,
	const wxString& help_text,
	bool            show_name) :
	wxControl{ parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, wxDefaultValidator, "stbutton" },
	show_name_{ show_name },
	action_id_{ action_id },
	action_name_{ action_name },
	help_text_{ help_text },
	pad_outer_{ UI::scalePx(1) },
	pad_inner_{ UI::scalePx(2) },
	icon_size_{ UI::scalePx(toolbar_size) }
{
	// Determine width of name text if shown
	text_width_ = show_name ? GetTextExtent(action_name).GetWidth() + pad_inner_ * 2 : 0;

	// Set size
	int height = pad_outer_ * 2 + pad_inner_ * 2 + icon_size_;
	int width  = height + text_width_;
	wxWindowBase::SetSizeHints(width, height, width, height);
	wxWindowBase::SetMinSize(wxSize(width, height));
	SetSize(width, height);

	// Load icon
	icon_ = Icons::getIcon(Icons::General, icon, icon_size_ > 16);

	// Set tooltip
	if (!show_name)
		SetToolTip(action_name);

	// Bind events
	Bind(wxEVT_PAINT, &SToolBarButton::onPaint, this);
	Bind(wxEVT_ENTER_WINDOW, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_LEAVE_WINDOW, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_LEFT_DOWN, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_LEFT_UP, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_KILL_FOCUS, &SToolBarButton::onFocus, this);
	Bind(wxEVT_LEFT_DCLICK, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_ERASE_BACKGROUND, &SToolBarButton::onEraseBackground, this);
}

// -----------------------------------------------------------------------------
// Allows to dynamically change the button's icon
// -----------------------------------------------------------------------------
void SToolBarButton::setIcon(const wxString& icon)
{
	if (!icon.IsEmpty())
		icon_ = Icons::getIcon(Icons::General, icon, icon_size_ > 16);
}

// -----------------------------------------------------------------------------
// Returns the pixel height of all SToolBarButtons
// -----------------------------------------------------------------------------
int SToolBarButton::pixelHeight()
{
	return UI::scalePx(toolbar_size + 8);
}

// -----------------------------------------------------------------------------
// Sends a button clicked event
// -----------------------------------------------------------------------------
void SToolBarButton::sendClickedEvent()
{
	// Generate event
	wxCommandEvent ev(wxEVT_STOOLBAR_BUTTON_CLICKED, GetId());
	ev.SetEventObject(this);
	ev.SetString(action_id_);
	ProcessWindowEvent(ev);
}


// -----------------------------------------------------------------------------
//
// SToolBarButton Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the button needs to be (re)drawn
// -----------------------------------------------------------------------------
void SToolBarButton::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);

	// Get system colours needed
	auto col_background = GetBackgroundColour();
	auto col_hilight    = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

	// Draw background
	dc.SetBackground(wxBrush(col_background));
	dc.Clear();

	// Create graphics context
	auto gc = wxGraphicsContext::Create(dc);
	if (!gc)
		return;

	// Get width of name text if shown
	int      name_height = 0;
	wxString name        = action_name_;
	if (show_name_)
	{
		name.Replace("&", "");
		auto name_size = GetTextExtent(name);
		name_height    = name_size.y;
	}

	int  height   = icon_size_ + pad_inner_ * 2;
	int  width    = height + text_width_;
	auto scale_px = UI::scaleFactor();

	// Draw toggled border/background
	if (action_ && action_->isChecked())
	{
		// Use greyscale version of hilight colour
		uint8_t r = col_hilight.Red();
		uint8_t g = col_hilight.Green();
		uint8_t b = col_hilight.Blue();
		wxColour::MakeGrey(&r, &g, &b);
		wxColour col_toggle(r, g, b, 255);
		wxColour col_trans(r, g, b, 150);

		if (toolbar_button_flat)
		{
			// Draw border
			col_trans.Set(COLWX(col_trans), 80);
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(Drawing::darkColour(col_toggle, 5.0f), scale_px));
			gc->DrawRectangle(pad_outer_, pad_outer_, width, height);
		}
		else
		{
			// Draw border
			col_trans.Set(COLWX(col_trans), 80);
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(col_hilight, scale_px));
			gc->DrawRoundedRectangle(pad_outer_, pad_outer_, width, height, 2 * scale_px);
		}
	}

	// Draw border on mouseover
	if (state_ == State::MouseOver || state_ == State::MouseDown)
	{
		// Determine transparency level
		int trans = 160;
		if (state_ == State::MouseDown)
			trans = 200;

		// Create semitransparent hilight colour
		wxColour col_trans(COLWX(col_hilight), trans);

		if (toolbar_button_flat)
		{
			// Draw border
			col_trans.Set(COLWX(col_trans), 80);
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(col_hilight, scale_px));
			gc->DrawRectangle(pad_outer_, pad_outer_, width, height);
		}
		else
		{
			// Draw border
			col_trans.Set(COLWX(col_trans), 80);
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(col_hilight));
			gc->DrawRoundedRectangle(pad_outer_, pad_outer_, width, height, 2 * scale_px);
		}
	}

	if (icon_.IsOk())
	{
		// Draw disabled icon if disabled
		if (!IsEnabled())
		{
			// Determine toolbar background brightness
			uint8_t r, g, b;
			r = col_background.Red();
			g = col_background.Green();
			b = col_background.Blue();
			wxColor::MakeGrey(&r, &g, &b);

			// Draw disabled icon
			gc->DrawBitmap(
				icon_.ConvertToDisabled(r), pad_outer_ + pad_inner_, pad_outer_ + pad_inner_, icon_size_, icon_size_);
		}

		// Otherwise draw normal icon
		else
			gc->DrawBitmap(icon_, pad_outer_ + pad_inner_, pad_outer_ + pad_inner_, icon_size_, icon_size_);
	}

	if (show_name_)
	{
		int top  = ((double)GetSize().y * 0.5) - ((double)name_height * 0.5);
		int left = pad_outer_ * 2 + pad_inner_ * 2 + icon_size_;
		dc.DrawText(name, left, top);
	}

	delete gc;
}

// -----------------------------------------------------------------------------
// Called when a mouse event happens within the control
// -----------------------------------------------------------------------------
void SToolBarButton::onMouseEvent(wxMouseEvent& e)
{
	auto parent_window = dynamic_cast<wxFrame*>(wxGetTopLevelParent(this));

	// Mouse enter
	if (e.GetEventType() == wxEVT_ENTER_WINDOW)
	{
		// Set state to mouseover
		state_ = State::MouseOver;

		// Set status bar help text
		if (parent_window && parent_window->GetStatusBar())
			parent_window->SetStatusText(help_text_);
	}

	// Mouse leave
	if (e.GetEventType() == wxEVT_LEAVE_WINDOW)
	{
		// Set state to normal
		state_ = State::Normal;

		// Clear status bar help text
		if (parent_window && parent_window->GetStatusBar())
			parent_window->SetStatusText("");
	}

	// Left button down
	if (e.GetEventType() == wxEVT_LEFT_DOWN || e.GetEventType() == wxEVT_LEFT_DCLICK)
	{
		state_ = State::MouseDown;

		if (action_)
		{
			if (action_->isRadio())
				GetParent()->Refresh();
			SActionHandler::doAction(action_->id());
		}
		else
			sendClickedEvent();
	}

	// Left button up
	if (e.GetEventType() == wxEVT_LEFT_UP)
	{
		state_ = State::MouseOver;

		// Clear status bar help text
		if (parent_window && parent_window->GetStatusBar())
			parent_window->SetStatusText("");
	}

	Refresh();

	// e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the control gains or loses focus
// -----------------------------------------------------------------------------
void SToolBarButton::onFocus(wxFocusEvent& e)
{
	// Redraw
	state_ = State::Normal;
	Update();
	Refresh();

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the background needs erasing
// -----------------------------------------------------------------------------
void SToolBarButton::onEraseBackground(wxEraseEvent& e) {}
