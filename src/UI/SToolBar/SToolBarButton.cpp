
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
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
	action_{ SAction::fromId(action.ToStdString()) },
	show_name_{ show_name },
	action_id_{ action_->id() },
	action_name_{ action_->text() },
	help_text_{ action_->helpText() },
	pad_outer_{ ui::scalePx(1) },
	pad_inner_{ ui::scalePx(2) },
	icon_size_{ ui::scalePx(toolbar_size) }
{
	// Determine width of name text if shown
	if (show_name)
	{
		wxString name = action_->text();
		name.Replace("&", "");
		text_width_ = GetTextExtent(name).GetWidth() + pad_inner_ * 2;
	}

	// Set size
	updateSize();

	// Load icon
	if (icon.IsEmpty())
		icon_ = icons::getIcon(icons::Any, action_->iconName(), icon_size_);
	else
		icon_ = icons::getIcon(icons::Any, icon.ToStdString(), icon_size_);

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
	bool            show_name,
	int             icon_size) :
	wxControl{ parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, wxDefaultValidator, "stbutton" },
	show_name_{ show_name },
	action_id_{ action_id },
	action_name_{ action_name },
	help_text_{ help_text },
	pad_outer_{ ui::scalePx(1) },
	pad_inner_{ ui::scalePx(2) },
	icon_size_{ ui::scalePx(icon_size < 0 ? toolbar_size : icon_size) }
{
	// Determine width of name text if shown
	text_width_ = show_name ? GetTextExtent(action_name).GetWidth() + pad_inner_ * 2 : 0;

	// Set size
	updateSize();

	// Load icon
	icon_ = icons::getIcon(icons::Any, icon.ToStdString(), icon_size_);

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
	Bind(wxEVT_IDLE, [this](wxIdleEvent&) { updateState(); });
}

// -----------------------------------------------------------------------------
// Returns whether this button is checked
// -----------------------------------------------------------------------------
bool SToolBarButton::isChecked() const
{
	return action_ ? action_->isChecked() : checked_;
}

// -----------------------------------------------------------------------------
// Allows to dynamically change the button's icon
// -----------------------------------------------------------------------------
void SToolBarButton::setIcon(const wxString& icon)
{
	if (!icon.IsEmpty())
		icon_ = icons::getIcon(icons::Any, icon.ToStdString(), icon_size_);
}

// -----------------------------------------------------------------------------
// Sets the button's checked state (in the associated SAction if any)
// -----------------------------------------------------------------------------
void SToolBarButton::setChecked(bool checked)
{
	if (action_)
		action_->setChecked(checked);
	else
	{
		checked_ = checked;
		Update();
		Refresh();
	}
}

// -----------------------------------------------------------------------------
// Sets dropdown menu for the button
// -----------------------------------------------------------------------------
void SToolBarButton::setMenu(wxMenu* menu, bool delete_existing)
{
	if (menu_dropdown_ && delete_existing)
		delete menu_dropdown_;

	menu_dropdown_ = menu;
	SetToolTip("");
	updateSize();
}

// -----------------------------------------------------------------------------
// Checks if the mouseover state of the button needs updating.
// If it does, the button is refreshed and this returns true
// -----------------------------------------------------------------------------
bool SToolBarButton::updateState()
{
	auto prev_state = state_;

	if (GetScreenRect().Contains(wxGetMousePosition()) && IsEnabled())
	{
		if (wxGetMouseState().LeftIsDown())
			state_ = State::MouseDown;
		else
			state_ = State::MouseOver;
	}
	else
		state_ = State::Normal;

	if (prev_state != state_)
	{
		Update();
		Refresh();
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Returns the pixel height of all SToolBarButtons
// -----------------------------------------------------------------------------
int SToolBarButton::pixelHeight()
{
	return ui::scalePx(toolbar_size + 8);
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
// Updates the buttons size
// -----------------------------------------------------------------------------
void SToolBarButton::updateSize()
{
	int height = pad_outer_ * 2 + pad_inner_ * 2 + icon_size_;
	int width  = height + text_width_;
	if (menu_dropdown_)
		width += icon_size_ * 0.6;

	wxWindowBase::SetSizeHints(width, height, width, height);
	wxWindowBase::SetMinSize(wxSize(width, height));
	SetSize(width, height);
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
	auto scale_px = ui::scaleFactor();
	if (menu_dropdown_)
		width += icon_size_ * 0.6;

	// Draw toggled border/background
	if (isChecked())
	{
		//// Use greyscale version of hilight colour
		// uint8_t r = col_hilight.Red();
		// uint8_t g = col_hilight.Green();
		// uint8_t b = col_hilight.Blue();
		// wxColour::MakeGrey(&r, &g, &b);
		// wxColour col_toggle(r, g, b, 255);
		// wxColour col_trans(r, g, b, 150);

		// Draw border
		// col_trans.Set(col_trans.Red(), col_trans.Green(), col_trans.Blue(), 80);
		gc->SetBrush(*wxTRANSPARENT_BRUSH);
		gc->SetPen(wxPen(col_hilight, scale_px));
		gc->DrawRoundedRectangle(pad_outer_, pad_outer_, width - 1., height - 1., ui::scalePxU(1));
	}

	// Draw border on mouseover
	if (state_ == State::MouseOver || state_ == State::MouseDown)
	{
		// Determine transparency level
		int trans = 80;
		if (state_ == State::MouseDown)
			trans = 160;

		// Create semitransparent hilight colour
		wxColour col_trans(col_hilight.Red(), col_hilight.Green(), col_hilight.Blue(), trans);

		// Draw border
		gc->SetBrush(col_trans);
		gc->SetPen(*wxTRANSPARENT_PEN);
		gc->DrawRoundedRectangle(pad_outer_, pad_outer_, width, height, ui::scalePxU(1));
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

	if (menu_dropdown_)
	{
		static auto arrow_down = icons::getInterfaceIcon("arrow-down", icon_size_ * 0.75);

		gc->DrawBitmap(
			arrow_down,
			width - arrow_down.GetWidth(),
			height / 2. - arrow_down.GetHeight() / 2.,
			arrow_down.GetWidth(),
			arrow_down.GetHeight());

		/*gc->SetBrush(*wxTRANSPARENT_BRUSH);
		gc->SetPen(wxPen(dc.GetTextForeground(), 1));
		wxPoint2DDouble points[] = {
			wxPoint2DDouble(width - 9. * scale_px, height / 2. - 1.),
			wxPoint2DDouble(width - 6. * scale_px, height / 2. + 2.),
			wxPoint2DDouble(width - 3. * scale_px, height / 2. - 1.),
		};
		gc->DrawLines(3, points);*/
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
		// Set status bar help text
		if (parent_window && parent_window->GetStatusBar())
			parent_window->SetStatusText(help_text_);
	}

	// Mouse leave
	if (e.GetEventType() == wxEVT_LEAVE_WINDOW)
	{
		// Clear status bar help text
		if (parent_window && parent_window->GetStatusBar())
			parent_window->SetStatusText("");
	}

	// Left button down
	if (e.GetEventType() == wxEVT_LEFT_DOWN && menu_dropdown_)
		PopupMenu(menu_dropdown_, 0, GetSize().y);

	// Left button up
	if (e.GetEventType() == wxEVT_LEFT_UP && !menu_dropdown_)
	{
		if (state_ == State::MouseDown)
		{
			if (action_)
			{
				if (action_->isRadio())
					GetParent()->Refresh();

				SActionHandler::doAction(action_->id());
			}
			else
				sendClickedEvent();
		}

		// Clear status bar help text
		if (parent_window && parent_window->GetStatusBar())
			parent_window->SetStatusText("");
	}

	updateState();

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
