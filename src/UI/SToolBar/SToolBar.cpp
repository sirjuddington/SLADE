
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SToolBar.cpp
 * Description: SToolBar class - a custom toolbar implementation
 *              that allows any kind of control to be placed on it
 *              and auto-arranges itself based on groups.
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
#include "SToolBar.h"
#include "SToolBarButton.h"
#include "OpenGL/Drawing.h"



/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, show_toolbar_names, false, CVAR_SAVE)
CVAR(String, toolbars_hidden, "", CVAR_SAVE)
CVAR(Int, toolbar_size, 16, CVAR_SAVE)
DEFINE_EVENT_TYPE(wxEVT_STOOLBAR_LAYOUT_UPDATED)


/*******************************************************************
 * STOOLBARSEPARATOR CLASS
 *******************************************************************
 * Simple control to use as a separator between toolbar groups
 */
class SToolBarSeparator : public wxControl
{
public:
	SToolBarSeparator(wxWindow* parent) : wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
	{
		// Set size
		int size = toolbar_size + 6;
		SetSizeHints(4, size, 4, size);
		SetMinSize(wxSize(4, size));
		SetSize(4, size);

		// Set window name
		SetName("tb_sep");

		// Bind events
		Bind(wxEVT_PAINT, &SToolBarSeparator::onPaint, this);
	}

	void onPaint(wxPaintEvent& e)
	{
		wxPaintDC dc(this);

		// Get system colours needed
		wxColour col_background = GetBackgroundColour();//toolbar_win10 ? *wxWHITE : Drawing::getPanelBGColour();
		rgba_t bg(COLWX(col_background));
		wxColour col_light = WXCOL(bg.amp(50, 50, 50, 0));
		wxColour col_dark = WXCOL(bg.amp(-50, -50, -50, 0));

		// Draw background
		dc.SetBackground(wxBrush(col_background));
		dc.Clear();

		// Draw separator lines
		int height = (toolbar_size / 16.0) * 11;
		dc.GradientFillLinear(wxRect(1, 0, 1, height), col_background, col_dark, wxSOUTH);
		dc.GradientFillLinear(wxRect(1, height, 1, height), col_background, col_dark, wxNORTH);
		dc.GradientFillLinear(wxRect(2, 0, 1, height), col_background, col_light, wxSOUTH);
		dc.GradientFillLinear(wxRect(2, height, 1, height), col_background, col_light, wxNORTH);
	}
};

/*******************************************************************
 * STOOLBARVLINE CLASS
 *******************************************************************
 * Used as a separator between toolbar rows
 */
class SToolBarVLine : public wxControl
{
public:
	SToolBarVLine(wxWindow* parent) : wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
	{
		// Set size
		SetMaxSize(wxSize(-1, 2));
		SetMinSize(wxSize(-1, 2));

		// Set window name
		SetName("tb_vline");

		//if (toolbar_win10)
		//	SetBackgroundColour(*wxWHITE);

		// Bind events
		Bind(wxEVT_PAINT, &SToolBarVLine::onPaint, this);
	}

	void onPaint(wxPaintEvent& e)
	{
		wxPaintDC dc(this);

		// Get system colours needed
		wxColour col_background = GetBackgroundColour();//toolbar_win10 ? *wxWHITE : Drawing::getPanelBGColour();
		wxColour col_light = Drawing::lightColour(col_background, 1.5f);
		wxColour col_dark = Drawing::darkColour(col_background, 1.5f);

		// Draw lines
		//dc.SetPen(wxPen(col_dark));
		//dc.DrawLine(wxPoint(0, 0), wxPoint(GetSize().x+1, 0));
		//dc.SetPen(wxPen(col_light));
		//dc.DrawLine(wxPoint(0, 1), wxPoint(GetSize().x+1, 1));
	}
};

/*******************************************************************
 * STOOLBARGROUP CLASS FUNCTIONS
 *******************************************************************/

/* SToolBarGroup::SToolBarGroup
 * SToolBarGroup class constructor
 *******************************************************************/
SToolBarGroup::SToolBarGroup(SToolBar* parent, string name, bool force_name) : wxPanel(parent, -1)
{
	// Init variables
	this->name = name;

	// Check if hidden
	string tb_hidden = toolbars_hidden;
	if (tb_hidden.Contains(S_FMT("[%s]", name)))
		hidden = true;
	else
		hidden = false;

	// Set colour
	SetBackgroundColour(parent->GetBackgroundColour());

	// Create sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Create group label if necessary
	if (show_toolbar_names || force_name)
	{
		string showname = name;
		if (name.StartsWith("_"))
			showname.Remove(0, 1);

		wxStaticText* label = new wxStaticText(this, -1, S_FMT("%s:", showname));
		label->SetForegroundColour(Drawing::getMenuTextColour());
		sizer->AddSpacer(4);
		sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL);
		sizer->AddSpacer(2);
	}
}

/* SToolBarGroup::~SToolBarGroup
 * SToolBarGroup class destructor
 *******************************************************************/
SToolBarGroup::~SToolBarGroup()
{
}

/* SToolBarGroup::hide
 * Hides the group if [hide] is true, otherwise shows it
 *******************************************************************/
void SToolBarGroup::hide(bool hide)
{
	// Update variables
	hidden = hide;

	// Update 'hidden toolbars' cvar
	string tb_hidden = toolbars_hidden;
	if (hide)
		tb_hidden += "[" + name + "]";
	else
		tb_hidden.Replace(S_FMT("[%s]", name), "");

	toolbars_hidden = tb_hidden;
}

/* SToolBarGroup::addActionButton
 * Adds a toolbar button to the group for [action]. If [icon] is
 * empty, the action's icon is used
 *******************************************************************/
SToolBarButton* SToolBarGroup::addActionButton(string action, string icon, bool show_name)
{
	// Get sizer
	wxSizer* sizer = GetSizer();

	// Create button
	SToolBarButton* button = new SToolBarButton(this, action, icon, show_name);
	button->SetBackgroundColour(GetBackgroundColour());

	// Add it to the group
	sizer->Add(button, 0, wxALIGN_CENTER_VERTICAL|wxALL, 1);

	return button;
}

/* SToolBarGroup::addActionButton
 * Adds a toolbar button to the group for [action_id]. [action_name],
 * [icon] and [help_text] can be defined to override the defaults
 * of the action
 *******************************************************************/
SToolBarButton* SToolBarGroup::addActionButton(string action_id, string action_name, string icon, string help_text, bool show_name)
{
	// Get sizer
	wxSizer* sizer = GetSizer();

	// Create button
	SToolBarButton* button = new SToolBarButton(this, action_id, action_name, icon, help_text, show_name);
	button->SetBackgroundColour(GetBackgroundColour());
	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &SToolBarGroup::onButtonClicked, this, button->GetId());

	// Add it to the group
	sizer->Add(button, 0, wxALIGN_CENTER_VERTICAL|wxALL, 1);

	return button;
}

/* SToolBarGroup::addCustomControl
 * Adds a control to the group
 *******************************************************************/
void SToolBarGroup::addCustomControl(wxWindow* control)
{
	// Set the control's parent to this panel
	control->SetParent(this);

	// Add it to the group
	GetSizer()->Add(control, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 1);
}

/* SToolBarGroup::redraw
 * Redraws all controls in the group
 *******************************************************************/
void SToolBarGroup::redraw()
{
	wxWindowList children = GetChildren();
	for (unsigned a = 0; a < children.size(); a++)
	{
		children[a]->Update();
		children[a]->Refresh();
	}
}

/* SToolBarGroup::onButtonClicked
 * Called when a toolbar button within the group is clicked
 *******************************************************************/
void SToolBarGroup::onButtonClicked(wxCommandEvent& e)
{
	// No idea why the event doesn't propagate as it's supposed to,
	// shouldn't need to do this
	wxCommandEvent ev(wxEVT_STOOLBAR_BUTTON_CLICKED, GetId());
	ev.SetEventObject(this);
	ev.SetString(e.GetString());
	ProcessWindowEvent(ev);
}


/*******************************************************************
 * STBSIZER CLASS
 *******************************************************************/
class STBSizer : public wxWrapSizer
{
public:
	STBSizer() : wxWrapSizer(wxHORIZONTAL, wxEXTEND_LAST_ON_EACH_LINE|wxREMOVE_LEADING_SPACES) {}
	~STBSizer() {}

protected:
	bool IsSpaceItem(wxSizerItem* item) const
	{
		if (!item->GetWindow())
			return true;

		if (item->GetWindow()->GetName() == "tb_sep")
			return true;

		return false;
	}
};


/*******************************************************************
 * STOOLBAR CLASS FUNCTIONS
 *******************************************************************/

/* SToolBar::SToolBar
 * SToolBar class constructor
 *******************************************************************/
SToolBar::SToolBar(wxWindow* parent, bool main_toolbar) : wxPanel(parent, -1)
{
	// Init variables
	min_height = 0;
	n_rows = 0;
	draw_border = true;
	this->main_toolbar = main_toolbar;

	// Enable double buffering to avoid flickering
#ifdef __WXMSW__
	// In Windows, only enable on Vista or newer
	if (Global::win_version_major >= 6)
		SetDoubleBuffered(true);
#elif !defined __WXMAC__
	SetDoubleBuffered(true);
#endif

	// Set background colour
	SetBackgroundColour((main_toolbar && Global::win_version_major >= 10) ? wxColor(250, 250, 250) : Drawing::getPanelBGColour());

	// Create sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
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

/* SToolBar::~SToolBar
 * SToolBar class destructor
 *******************************************************************/
SToolBar::~SToolBar()
{
}

/* SToolBar::addGroup
 * Adds [group] to the toolbar
 *******************************************************************/
void SToolBar::addGroup(SToolBarGroup* group)
{
	// Set the group's parent
	group->SetParent(this);

	// Set background colour
	group->SetBackgroundColour(GetBackgroundColour());

	// Add it to the list of groups
	groups.push_back(group);

	// Update layout
	updateLayout(true);

	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &SToolBar::onButtonClick, this, group->GetId());
}

/* SToolBar::deleteGroup
 * Removes the group matching [name] from the toolbar
 *******************************************************************/
void SToolBar::deleteGroup(string name)
{
	// Do nothing if no name specified
	if (name.IsEmpty())
		return;

	// Find group to delete
	for (unsigned a = 0; a < groups.size(); a++)
	{
		if (S_CMPNOCASE(groups[a]->getName(), name))
		{
			// Delete group
			delete groups[a];
			groups.erase(groups.begin() + a);
			break;
		}
	}

	// Update layout
	updateLayout(true);
}

/* SToolBar::deleteCustomGroups
 * Removes any 'custom' groups from the toolbar (name starting with
 * '_')
 *******************************************************************/
void SToolBar::deleteCustomGroups()
{
	// Go through groups
	bool deleted = false;
	for (unsigned a = 0; a < groups.size(); a++)
	{
		// Check if group is custom (custom group names don't begin with _)
		if (!groups[a]->getName().StartsWith("_"))
		{
			// Delete group
			delete groups[a];
			groups.erase(groups.begin() + a);
			deleted = true;
		}
	}

	// Update layout
	if (deleted)
		updateLayout(true);
}

/* SToolBar::addActionGroup
 * Adds a new group [name] to the toolbar, containing toolbar buttons
 * for each action in [actions]
 *******************************************************************/
void SToolBar::addActionGroup(string name, wxArrayString actions)
{
	// Do nothing if no actions were given
	if (actions.size() == 0)
		return;

	// Create new toolbar group
	SToolBarGroup* group = new SToolBarGroup(this, name);
	groups.push_back(group);

	// Add actions to the group
	for (unsigned a = 0; a < actions.size(); a++)
		group->addActionButton(actions[a]);

	// Update layout
	updateLayout(true);
}

/* SToolBar::updateLayout
 * Recalculates the toolbar layout
 *******************************************************************/
void SToolBar::updateLayout(bool force, bool generate_event)
{
	// Check if we need to update at all
	if (calculateNumRows(GetSize().x) == n_rows && !force)
	{
		Layout();
		return;
	}

	// Clear main sizer
	wxSizer* sizer = GetSizer();
	sizer->Clear();

	// Delete previous separators
	for (unsigned a = 0; a < separators.size(); a++)
		delete separators[a];
	separators.clear();

	// Delete previous vlines
	for (unsigned a = 0; a < vlines.size(); a++)
		delete vlines[a];
	vlines.clear();

	// Create horizontal sizer
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox);

	// Go through all groups
	int current_width = 0;
	int groups_line = 0;
	n_rows = 0;
	for (unsigned a = 0; a < groups.size(); a++)
	{
		// Skip if group is hidden
		if (groups[a]->isHidden())
		{
			groups[a]->Show(false);
			continue;
		}

		// Check if the group will fit
		groups[a]->Show();
		if (groups[a]->GetBestSize().x + current_width + 4 > GetSize().x && groups_line > 0)
		{
			// The group won't fit, begin a new line
			SToolBarVLine* vline = new SToolBarVLine(this);
			sizer->Add(vline, 0, wxEXPAND);
			vlines.push_back(vline);

			hbox = new wxBoxSizer(wxHORIZONTAL);
			sizer->Add(hbox, 0);
			groups_line = 0;
			current_width = 0;
			n_rows++;
		}

		// Add separator if needed
		if (groups_line > 0)
		{
			SToolBarSeparator* sep = new SToolBarSeparator(this);
			sep->SetBackgroundColour(GetBackgroundColour());
			separators.push_back(sep);
			hbox->Add(sep, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 2);
			current_width += 8;
		}

		// Add the group
		hbox->Add(groups[a], 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT|wxRIGHT, 2);
		current_width += groups[a]->GetBestSize().x + 4;

		groups_line++;
	}

	// Apply layout
	Layout();
	Refresh();

	// Check if the toolbar height changed
	int h = toolbar_size + 14;
	if (min_height != (n_rows+1) * h)
	{
		// Update minimum height
		min_height = (n_rows+1) * h;

		// Generate layout update event
		if (generate_event)
		{
			wxNotifyEvent e(wxEVT_STOOLBAR_LAYOUT_UPDATED, GetId());
			e.SetEventObject(this);
			GetEventHandler()->ProcessEvent(e);
		}
	}
}

/* SToolBar::enableGroup
 * Enables or disables toolbar group [name]
 *******************************************************************/
void SToolBar::enableGroup(string name, bool enable)
{
	// Go through toolbar groups
	for (unsigned a = 0; a < groups.size(); a++)
	{
		if (S_CMPNOCASE(groups[a]->getName(), name))
		{
			if (groups[a]->IsEnabled() != enable)
			{
				groups[a]->Enable(enable);
				groups[a]->Update();
				groups[a]->Refresh();
			}
			else
				return;
		}
	}

	// Redraw
	Update();
	Refresh();
}

/* SToolBar::calculateNumRows
 * Calculates the number of toolbar rows to fit within [width]
 *******************************************************************/
int SToolBar::calculateNumRows(int width)
{
	// Go through all groups
	int current_width = 0;
	int groups_line = 0;
	int rows = 0;
	for (unsigned a = 0; a < groups.size(); a++)
	{
		// Skip if group is hidden
		if (groups[a]->isHidden())
			continue;

		// Check if the group will fit
		if (groups[a]->GetBestSize().x + current_width + 4 > width && groups_line > 0)
		{
			// The group won't fit, begin a new line
			groups_line = 0;
			current_width = 0;
			rows++;
		}

		// Add separator if needed
		if (groups_line > 0)
			current_width += 4;

		// Add the group
		current_width += groups[a]->GetBestSize().x;
		groups_line++;
	}

	return rows;
}


/*******************************************************************
 * STOOLBAR CLASS EVENTS
 *******************************************************************/

/* SToolBar::onSize
 * Called when the toolbar is resized
 *******************************************************************/
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

/* SToolBar::onPaint
 * Called when the toolbar needs to be drawn
 *******************************************************************/
void SToolBar::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);

	// Get system colours needed
	wxColour col_background = GetBackgroundColour();
	wxColour col_light = Drawing::lightColour(col_background, 1.5f);
	//wxColour col_dark = win10_theme ? *wxWHITE : Drawing::darkColour(col_background, 1.5f);
	
	// Draw background
	dc.SetBackground(wxBrush(col_background));
	dc.Clear();

	if (draw_border)
	{
		// Draw top
		//dc.SetPen(wxPen(wxColor(220, 220, 220)));// col_light));
		//dc.DrawLine(wxPoint(0, 0), wxPoint(GetSize().x+1, 0));

		// Draw bottom
		//dc.SetPen(wxPen(col_dark));
		//dc.DrawLine(wxPoint(0, GetSize().y-1), wxPoint(GetSize().x+1, GetSize().y-1));
	}
}

/* SToolBar::onFocus
 * Called when the toolbar receives or loses focus
 *******************************************************************/
void SToolBar::onFocus(wxFocusEvent& e)
{
	// Redraw
	Update();
	Refresh();

	e.Skip();
}

/* SToolBar::onMouseEvent
 * Called when a mouse event is raised for the control
 *******************************************************************/
void SToolBar::onMouseEvent(wxMouseEvent& e)
{
	// Right click
	if (e.GetEventType() == wxEVT_RIGHT_DOWN)
	{
		// Build context menu
		wxMenu context;
		for (unsigned a = 0; a < groups.size(); a++)
		{
			string name = groups[a]->getName();
			name.Replace("_", "");
			wxMenuItem* item = context.AppendCheckItem(a, name);
			item->Check(!groups[a]->isHidden());
		}

		// Add 'show names' item
		wxMenuItem* item = context.AppendCheckItem(groups.size(), "Show group names", "Show names of toolbar groups (requires program restart to take effect)");
		item->Check(show_toolbar_names);

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

/* SToolBar::onContextMenu
 * Called when a toolbar context menu item is clicked
 *******************************************************************/
void SToolBar::onContextMenu(wxCommandEvent& e)
{
	// Check index
	if (e.GetId() < 0)
		return;

	// 'Show group names' item
	else if (e.GetId() == groups.size())
		show_toolbar_names = !show_toolbar_names;

	// Group toggle
	else if ((unsigned)e.GetId() < groups.size())
	{
		// Toggle group hidden
		groups[e.GetId()]->hide(!groups[e.GetId()]->isHidden());

		// Update layout
		updateLayout(true);
	}
}

/* SToolBar::onButtonClick
 * Called when a toolbar button is clicked
 *******************************************************************/
void SToolBar::onButtonClick(wxCommandEvent& e)
{
	// See SToolBarGroup::onButtonClicked
	wxCommandEvent ev(wxEVT_STOOLBAR_BUTTON_CLICKED, GetId());
	ev.SetEventObject(this);
	ev.SetString(e.GetString());
	ProcessWindowEvent(ev);
}

/* SToolBar::onEraseBackground
 * Called when the background of the toolbar needs erasing
 *******************************************************************/
void SToolBar::onEraseBackground(wxEraseEvent& e)
{
}


/*******************************************************************
 * STOOLBAR CLASS STATIC FUNCTIONS
 *******************************************************************/

/* SToolBar::getBarHeight
* Returns the height for all toolbars
*******************************************************************/
int SToolBar::getBarHeight()
{
	return toolbar_size + 14;
}
