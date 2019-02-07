
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SToolBar.cpp
// Description: SToolBar class - a custom toolbar implementation that allows
//              any kind of control to be placed on it and auto-arranges itself
//              based on groups.
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
#include "SToolBar.h"
#include "OpenGL/Drawing.h"
#include "SToolBarButton.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, show_toolbar_names, false, CVar::Flag::Save)
CVAR(String, toolbars_hidden, "", CVar::Flag::Save)
CVAR(Int, toolbar_size, 16, CVar::Flag::Save)
DEFINE_EVENT_TYPE(wxEVT_STOOLBAR_LAYOUT_UPDATED)


// -----------------------------------------------------------------------------
// SToolBarSeparator Class
//
// Simple control to use as a separator between toolbar groups
// -----------------------------------------------------------------------------
class SToolBarSeparator : public wxControl
{
public:
	SToolBarSeparator(wxWindow* parent) : wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
	{
		// Set size
		int height = UI::scalePx(toolbar_size + 6);
		int width  = UI::scalePx(4);
		wxWindowBase::SetSizeHints(width, height, width, height);
		wxWindowBase::SetMinSize(wxSize(width, height));
		SetSize(width, height);

		// Set window name
		wxWindowBase::SetName("tb_sep");

		// Bind events
		Bind(wxEVT_PAINT, &SToolBarSeparator::onPaint, this);
	}

	void onPaint(wxPaintEvent& e)
	{
		wxPaintDC dc(this);

		// Get system colours needed
		auto    col_background = GetBackgroundColour();
		ColRGBA bg(COLWX(col_background));
		auto    col_light = WXCOL(bg.amp(50, 50, 50, 0));
		auto    col_dark  = WXCOL(bg.amp(-50, -50, -50, 0));

		// Draw background
		dc.SetBackground(wxBrush(col_background));
		dc.Clear();

		// Draw separator lines
		int height = (toolbar_size / 16.0) * 11;
		dc.GradientFillLinear(WxUtils::scaledRect(1, 0, 1, height), col_background, col_dark, wxSOUTH);
		dc.GradientFillLinear(WxUtils::scaledRect(1, height, 1, height), col_background, col_dark, wxNORTH);
		dc.GradientFillLinear(WxUtils::scaledRect(2, 0, 1, height), col_background, col_light, wxSOUTH);
		dc.GradientFillLinear(WxUtils::scaledRect(2, height, 1, height), col_background, col_light, wxNORTH);
	}
};

// -----------------------------------------------------------------------------
// SToolBarVLine Class
//
// Used as a separator between toolbar rows
// -----------------------------------------------------------------------------
class SToolBarVLine : public wxControl
{
public:
	SToolBarVLine(wxWindow* parent) : wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
	{
		// Set size
		wxWindowBase::SetMaxSize(WxUtils::scaledSize(-1, 2));
		wxWindowBase::SetMinSize(WxUtils::scaledSize(-1, 2));

		// Set window name
		wxWindowBase::SetName("tb_vline");

		// if (toolbar_win10)
		//	SetBackgroundColour(*wxWHITE);

		// Bind events
		Bind(wxEVT_PAINT, &SToolBarVLine::onPaint, this);
	}

	void onPaint(wxPaintEvent& e)
	{
		wxPaintDC dc(this);

		// Get system colours needed
		auto col_background = GetBackgroundColour(); // toolbar_win10 ? *wxWHITE : Drawing::getPanelBGColour();
		auto col_light      = Drawing::lightColour(col_background, 1.5f);
		auto col_dark       = Drawing::darkColour(col_background, 1.5f);

		// Draw lines
		// dc.SetPen(wxPen(col_dark));
		// dc.DrawLine(wxPoint(0, 0), wxPoint(GetSize().x+1, 0));
		// dc.SetPen(wxPen(col_light));
		// dc.DrawLine(wxPoint(0, 1), wxPoint(GetSize().x+1, 1));
	}
};


// -----------------------------------------------------------------------------
//
// SToolBarGroup Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SToolBarGroup class constructor
// -----------------------------------------------------------------------------
SToolBarGroup::SToolBarGroup(SToolBar* parent, const wxString& name, bool force_name) :
	wxPanel(parent, -1),
	name_{ name }
{
	// Check if hidden
	wxString tb_hidden = toolbars_hidden;
	if (tb_hidden.Contains(wxString::Format("[%s]", name)))
		hidden_ = true;
	else
		hidden_ = false;

	// Set colour
	wxWindowBase::SetBackgroundColour(parent->GetBackgroundColour());

	// Create sizer
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Create group label if necessary
	if (show_toolbar_names || force_name)
	{
		wxString showname = name;
		if (name.StartsWith("_"))
			showname.Remove(0, 1);

		auto label = new wxStaticText(this, -1, wxString::Format("%s:", showname));
		label->SetForegroundColour(Drawing::systemMenuTextColour());
		sizer->AddSpacer(UI::pad());
		sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL);
		sizer->AddSpacer(UI::px(UI::Size::PadMinimum));
	}
}

// -----------------------------------------------------------------------------
// Hides the group if [hide] is true, otherwise shows it
// -----------------------------------------------------------------------------
void SToolBarGroup::hide(bool hide)
{
	// Update variables
	hidden_ = hide;

	// Update 'hidden toolbars' cvar
	wxString tb_hidden = toolbars_hidden;
	if (hide)
		tb_hidden += "[" + name_ + "]";
	else
		tb_hidden.Replace(wxString::Format("[%s]", name_), "");

	toolbars_hidden = tb_hidden;
}

// -----------------------------------------------------------------------------
// Adds a toolbar button to the group for [action]. If [icon] is empty, the
// action's icon is used
// -----------------------------------------------------------------------------
SToolBarButton* SToolBarGroup::addActionButton(const wxString& action, const wxString& icon, bool show_name)
{
	// Get sizer
	auto sizer = GetSizer();

	// Create button
	auto button = new SToolBarButton(this, action, icon, show_name);
	button->SetBackgroundColour(GetBackgroundColour());

	// Add it to the group
	sizer->Add(button, 0, wxALIGN_CENTER_VERTICAL | wxALL, UI::scalePx(1));

	return button;
}

// -----------------------------------------------------------------------------
// Adds a toolbar button to the group for [action_id]. [action_name], [icon]
// and [help_text] can be defined to override the defaults of the action
// -----------------------------------------------------------------------------
SToolBarButton* SToolBarGroup::addActionButton(
	const wxString& action_id,
	const wxString& action_name,
	const wxString& icon,
	const wxString& help_text,
	bool            show_name)
{
	// Get sizer
	auto sizer = GetSizer();

	// Create button
	auto button = new SToolBarButton(this, action_id, action_name, icon, help_text, show_name);
	button->SetBackgroundColour(GetBackgroundColour());
	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &SToolBarGroup::onButtonClicked, this, button->GetId());

	// Add it to the group
	sizer->Add(button, 0, wxALIGN_CENTER_VERTICAL | wxALL, UI::scalePx(1));

	return button;
}

// -----------------------------------------------------------------------------
// Adds a control to the group
// -----------------------------------------------------------------------------
void SToolBarGroup::addCustomControl(wxWindow* control)
{
	// Set the control's parent to this panel
	control->SetParent(this);

	// Add it to the group
	GetSizer()->Add(control, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, UI::scalePx(1));
}

// -----------------------------------------------------------------------------
// Redraws all controls in the group
// -----------------------------------------------------------------------------
void SToolBarGroup::redraw()
{
	auto children = GetChildren();
	for (auto window : children)
	{
		window->Update();
		window->Refresh();
	}
}

// -----------------------------------------------------------------------------
// Called when a toolbar button within the group is clicked
// -----------------------------------------------------------------------------
void SToolBarGroup::onButtonClicked(wxCommandEvent& e)
{
	// No idea why the event doesn't propagate as it's supposed to,
	// shouldn't need to do this
	wxCommandEvent ev(wxEVT_STOOLBAR_BUTTON_CLICKED, GetId());
	ev.SetEventObject(this);
	ev.SetString(e.GetString());
	ProcessWindowEvent(ev);
}


// -----------------------------------------------------------------------------
//
// STBSizer Class
//
// -----------------------------------------------------------------------------
class STBSizer : public wxWrapSizer
{
public:
	STBSizer() : wxWrapSizer(wxHORIZONTAL, wxEXTEND_LAST_ON_EACH_LINE | wxREMOVE_LEADING_SPACES) {}
	~STBSizer() = default;

protected:
	bool IsSpaceItem(wxSizerItem* item) const override
	{
		if (!item->GetWindow())
			return true;

		if (item->GetWindow()->GetName() == "tb_sep")
			return true;

		return false;
	}
};


// -----------------------------------------------------------------------------
//
// SToolBar Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SToolBar class constructor
// -----------------------------------------------------------------------------
SToolBar::SToolBar(wxWindow* parent, bool main_toolbar) : wxPanel(parent, -1), main_toolbar_{ main_toolbar }
{
	// Enable double buffering to avoid flickering
#ifdef __WXMSW__
	// In Windows, only enable on Vista or newer
	if (Global::win_version_major >= 6)
		wxWindow::SetDoubleBuffered(true);
#elif !defined __WXMAC__
	SetDoubleBuffered(true);
#endif

	// Set background colour
	wxPanel::SetBackgroundColour(
		(main_toolbar && Global::win_version_major >= 10) ? wxColor(250, 250, 250) : Drawing::systemPanelBGColour());

	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Bind events
	Bind(wxEVT_SIZE, &SToolBar::onSize, this);
	Bind(wxEVT_PAINT, &SToolBar::onPaint, this);
	Bind(wxEVT_KILL_FOCUS, &SToolBar::onFocus, this);
	Bind(wxEVT_RIGHT_DOWN, &SToolBar::onMouseEvent, this);
	Bind(wxEVT_LEFT_DOWN, &SToolBar::onMouseEvent, this);
	Bind(wxEVT_MENU, &SToolBar::onContextMenu, this);
	Bind(wxEVT_ERASE_BACKGROUND, &SToolBar::onEraseBackground, this);
}

// -----------------------------------------------------------------------------
// Adds [group] to the toolbar
// -----------------------------------------------------------------------------
void SToolBar::addGroup(SToolBarGroup* group)
{
	// Set the group's parent
	group->SetParent(this);

	// Set background colour
	group->SetBackgroundColour(GetBackgroundColour());

	// Add it to the list of groups
	groups_.push_back(group);

	// Update layout
	updateLayout(true);

	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &SToolBar::onButtonClick, this, group->GetId());
}

// -----------------------------------------------------------------------------
// Removes the group matching [name] from the toolbar
// -----------------------------------------------------------------------------
void SToolBar::deleteGroup(const wxString& name)
{
	// Do nothing if no name specified
	if (name.IsEmpty())
		return;

	// Find group to delete
	for (unsigned a = 0; a < groups_.size(); a++)
	{
		if (S_CMPNOCASE(groups_[a]->name(), name))
		{
			// Delete group
			delete groups_[a];
			groups_.erase(groups_.begin() + a);
			break;
		}
	}

	// Update layout
	updateLayout(true);
}

// -----------------------------------------------------------------------------
// Removes any 'custom' groups from the toolbar (name starting with '_')
// -----------------------------------------------------------------------------
void SToolBar::deleteCustomGroups()
{
	// Go through groups
	bool deleted = false;
	for (unsigned a = 0; a < groups_.size(); a++)
	{
		// Check if group is custom (custom group names don't begin with _)
		if (!groups_[a]->name().StartsWith("_"))
		{
			// Delete group
			delete groups_[a];
			groups_.erase(groups_.begin() + a);
			deleted = true;
		}
	}

	// Update layout
	if (deleted)
		updateLayout(true);
}

// -----------------------------------------------------------------------------
// Adds a new group [name] to the toolbar, containing toolbar buttons for each
// action in [actions]
// -----------------------------------------------------------------------------
void SToolBar::addActionGroup(const wxString& name, wxArrayString actions)
{
	// Do nothing if no actions were given
	if (actions.empty())
		return;

	// Create new toolbar group
	auto group = new SToolBarGroup(this, name);
	groups_.push_back(group);

	// Add actions to the group
	for (const auto& action : actions)
		group->addActionButton(action);

	// Update layout
	updateLayout(true);
}

// -----------------------------------------------------------------------------
// Recalculates the toolbar layout
// -----------------------------------------------------------------------------
void SToolBar::updateLayout(bool force, bool generate_event)
{
	// Check if we need to update at all
	if (calculateNumRows(GetSize().x) == n_rows_ && !force)
	{
		Layout();
		return;
	}

	// Clear main sizer
	auto sizer = GetSizer();
	sizer->Clear();

	// Delete previous separators
	for (auto& separator : separators_)
		delete separator;
	separators_.clear();

	// Delete previous vlines
	for (auto& vline : vlines_)
		delete vline;
	vlines_.clear();

	// Create horizontal sizer
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox);

	// Go through all groups
	int current_width = 0;
	int groups_line   = 0;
	n_rows_           = 0;
	for (auto& group : groups_)
	{
		// Skip if group is hidden
		if (group->hidden())
		{
			group->Show(false);
			continue;
		}

		// Check if the group will fit
		group->Show();
		if (group->GetBestSize().x + current_width + UI::pad() > GetSize().x && groups_line > 0)
		{
			// The group won't fit, begin a new line
			auto vline = new SToolBarVLine(this);
			sizer->Add(vline, 0, wxEXPAND);
			vlines_.push_back(vline);

			hbox = new wxBoxSizer(wxHORIZONTAL);
			sizer->Add(hbox, 0);
			groups_line   = 0;
			current_width = 0;
			n_rows_++;
		}

		// Add separator if needed
		if (groups_line > 0)
		{
			auto sep = new SToolBarSeparator(this);
			sep->SetBackgroundColour(GetBackgroundColour());
			separators_.push_back(sep);
			hbox->Add(sep, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, UI::px(UI::Size::PadMinimum));
			current_width += UI::pad() * 2;
		}

		// Add the group
		hbox->Add(group, 0, wxEXPAND | wxTOP | wxBOTTOM | wxLEFT | wxRIGHT, UI::px(UI::Size::PadMinimum));
		current_width += group->GetBestSize().x + UI::pad();

		groups_line++;
	}

	// Apply layout
	Layout();
	Refresh();

	// Check if the toolbar height changed
	int h = getBarHeight();
	if (min_height_ != (n_rows_ + 1) * h)
	{
		// Update minimum height
		min_height_ = (n_rows_ + 1) * h;

		// Generate layout update event
		if (generate_event)
		{
			wxNotifyEvent e(wxEVT_STOOLBAR_LAYOUT_UPDATED, GetId());
			e.SetEventObject(this);
			GetEventHandler()->ProcessEvent(e);
		}
	}
}

// -----------------------------------------------------------------------------
// Enables or disables toolbar group [name]
// -----------------------------------------------------------------------------
void SToolBar::enableGroup(const wxString& name, bool enable)
{
	// Go through toolbar groups
	for (auto& group : groups_)
	{
		if (S_CMPNOCASE(group->name(), name))
		{
			if (group->IsEnabled() != enable)
			{
				group->Enable(enable);
				group->Update();
				group->Refresh();
			}
			else
				return;
		}
	}

	// Redraw
	Update();
	Refresh();
}

// -----------------------------------------------------------------------------
// Populates [menu] with items to toggle each toolbar group and an item to
// toggle the 'show_toolbar_names' option
// -----------------------------------------------------------------------------
void SToolBar::populateGroupsMenu(wxMenu* menu, int start_id)
{
	for (unsigned a = 0; a < groups_.size(); a++)
	{
		wxString name = groups_[a]->name();
		name.Replace("_", "");
		auto item = menu->AppendCheckItem(start_id + a, name);
		item->Check(!groups_[a]->hidden());
	}

	// Add 'show names' item
	auto item = menu->AppendCheckItem(
		start_id + groups_.size(),
		"Show group names",
		"Show names of toolbar groups (requires program restart to take effect)");
	item->Check(show_toolbar_names);
}

// -----------------------------------------------------------------------------
// Calculates the number of toolbar rows to fit within [width]
// -----------------------------------------------------------------------------
int SToolBar::calculateNumRows(int width)
{
	// Go through all groups
	int current_width = 0;
	int groups_line   = 0;
	int rows          = 0;
	for (auto& group : groups_)
	{
		// Skip if group is hidden
		if (group->hidden())
			continue;

		// Check if the group will fit
		if (group->GetBestSize().x + current_width + UI::pad() > width && groups_line > 0)
		{
			// The group won't fit, begin a new line
			groups_line   = 0;
			current_width = 0;
			rows++;
		}

		// Add separator if needed
		if (groups_line > 0)
			current_width += UI::pad();

		// Add the group
		current_width += group->GetBestSize().x;
		groups_line++;
	}

	return rows;
}

// -----------------------------------------------------------------------------
// Returns the SToolBarGroup matching [name], or nullptr if not found
// -----------------------------------------------------------------------------
SToolBarGroup* SToolBar::group(const wxString& name)
{
	for (auto group : groups_)
		if (S_CMPNOCASE(group->name(), name))
			return group;

	return nullptr;
}


// -----------------------------------------------------------------------------
//
// SToolBar Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the toolbar is resized
// -----------------------------------------------------------------------------
void SToolBar::onSize(wxSizeEvent& e)
{
	// Update layout
#ifndef __WXMSW__
	updateLayout(false, false);
#else
	updateLayout();
#endif

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the toolbar needs to be drawn
// -----------------------------------------------------------------------------
void SToolBar::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);

	// Get system colours needed
	auto col_background = GetBackgroundColour();
	auto col_light      = Drawing::lightColour(col_background, 1.5f);
	// wxColour col_dark = win10_theme ? *wxWHITE : Drawing::darkColour(col_background, 1.5f);

	// Draw background
	dc.SetBackground(wxBrush(col_background));
	dc.Clear();

	if (draw_border_)
	{
		// Draw top
		// dc.SetPen(wxPen(wxColor(220, 220, 220)));// col_light));
		// dc.DrawLine(wxPoint(0, 0), wxPoint(GetSize().x+1, 0));

		// Draw bottom
		// dc.SetPen(wxPen(col_dark));
		// dc.DrawLine(wxPoint(0, GetSize().y-1), wxPoint(GetSize().x+1, GetSize().y-1));
	}
}

// -----------------------------------------------------------------------------
// Called when the toolbar receives or loses focus
// -----------------------------------------------------------------------------
void SToolBar::onFocus(wxFocusEvent& e)
{
	// Redraw
	Update();
	Refresh();

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a mouse event is raised for the control
// -----------------------------------------------------------------------------
void SToolBar::onMouseEvent(wxMouseEvent& e)
{
	// Right click
	if (e.GetEventType() == wxEVT_RIGHT_DOWN && enable_context_menu_)
	{
		// Build context menu
		wxMenu context;
		populateGroupsMenu(&context);

		// Popup context menu
		PopupMenu(&context);
	}

	// Left click
	if (e.GetEventType() == wxEVT_LEFT_DOWN)
	{
		Refresh();
		Update();
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a toolbar context menu item is clicked
// -----------------------------------------------------------------------------
void SToolBar::onContextMenu(wxCommandEvent& e)
{
	// Check index
	if (e.GetId() < 0)
		return;

	// 'Show group names' item
	else if (e.GetId() == (int)groups_.size())
		show_toolbar_names = !show_toolbar_names;

	// Group toggle
	else if ((unsigned)e.GetId() < groups_.size())
	{
		// Toggle group hidden
		groups_[e.GetId()]->hide(!groups_[e.GetId()]->hidden());

		// Update layout
		updateLayout(true);
	}
}

// -----------------------------------------------------------------------------
// Called when a toolbar button is clicked
// -----------------------------------------------------------------------------
void SToolBar::onButtonClick(wxCommandEvent& e)
{
	// See SToolBarGroup::onButtonClicked
	wxCommandEvent ev(wxEVT_STOOLBAR_BUTTON_CLICKED, GetId());
	ev.SetEventObject(this);
	ev.SetString(e.GetString());
	ProcessWindowEvent(ev);
}

// -----------------------------------------------------------------------------
// Called when the background of the toolbar needs erasing
// -----------------------------------------------------------------------------
void SToolBar::onEraseBackground(wxEraseEvent& e) {}


// -----------------------------------------------------------------------------
//
// SToolBar Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the height for all toolbars
// -----------------------------------------------------------------------------
int SToolBar::getBarHeight()
{
	return UI::scalePx(toolbar_size + 14);
}
