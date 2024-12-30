
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "General/SAction.h"
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
	pad_outer_{ 1 },
	pad_inner_{ 2 },
	icon_size_{ toolbar_size },
	text_offset_{ parent->FromDIP(2) }
{
	action_name_.Replace("&", "");

	setup(show_name, icon.IsEmpty() ? action_->iconName() : wxutil::strToView(icon));

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
	pad_outer_{ 1 },
	pad_inner_{ 2 },
	icon_size_{ icon_size < 0 ? toolbar_size : icon_size },
	text_offset_{ parent->FromDIP(2) }
{
	setup(show_name, wxutil::strToView(icon));

	// Set tooltip
	if (!show_name)
		SetToolTip(action_name);
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

	// Refresh when menu is closed
	menu->Bind(
		wxEVT_MENU_CLOSE,
		[this](wxMenuEvent&)
		{
			menu_open_ = false;
			pressed_   = false;
			Update();
			Refresh();
		});

	menu_dropdown_ = menu;
	SetToolTip("");
	updateSize();
}

// -----------------------------------------------------------------------------
// Sets the font size (scale) for the button text
// -----------------------------------------------------------------------------
void SToolBarButton::setFontSize(float scale)
{
	SetFont(GetFont().Scale(scale));
	text_width_ = ToDIP(GetTextExtent(action_name_).GetWidth()) + pad_inner_ * 2;
	updateSize();
}

// -----------------------------------------------------------------------------
// Sets the button padding
// -----------------------------------------------------------------------------
void SToolBarButton::setPadding(int inner, int outer)
{
	pad_inner_ = inner;
	pad_outer_ = outer;
	updateSize();
}

// -----------------------------------------------------------------------------
// Sets 'exact fit' mode.
// If [fit] is true the button will automatically size to fit its contents
// -----------------------------------------------------------------------------
void SToolBarButton::setExactFit(bool fit)
{
	exact_fit_ = fit;
	updateSize();
}

// -----------------------------------------------------------------------------
// Sets the text offset (space between icon and text)
// -----------------------------------------------------------------------------
void SToolBarButton::setTextOffset(int offset)
{
	text_offset_ = offset;
	updateSize();
}

// -----------------------------------------------------------------------------
// Returns the pixel height of all SToolBarButtons
// -----------------------------------------------------------------------------
int SToolBarButton::pixelHeight()
{
	return toolbar_size + 8;
}

// -----------------------------------------------------------------------------
// Setup the SToolBarButton (icon, text, size and events)
// -----------------------------------------------------------------------------
void SToolBarButton::setup(bool show_name, string_view icon)
{
	// Double buffer to avoid flicker
	SetDoubleBuffered(true);

	// Determine width of name text if shown
	if (show_name)
		text_width_ = ToDIP(GetTextExtent(action_name_).GetWidth()) + pad_inner_ * 2;

	// Set size
	updateSize();

	// Load icon
	icon_ = icons::getIcon(icons::Any, icon, icon_size_);

	// Bind events
	Bind(wxEVT_PAINT, &SToolBarButton::onPaint, this);
	Bind(wxEVT_ENTER_WINDOW, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_LEAVE_WINDOW, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_LEFT_DOWN, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_LEFT_UP, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_LEFT_DCLICK, &SToolBarButton::onMouseEvent, this);
	Bind(wxEVT_ERASE_BACKGROUND, &SToolBarButton::onEraseBackground, this);
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
	int height    = pad_outer_ * 2 + pad_inner_ * 2 + icon_size_;
	int min_width = height + text_width_;
	if (text_width_ > 0)
		min_width += pad_inner_ + text_offset_;
	if (menu_dropdown_)
		min_width += icon_size_ * 0.6;

	auto width = FromDIP(exact_fit_ ? min_width : -1);
	min_width  = FromDIP(min_width);
	height     = FromDIP(height);

	wxWindowBase::SetSizeHints(min_width, height, width, height);
	wxWindowBase::SetMinSize(wxSize(min_width, height));
	SetSize(width, height);
}

// -----------------------------------------------------------------------------
// Draws the button content using [gc].
// If [mouse_over] is true, the button is being hovered over
// -----------------------------------------------------------------------------
void SToolBarButton::drawContent(wxGraphicsContext* gc, bool mouse_over)
{
	// Get system colours needed
	auto col_background = GetBackgroundColour();
	auto col_hilight    = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

	// Get width of name text if shown
	int name_height = 0;
	if (show_name_)
	{
		auto name_size = GetTextExtent(action_name_);
		name_height    = name_size.y;
	}

	auto width        = GetSize().x;
	auto width_inner  = width - (2. * pad_outer_);
	auto height       = GetSize().y;
	auto height_inner = height - (2. * pad_outer_);

	// Draw toggled border/background
	if (isChecked())
	{
		// Draw border
		gc->SetBrush(*wxTRANSPARENT_BRUSH);
		gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(col_hilight, 1.5)));
		gc->DrawRoundedRectangle(pad_outer_, pad_outer_, width_inner, height_inner, FromDIP(2));
	}

	// Draw border on mouseover
	if (mouse_over || pressed_ || menu_open_ || (isChecked() && fill_checked_))
	{
		// Determine transparency level
		int trans = 80;
		if (pressed_ || menu_open_ || isChecked())
			trans = 160;

		// Create semitransparent hilight colour
		wxColour col_trans(col_hilight.Red(), col_hilight.Green(), col_hilight.Blue(), trans);

		// Draw border
		gc->SetBrush(col_trans);
		gc->SetPen(*wxTRANSPARENT_PEN);
		gc->DrawRoundedRectangle(pad_outer_, pad_outer_, width_inner, height_inner, FromDIP(2));
	}

	auto icon = icon_.GetBitmapFor(this);

	if (icon.IsOk())
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
				icon.ConvertToDisabled(r),
				FromDIP(pad_outer_ + pad_inner_),
				FromDIP(pad_outer_ + pad_inner_),
				FromPhys(icon.GetWidth()),
				FromPhys(icon.GetHeight()));
		}

		// Otherwise draw normal icon
		else
			gc->DrawBitmap(
				icon,
				FromDIP(pad_outer_ + pad_inner_),
				FromDIP(pad_outer_ + pad_inner_),
				FromPhys(icon.GetWidth()),
				FromPhys(icon.GetHeight()));
	}

	if (show_name_)
	{
		int top  = (static_cast<double>(GetSize().y) * 0.5) - (static_cast<double>(name_height) * 0.5);
		int left = pad_outer_ + pad_inner_ * 2 + icon_size_ + text_offset_;
		gc->DrawText(action_name_, FromDIP(left), top);
	}

	if (menu_dropdown_)
	{
		static auto arrow_down = icons::getInterfaceIcon("arrow-down", icon_size_ * 0.75).GetBitmapFor(this);

		auto a_width  = FromPhys(arrow_down.GetWidth());
		auto a_height = FromPhys(arrow_down.GetHeight());

		gc->DrawBitmap(arrow_down, width - a_width - pad_outer_, height / 2. - a_height / 2., a_width, a_height);
	}
}


// -----------------------------------------------------------------------------
//
// SToolBarButton Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the button needs to be (re)drawn
// -----------------------------------------------------------------------------
void SToolBarButton::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);

	// Check if the mouse is within the button
	auto mouse_pos  = wxGetMousePosition();
	bool mouse_over = GetClientRect().Contains(ScreenToClient(mouse_pos));

	// Draw background
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();

	// Create graphics context
	auto gc = wxGraphicsContext::Create(dc);
	if (!gc)
		return;

	// Draw button content
	gc->SetFont(GetFont(), GetForegroundColour());
	drawContent(gc, mouse_over);

	delete gc;
}

// -----------------------------------------------------------------------------
// Called when a mouse event happens within the control
// -----------------------------------------------------------------------------
void SToolBarButton::onMouseEvent(wxMouseEvent& e)
{
	auto parent_window = dynamic_cast<wxFrame*>(wxGetTopLevelParent(this));
	bool refresh       = false;

	// Mouse enter
	if (e.GetEventType() == wxEVT_ENTER_WINDOW)
	{
		// Set status bar help text
		if (parent_window && parent_window->GetStatusBar())
			parent_window->SetStatusText(help_text_);

		refresh = true;
	}

	// Mouse leave
	if (e.GetEventType() == wxEVT_LEAVE_WINDOW)
	{
		// Clear status bar help text
		if (parent_window && parent_window->GetStatusBar())
			parent_window->SetStatusText("");

		pressed_ = false;
		refresh  = true;
	}

	// Left button down
	if (e.GetEventType() == wxEVT_LEFT_DOWN)
	{
		pressed_ = true;
		refresh  = true;

		if (menu_dropdown_)
		{
			menu_open_ = true;
			Update();
			Refresh();
			PopupMenu(menu_dropdown_, 0, GetSize().y);
		}
	}

	// Left button up
	if (e.GetEventType() == wxEVT_LEFT_UP && !menu_dropdown_)
	{
		if (pressed_)
		{
			if (action_)
			{
				if (action_->isRadio())
					GetParent()->Refresh();

				SActionHandler::setWxIdOffset(action_wx_id_offset_);
				SActionHandler::doAction(action_->id());
			}
			else
				sendClickedEvent();

			if (click_can_delete_)
				return;

			pressed_ = false;
			refresh  = true;
		}

		// Clear status bar help text
		if (parent_window && parent_window->GetStatusBar())
			parent_window->SetStatusText("");
	}

	if (refresh)
	{
		Update();
		Refresh();
	}
}

// -----------------------------------------------------------------------------
// Called when the background needs erasing
// -----------------------------------------------------------------------------
void SToolBarButton::onEraseBackground(wxEraseEvent& e) {}
