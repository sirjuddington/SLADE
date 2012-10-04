
#include "Main.h"
#include "WxStuff.h"
#include "MainApp.h"
#include "SToolBarButton.h"
#include "Icons.h"
#include "Drawing.h"
#include "MainWindow.h"
#include <wx/graphics.h>

CVAR(Bool, toolbar_button_flat, false, CVAR_SAVE)

SToolBarButton::SToolBarButton(wxWindow* parent, string action, string icon)
: wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, wxDefaultValidator, "stbutton") {
	// Init variables
	this->action = theApp->getAction(action);
	this->state = STATE_NORMAL;

	// Set size
	int size = 22;
	SetSizeHints(size, size, size, size);
	SetMinSize(wxSize(size, size));
	SetSize(size, size);

	// Load icon
	if (icon.IsEmpty())
		this->icon = getIcon(this->action->getIconName());
	else
		this->icon = getIcon(icon);

	// Set tooltip
	string tip = this->action->getText();
	tip.Replace("&", "");
	SetToolTip(tip);

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

SToolBarButton::~SToolBarButton() {
}

void SToolBarButton::onPaint(wxPaintEvent& e) {
	wxPaintDC dc(this);

	// Get system colours needed
	wxColour col_background = Drawing::getMenuBarBGColour();
	wxColour col_hilight = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

	// Draw background
	dc.SetBackground(wxBrush(col_background));
	dc.Clear();

	// Create graphics context
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (!gc)
		return;

	// Draw toggled border/background
	if (action->isToggled()) {
		// Use greyscale version of hilight colour
		uint8_t r = col_hilight.Red();
		uint8_t g = col_hilight.Green();
		uint8_t b = col_hilight.Blue();
		wxColour::MakeGrey(&r, &g, &b);
		wxColour col_toggle(r, g, b, 255);
		wxColour col_trans(r, g, b, 150);

		if (toolbar_button_flat) {
			// Draw border
			col_trans.Set(col_trans.Red(), col_trans.Green(), col_trans.Blue(), 80);
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(Drawing::darkColour(col_toggle, 5.0f)));
			gc->DrawRectangle(1, 1, 20, 20);
		}
		else {
			// Draw border
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(Drawing::lightColour(col_toggle, 5.0f), 1));
			gc->DrawRoundedRectangle(2, 2, 18, 18, 2);

			// Draw outer border
			gc->SetBrush(wxBrush(col_toggle, wxBRUSHSTYLE_TRANSPARENT));
			gc->SetPen(wxPen(Drawing::darkColour(col_toggle, 5.0f)));
			gc->DrawRoundedRectangle(1, 1, 20, 20, 2);
		}
	}

	// Draw border on mouseover
	if (state == STATE_MOUSEOVER || state == STATE_MOUSEDOWN) {
		// Determine transparency level
		int trans = 160;
		if (state == STATE_MOUSEDOWN)
			trans = 200;

		// Create semitransparent hilight colour
		wxColour col_trans(col_hilight.Red(), col_hilight.Green(), col_hilight.Blue(), trans);

		if (toolbar_button_flat) {
			// Draw border
			col_trans.Set(col_trans.Red(), col_trans.Green(), col_trans.Blue(), 80);
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(col_hilight));
			gc->DrawRectangle(1, 1, 20, 20);
		}
		else {
			// Draw border
			gc->SetBrush(col_trans);
			gc->SetPen(wxPen(Drawing::lightColour(col_hilight, 5.0f), 1));
			gc->DrawRoundedRectangle(2, 2, 18, 18, 2);

			// Draw outer border
			gc->SetBrush(wxBrush(col_hilight, wxBRUSHSTYLE_TRANSPARENT));
			gc->SetPen(wxPen(Drawing::darkColour(col_hilight, 5.0f)));
			gc->DrawRoundedRectangle(1, 1, 20, 20, 2);
		}
	}

	// Draw disabled icon if disabled
	if (!IsEnabled()) {
		// Determine toolbar background brightness
		uint8_t r,g,b;
		r = col_background.Red();
		g = col_background.Green();
		b = col_background.Blue();
		wxColor::MakeGrey(&r, &g, &b);

		// Draw disabled icon
		gc->DrawBitmap(icon.ConvertToDisabled(r), 3, 3, 16, 16);
	}

	// Otherwise draw normal icon
	else
		gc->DrawBitmap(icon, 3, 3, 16, 16);

	delete gc;
}

void SToolBarButton::onMouseEvent(wxMouseEvent& e) {
	wxFrame* parent_window = (wxFrame*)wxGetTopLevelParent(this);

	// Mouse enter
	if (e.GetEventType() == wxEVT_ENTER_WINDOW) {
		// Set state to mouseover
		state = STATE_MOUSEOVER;

		// Set status bar help text
		if (parent_window)
			parent_window->SetStatusText(action->getHelpText());
	}

	// Mouse leave
	if (e.GetEventType() == wxEVT_LEAVE_WINDOW) {
		// Set state to normal
		state = STATE_NORMAL;

		// Clear status bar help text
		if (parent_window)
			parent_window->SetStatusText("");
	}

	// Left button down
	if (e.GetEventType() == wxEVT_LEFT_DOWN || e.GetEventType() == wxEVT_LEFT_DCLICK) {
		state = STATE_MOUSEDOWN;
		if (action->isRadio()) GetParent()->Refresh();

		theApp->doAction(action->getId());
	}

	// Left button up
	if (e.GetEventType() == wxEVT_LEFT_UP) {
		state = STATE_MOUSEOVER;

		// Clear status bar help text
		if (parent_window)
			parent_window->SetStatusText("");
	}

	Refresh();

	//e.Skip();
}

void SToolBarButton::onFocus(wxFocusEvent& e) {
	// Redraw
	state = STATE_NORMAL;
	Update();
	Refresh();

	e.Skip();
}

void SToolBarButton::onEraseBackground(wxEraseEvent& e) {
}
