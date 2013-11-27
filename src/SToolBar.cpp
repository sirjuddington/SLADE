
#include "Main.h"
#include "WxStuff.h"
#include "SToolBar.h"
#include "SToolBarButton.h"
#include "Drawing.h"
#include <wx/wrapsizer.h>

CVAR(Bool, show_toolbar_names, false, CVAR_SAVE)
CVAR(String, toolbars_hidden, "", CVAR_SAVE)
CVAR(Int, toolbar_size, 16, CVAR_SAVE)
DEFINE_EVENT_TYPE(wxEVT_STOOLBAR_LAYOUT_UPDATED)

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
		wxColour col_background = Drawing::getPanelBGColour();
		rgba_t bg(col_background.Red(), col_background.Green(), col_background.Blue());
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

		// Bind events
		Bind(wxEVT_PAINT, &SToolBarVLine::onPaint, this);
	}

	void onPaint(wxPaintEvent& e)
	{
		wxPaintDC dc(this);

		// Get system colours needed
		wxColour col_background = Drawing::getPanelBGColour();
		wxColour col_light = Drawing::lightColour(col_background, 1.5f);
		wxColour col_dark = Drawing::darkColour(col_background, 1.5f);

		// Draw lines
		dc.SetPen(wxPen(col_dark));
		dc.DrawLine(wxPoint(0, 0), wxPoint(GetSize().x+1, 0));
		dc.SetPen(wxPen(col_light));
		dc.DrawLine(wxPoint(0, 1), wxPoint(GetSize().x+1, 1));
	}
};


SToolBarGroup::SToolBarGroup(wxWindow* parent, string name, bool force_name) : wxPanel(parent, -1)
{
	// Init variables
	this->name = name;

	// Check if hidden
	string tb_hidden = toolbars_hidden;
	if (tb_hidden.Contains(S_FMT("[%s]", CHR(name))))
		hidden = true;
	else
		hidden = false;

	// Set colours
	SetBackgroundColour(Drawing::getPanelBGColour());

	// Create sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Create group label if necessary
	if (show_toolbar_names || force_name)
	{
		string showname = name;
		if (name.StartsWith("_"))
			showname.Remove(0, 1);

		wxStaticText* label = new wxStaticText(this, -1, S_FMT("%s:", CHR(showname)));
		label->SetForegroundColour(Drawing::getMenuTextColour());
		sizer->AddSpacer(4);
		sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL);
		sizer->AddSpacer(2);
	}
}

SToolBarGroup::~SToolBarGroup()
{
}

void SToolBarGroup::hide(bool hide)
{
	// Update variables
	hidden = hide;

	// Update 'hidden toolbars' cvar
	string tb_hidden = toolbars_hidden;
	if (hide)
		tb_hidden += "[" + name + "]";
	else
		tb_hidden.Replace(S_FMT("[%s]", CHR(name)), "");

	toolbars_hidden = tb_hidden;
}

SToolBarButton* SToolBarGroup::addActionButton(string action, string icon, bool show_name)
{
	// Get sizer
	wxSizer* sizer = GetSizer();

	// Create button
	SToolBarButton* button = new SToolBarButton(this, action, icon, show_name);

	// Add it to the group
	sizer->Add(button, 0, wxALIGN_CENTER_VERTICAL|wxALL, 1);

	return button;
}

SToolBarButton* SToolBarGroup::addActionButton(string action_id, string action_name, string icon, string help_text, bool show_name)
{
	// Get sizer
	wxSizer* sizer = GetSizer();

	// Create button
	SToolBarButton* button = new SToolBarButton(this, action_id, action_name, icon, help_text, show_name);
	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &SToolBarGroup::onButtonClicked, this, button->GetId());

	// Add it to the group
	sizer->Add(button, 0, wxALIGN_CENTER_VERTICAL|wxALL, 1);

	return button;
}


void SToolBarGroup::addCustomControl(wxWindow* control)
{
	// Set the control's parent to this panel
	control->SetParent(this);

	// Add it to the group
	GetSizer()->Add(control, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 1);
}

void SToolBarGroup::redraw()
{
	wxWindowList children = GetChildren();
	for (unsigned a = 0; a < children.size(); a++)
	{
		children[a]->Update();
		children[a]->Refresh();
	}
}

void SToolBarGroup::onButtonClicked(wxCommandEvent& e)
{
	// No idea why the event doesn't propagate as it's supposed to,
	// shouldn't need to do this
	wxCommandEvent ev(wxEVT_STOOLBAR_BUTTON_CLICKED, GetId());
	ev.SetEventObject(this);
	ev.SetString(e.GetString());
	ProcessWindowEvent(ev);
}



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


SToolBar::SToolBar(wxWindow* parent) : wxPanel(parent, -1)
{
	// Init variables
	min_height = 0;
	n_rows = 0;
	draw_border = true;

	// Enable double buffering to avoid flickering
#ifdef __WXMSW__
	// In Windows, only enable on Vista or newer
	int win_vers;
	wxGetOsVersion(&win_vers);
	if (win_vers >= 6)
		SetDoubleBuffered(true);
#else
	SetDoubleBuffered(true);
#endif

	// Set background colour
	SetBackgroundColour(Drawing::getPanelBGColour());

	// Create sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Bind events
	Bind(wxEVT_SIZE, &SToolBar::onSize, this);
	Bind(wxEVT_PAINT, &SToolBar::onPaint, this);
	Bind(wxEVT_KILL_FOCUS, &SToolBar::onFocus, this);
	Bind(wxEVT_RIGHT_DOWN, &SToolBar::onMouseEvent, this);
	Bind(wxEVT_LEFT_DOWN, &SToolBar::onMouseEvent, this);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &SToolBar::onContextMenu, this);
	Bind(wxEVT_ERASE_BACKGROUND, &SToolBar::onEraseBackground, this);
}

SToolBar::~SToolBar()
{
}

void SToolBar::addGroup(SToolBarGroup* group)
{
	// Set the group's parent
	group->SetParent(this);

	// Add it to the list of groups
	groups.push_back(group);

	// Update layout
	updateLayout(true);

	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &SToolBar::onButtonClick, this, group->GetId());
}

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
			separators.push_back(sep);
			hbox->Add(sep, 0, wxALIGN_CENTER_VERTICAL);
			current_width += 4;
		}

		// Add the group
		hbox->Add(groups[a], 0, wxEXPAND|wxTOP|wxBOTTOM, 2);
		current_width += groups[a]->GetBestSize().x;

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

void SToolBar::enableGroup(string name, bool enable)
{
	// Go through toolbar groups
	for (unsigned a = 0; a < groups.size(); a++)
	{
		if (S_CMPNOCASE(groups[a]->getName(), name))
			groups[a]->Enable(enable);
	}

	// Redraw
	Update();
	Refresh();
}

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

void SToolBar::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);

	// Get system colours needed
	wxColour col_background = Drawing::getPanelBGColour();
	wxColour col_light = Drawing::lightColour(col_background, 1.5f);
	wxColour col_dark = Drawing::darkColour(col_background, 1.5f);
	
	// Draw background
	dc.SetBackground(wxBrush(col_background));
	dc.Clear();

	if (draw_border)
	{
		// Draw top
		dc.SetPen(wxPen(col_light));
		dc.DrawLine(wxPoint(0, 0), wxPoint(GetSize().x+1, 0));

		// Draw bottom
		dc.SetPen(wxPen(col_dark));
		dc.DrawLine(wxPoint(0, GetSize().y-1), wxPoint(GetSize().x+1, GetSize().y-1));
	}
}

void SToolBar::onFocus(wxFocusEvent& e)
{
	// Redraw
	Update();
	Refresh();

	e.Skip();
}

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

int SToolBar::getBarHeight()
{
	return toolbar_size + 14;
}

void SToolBar::onButtonClick(wxCommandEvent& e)
{
	// See SToolBarGroup::onButtonClicked
	wxCommandEvent ev(wxEVT_STOOLBAR_BUTTON_CLICKED, GetId());
	ev.SetEventObject(this);
	ev.SetString(e.GetString());
	ProcessWindowEvent(ev);
}

void SToolBar::onEraseBackground(wxEraseEvent& e)
{
}
