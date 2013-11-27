
#include "Main.h"
#include "WxStuff.h"
#include "MainApp.h"
#include "SToolBarButton.h"
#include "Icons.h"
#include "Drawing.h"
#include "MainWindow.h"
#include <wx/graphics.h>

CVAR(Bool, toolbar_button_flat, true, CVAR_SAVE)
EXTERN_CVAR(Int, toolbar_size);
wxDEFINE_EVENT(wxEVT_STOOLBAR_BUTTON_CLICKED, wxCommandEvent);

SToolBarButton::SToolBarButton(wxWindow* parent, string action, string icon, bool show_name)
	: wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, wxDefaultValidator, "stbutton")
{
	// Init variables
	this->action = theApp->getAction(action);
	this->state = STATE_NORMAL;
	this->show_name = show_name;
	this->action_id = this->action->getId();
	this->action_name = this->action->getText();
	this->help_text = this->action->getHelpText();

	// Determine width of name text if shown
	int name_width = 0;
	if (show_name)
	{
		string name = this->action->getText();
		name.Replace("&", "");
		name_width = GetTextExtent(name).GetWidth() + 2;
	}

	// Set size
	int size = toolbar_size + 6;
	SetSizeHints(size+name_width, size, size+name_width, size);
	SetMinSize(wxSize(size+name_width, size));
	SetSize(size+name_width, size);

	// Load icon
	if (icon.IsEmpty())
		this->icon = getIcon(this->action->getIconName(), toolbar_size > 16);
	else
		this->icon = getIcon(icon, toolbar_size > 16);

	// Set tooltip
	if (!show_name)
	{
		string tip = this->action->getText();
		tip.Replace("&", "");
		SetToolTip(tip);
	}

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

SToolBarButton::SToolBarButton(wxWindow* parent, string action_id, string action_name, string icon, string help_text, bool show_name)
	: wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, wxDefaultValidator, "stbutton")
{
	// Init variables
	this->action = NULL;
	this->action_id = action_id;
	this->action_name = action_name;
	this->help_text = help_text;
	this->state = STATE_NORMAL;
	this->show_name = show_name;

	// Determine width of name text if shown
	int name_width = show_name ? GetTextExtent(action_name).GetWidth() + 2 : 0;

	// Set size
	int size = toolbar_size + 6;
	SetSizeHints(size+name_width, size, size+name_width, size);
	SetMinSize(wxSize(size+name_width, size));
	SetSize(size+name_width, size);

	// Load icon
	this->icon = getIcon(icon, toolbar_size > 16);

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

SToolBarButton::~SToolBarButton()
{
}

void SToolBarButton::sendClickedEvent()
{
	// Generate event
	wxCommandEvent ev(wxEVT_STOOLBAR_BUTTON_CLICKED, GetId());
	ev.SetEventObject(this);
	ev.SetString(action_id);
	ProcessWindowEvent(ev);
}

void SToolBarButton::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);

	// Get system colours needed
	wxColour col_background = Drawing::getPanelBGColour();//Drawing::getMenuBarBGColour();
	wxColour col_hilight = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

	// Draw background
	dc.SetBackground(wxBrush(col_background));
	dc.Clear();

	// Create graphics context
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (!gc)
		return;

	// Get width of name text if shown
	int name_width = 0;
	int name_height = 0;
	string name = action_name;
	if (show_name)
	{
		name.Replace("&", "");
		wxSize name_size = GetTextExtent(name);
		name_width = name_size.GetWidth() + 2;
		name_height = name_size.y;
	}

	// Draw toggled border/background
	if (action && action->isToggled())
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
			col_trans.Set(col_trans.Red(), col_trans.Green(), col_trans.Blue(), 80);
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(Drawing::darkColour(col_toggle, 5.0f)));
			gc->DrawRectangle(1, 1, toolbar_size+4+name_width, toolbar_size+4);
		}
		else
		{
			// Draw border
			col_trans.Set(col_trans.Red(), col_trans.Green(), col_trans.Blue(), 80);
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(col_hilight));
			gc->DrawRoundedRectangle(1, 1, toolbar_size+4+name_width, toolbar_size+4, 2);
		}
	}

	// Draw border on mouseover
	if (state == STATE_MOUSEOVER || state == STATE_MOUSEDOWN)
	{
		// Determine transparency level
		int trans = 160;
		if (state == STATE_MOUSEDOWN)
			trans = 200;

		// Create semitransparent hilight colour
		wxColour col_trans(col_hilight.Red(), col_hilight.Green(), col_hilight.Blue(), trans);

		if (toolbar_button_flat)
		{
			// Draw border
			col_trans.Set(col_trans.Red(), col_trans.Green(), col_trans.Blue(), 80);
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(col_hilight));
			gc->DrawRectangle(1, 1, toolbar_size+4+name_width, toolbar_size+4);
		}
		else
		{
			// Draw border
			col_trans.Set(col_trans.Red(), col_trans.Green(), col_trans.Blue(), 80);
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(col_hilight));
			gc->DrawRoundedRectangle(1, 1, toolbar_size+4+name_width, toolbar_size+4, 2);
		}
	}

	// Draw disabled icon if disabled
	if (!IsEnabled())
	{
		// Determine toolbar background brightness
		uint8_t r,g,b;
		r = col_background.Red();
		g = col_background.Green();
		b = col_background.Blue();
		wxColor::MakeGrey(&r, &g, &b);

		// Draw disabled icon
		gc->DrawBitmap(icon.ConvertToDisabled(r), 3, 3, toolbar_size, toolbar_size);
	}

	// Otherwise draw normal icon
	else
		gc->DrawBitmap(icon, 3, 3, toolbar_size, toolbar_size);

	if (show_name)
	{
		int top = ((double)GetSize().y * 0.5) - ((double)name_height * 0.5);
		dc.DrawText(name, toolbar_size + 5, top);
	}

	delete gc;
}

void SToolBarButton::onMouseEvent(wxMouseEvent& e)
{
	wxFrame* parent_window = (wxFrame*)wxGetTopLevelParent(this);

	// Mouse enter
	if (e.GetEventType() == wxEVT_ENTER_WINDOW)
	{
		// Set state to mouseover
		state = STATE_MOUSEOVER;

		// Set status bar help text
		if (parent_window)
			parent_window->SetStatusText(help_text);
	}

	// Mouse leave
	if (e.GetEventType() == wxEVT_LEAVE_WINDOW)
	{
		// Set state to normal
		state = STATE_NORMAL;

		// Clear status bar help text
		if (parent_window)
			parent_window->SetStatusText("");
	}

	// Left button down
	if (e.GetEventType() == wxEVT_LEFT_DOWN || e.GetEventType() == wxEVT_LEFT_DCLICK)
	{
		state = STATE_MOUSEDOWN;

		if (action)
		{
			if (action->isRadio())
				GetParent()->Refresh();
			theApp->doAction(action->getId());
		}
		else
			sendClickedEvent();
	}

	// Left button up
	if (e.GetEventType() == wxEVT_LEFT_UP)
	{
		state = STATE_MOUSEOVER;

		// Clear status bar help text
		if (parent_window)
			parent_window->SetStatusText("");
	}

	Refresh();

	//e.Skip();
}

void SToolBarButton::onFocus(wxFocusEvent& e)
{
	// Redraw
	state = STATE_NORMAL;
	Update();
	Refresh();

	e.Skip();
}

void SToolBarButton::onEraseBackground(wxEraseEvent& e)
{
}
