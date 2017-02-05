
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SCallTip.cpp
 * Description: Custom calltip implementation for the text editor
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
#include "SCallTip.h"
#include "TextLanguage.h"
#include "Utility/Tokenizer.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, txed_calltips_dim_optional, true, CVAR_SAVE)


/*******************************************************************
 * SCALLTIP CLASS FUNCTIONS
 *******************************************************************/

/* SCallTip::SCallTip
 * SCallTip class constructor
 *******************************************************************/
SCallTip::SCallTip(wxWindow* parent)
	: wxPopupWindow(parent),
	col_bg(240, 240, 240),
	col_fg(240, 240, 240),
	function(NULL),
	arg_current(-1),
	switch_args(false),
	btn_mouse_over(0),
	buffer(1000, 1000, 32)
{
	font = GetFont();

	Show(false);

#ifndef __WXOSX__
	SetDoubleBuffered(true);
#endif // !__WXOSX__
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	// Bind events
	Bind(wxEVT_PAINT, &SCallTip::onPaint, this);
	Bind(wxEVT_ERASE_BACKGROUND, &SCallTip::onEraseBackground, this);
	Bind(wxEVT_MOTION, &SCallTip::onMouseMove, this);
	Bind(wxEVT_LEFT_DOWN, &SCallTip::onMouseDown, this);
	Bind(wxEVT_SHOW, &SCallTip::onShow, this);
}

/* SCallTip::~SCallTip
 * SCallTip class destructor
 *******************************************************************/
SCallTip::~SCallTip()
{
}

/* SCallTip::setFont
 * Sets the font [face] and [size]
 *******************************************************************/
void SCallTip::setFont(string face, int size)
{
	if (face == "")
	{
		font.SetFaceName(GetFont().GetFaceName());
		font.SetPointSize(GetFont().GetPointSize());
	}
	else
	{
		font.SetFaceName(face);
		font.SetPointSize(size);
	}
}

/* SCallTip::addArg
 * Adds a function argument, defined in [tokens]
 *******************************************************************/
void SCallTip::addArg(vector<string>& tokens)
{
	arg_t arg;

	// Optional
	if (tokens[0] == "[")
	{
		arg.optional = true;
		tokens.erase(tokens.begin());
		tokens.pop_back();
	}

	if (tokens.empty())
		return;

	// (Type) and name
	if (tokens.size() > 1)
	{
		arg.type = tokens[0];
		arg.name = tokens[1];
	}
	else
	{
		arg.name = tokens[0];
	}

	args.push_back(arg);
	tokens.clear();
}

/* SCallTip::loadArgSet
 * Opens and displays the arg set [set] from the current function
 *******************************************************************/
void SCallTip::loadArgSet(int set)
{
	args.clear();
	if (!function->getArgSet(set).empty())
	{
		Tokenizer tz;
		tz.setSpecialCharacters("[],");
		tz.openString(function->getArgSet(set));
		string token = tz.getToken();
		vector<string> tokens;
		while (true)
		{
			tokens.push_back(token);

			string next = tz.peekToken();
			if (next == "," || next == "")
			{
				addArg(tokens);
				tokens.clear();
				tz.getToken();
			}

			if (next == "")
				break;

			token = tz.getToken();
		}
	}

	updateSize();

	Update();
	Refresh();
}

/* SCallTip::openFunction
 * Opens [function] in the call tip, with [arg] highlighted
 *******************************************************************/
void SCallTip::openFunction(TLFunction* function, int arg)
{
	// Set current function
	this->function = function;
	if (!function)
		return;

	// Init with first arg set
	arg_set_current = 0;
	arg_current = arg;
	loadArgSet(arg_set_current);
}

/* SCallTip::nextArgSet
 * Open the next (cyclic) arg set in the current function
 *******************************************************************/
void SCallTip::nextArgSet()
{
	arg_set_current++;
	if (arg_set_current >= (int)function->nArgSets())
		arg_set_current = 0;
	loadArgSet(arg_set_current);
}

/* SCallTip::prevArgSet
 * Open the previous (cyclic) arg set in the current function
 *******************************************************************/
void SCallTip::prevArgSet()
{
	arg_set_current--;
	if (arg_set_current < 0)
		arg_set_current = function->nArgSets() - 1;
	loadArgSet(arg_set_current);
}

/* SCallTip::updateSize
 * Recalculates the calltip text and size
 *******************************************************************/
void SCallTip::updateSize()
{
	updateBuffer();
	SetSize(buffer.GetWidth() + 24, buffer.GetHeight() + 16);

	// Get screen bounds and window bounds
	int index = wxDisplay::GetFromWindow(this->GetParent());
	wxDisplay display(index);
	wxRect screen_area = display.GetClientArea();
	wxRect ct_area = GetScreenRect();

	// Check if calltip extends off the right of the screen
	if (ct_area.GetRight() > screen_area.GetRight())
	{
		// Move back so we're within the screen
		int offset = ct_area.GetRight() - screen_area.GetRight();
		SetPosition(wxPoint(GetPosition().x - offset, GetPosition().y));
	}
	
	Update();
	Refresh();
}

/* SCallTip::drawText
 * Using [dc], draw [text] at [left,top], writing the bounds of the
 * drawn text to [bounds]
 *******************************************************************/
int SCallTip::drawText(wxDC& dc, string text, int left, int top, wxRect* bounds)
{
	dc.DrawLabel(text, wxNullBitmap, wxRect(left, top, 900, 900), 0, -1, bounds);
	return bounds->GetRight() + 1;
}

/* SCallTip::drawCallTip
 * Using [dc], draw the calltip contents at [xoff,yoff]. Returns the
 * dimensions of the drawn calltip text
 *******************************************************************/
wxSize SCallTip::drawCallTip(wxDC& dc, int xoff, int yoff)
{
	wxSize ct_size;
	wxFont bold = font.Bold();

	// Setup faded text colour
	rgba_t faded;
	if (txed_calltips_dim_optional)
		faded = rgba_t(
			col_fg.r * 0.5 + col_bg.r * 0.5,
			col_fg.g * 0.5 + col_bg.g * 0.5,
			col_fg.b * 0.5 + col_bg.b * 0.5
		);
	else
		faded = col_fg;

	// Clear
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(WXCOL(col_bg)));
	dc.DrawRectangle(0, 0, 1000, 1000);

	if (function)
	{
		dc.SetFont(font);
		dc.SetTextForeground(WXCOL(col_fg));

		// Draw arg set switching stuff
		int left = xoff;
		if (switch_args)
		{
			// up-filled	\xE2\x96\xB2
			// up-empty		\xE2\x96\xB3
			// down-filled	\xE2\x96\xBC
			// down-empty	\xE2\x96\xBD

			// Up arrow
			dc.SetTextForeground((btn_mouse_over == 2) ? WXCOL(col_fg_hl) : WXCOL(col_fg));
			dc.DrawLabel(
				wxString::FromUTF8("\xE2\x96\xB2"),
				wxNullBitmap,
				wxRect(xoff, yoff, 100, 100),
				0,
				-1,
				&rect_btn_up);

			// Arg set
			int width = dc.GetTextExtent("X/X").x;
			dc.SetTextForeground(WXCOL(col_fg));
			dc.DrawLabel(
				S_FMT("%d/%d", arg_set_current + 1, function->nArgSets()),
				wxNullBitmap,
				wxRect(rect_btn_up.GetRight() + 4, yoff, width, 900),
				wxALIGN_CENTER_HORIZONTAL);

			// Down arrow
			dc.SetTextForeground((btn_mouse_over == 1) ? WXCOL(col_fg_hl) : WXCOL(col_fg));
			dc.DrawLabel(
				wxString::FromUTF8("\xE2\x96\xBC"),
				wxNullBitmap,
				wxRect(rect_btn_up.GetRight() + width + 8, yoff, 900, 900),
				0,
				-1,
				&rect_btn_down);

			left = rect_btn_down.GetRight() + 8;
			rect_btn_up.Offset(12, 8);
			rect_btn_down.Offset(12, 8);
		}

		// Draw function return type
		string ftype = function->getReturnType() + " ";
		wxRect rect;
		dc.SetTextForeground(WXCOL(col_type));
		dc.DrawLabel(ftype, wxNullBitmap, wxRect(left, yoff, 900, 900), 0, -1, &rect);

		// Draw function name
		string fname = function->getName();
		//wxRect rect;
		dc.SetTextForeground(WXCOL(col_func));
		//dc.DrawLabel(fname, wxNullBitmap, wxRect(left, 0, 900, 900), 0, -1, &rect);
		left = drawText(dc, fname, rect.GetRight() + 1, rect.GetTop(), &rect);

		// Draw opening bracket
		dc.SetTextForeground(WXCOL(col_fg));
		left = drawText(dc, "(", left, rect.GetTop(), &rect);

		// Draw args
		int top = rect.GetTop();
		int max_right = 0;
		int args_left = left;
		for (unsigned a = 0; a < args.size(); a++)
		{
			// Go down to next line if current is too long
			if (left > SCALLTIP_MAX_WIDTH)
			{
				left = args_left;
				top = rect.GetBottom() + 2;
			}

			// Set highlight colour if current arg
			if (a == arg_current)
			{
				dc.SetTextForeground(WXCOL(col_fg_hl));
				dc.SetFont(bold);
			}

			// Optional opening bracket
			if (args[a].optional && !txed_calltips_dim_optional)
				left = drawText(dc, "[", left, top, &rect);

			// Type
			if (!args[a].type.empty())
			{
				if (a != arg_current) dc.SetTextForeground(WXCOL(col_type));
				left = drawText(dc, args[a].type + " ", left, top, &rect);
			}

			// Name
			if (a != arg_current)
				dc.SetTextForeground(args[a].optional ? WXCOL(faded) : WXCOL(col_fg));	// Faded text if optional
			left = drawText(dc, args[a].name, left, top, &rect);

			// Optional closing bracket
			if (args[a].optional && !txed_calltips_dim_optional)
				left = drawText(dc, "]", left, top, &rect);

			// Comma (if needed)
			dc.SetFont(font);
			dc.SetTextForeground(WXCOL(col_fg));
			if (a < args.size() - 1)
				left = drawText(dc, ", ", left, top, &rect);

			// Update max width
			if (rect.GetRight() > max_right)
				max_right = rect.GetRight();
		}

		// Draw closing bracket
		left = drawText(dc, ")", left, top, &rect);

		// Draw overloads number
		if (function->nArgSets() > 1 && !switch_args)
			dc.DrawLabel(
				S_FMT(" (+%d)", function->nArgSets() - 1),
				wxNullBitmap,
				wxRect(left, top, 900, 900),
				0,
				-1,
				&rect);

		// Update max width
		if (rect.GetRight() > max_right)
			max_right = rect.GetRight();

		// Description
		string desc = function->getDescription();
		if (!desc.IsEmpty())
		{
			wxFont italic = font.Italic();
			dc.SetFont(italic);
			if (dc.GetTextExtent(desc).x > SCALLTIP_MAX_WIDTH)
			{
				// Description is too long, split into multiple lines
				vector<string> desc_lines;
				string line = desc;
				wxArrayInt extents;
				while (true)
				{
					dc.GetPartialTextExtents(line, extents);
					bool split = false;
					for (unsigned a = 0; a < extents.size(); a++)
					{
						if (extents[a] > SCALLTIP_MAX_WIDTH)
						{
							int eol = line.SubString(0, a).Last(' ');
							desc_lines.push_back(line.SubString(0, eol));
							line = line.SubString(eol + 1, line.Length());
							split = true;
							break;
						}
					}

					if (!split)
					{
						desc_lines.push_back(line);
						break;
					}
				}

				int bottom = rect.GetBottom() + 8;
				for (unsigned a = 0; a < desc_lines.size(); a++)
				{
					drawText(dc, desc_lines[a], 0, bottom, &rect);
					bottom = rect.GetBottom();
					if (rect.GetRight() > max_right)
						max_right = rect.GetRight();
				}
			}
			else
			{
				drawText(dc, desc, 0, rect.GetBottom() + 8, &rect);
				if (rect.GetRight() > max_right)
					max_right = rect.GetRight();
			}
		}

		// Size buffer bitmap to fit
		ct_size.SetWidth(max_right + 1);
		ct_size.SetHeight(rect.GetBottom() + 1);
	}
	else
	{
		// No function, empty buffer
		ct_size.SetWidth(16);
		ct_size.SetHeight(16);
	}

	return ct_size;
}

/* SCallTip::updateBuffer
 * Redraws the calltip text to the buffer image, setting the buffer
 * image size to the exact dimensions of the text
 *******************************************************************/
void SCallTip::updateBuffer()
{
	buffer.SetWidth(1000);
	buffer.SetHeight(1000);

	wxMemoryDC dc(buffer);
	auto size = drawCallTip(dc);
	buffer.SetWidth(size.GetWidth());
	buffer.SetHeight(size.GetHeight());
}


/*******************************************************************
 * SCALLTIP CLASS EVENTS
 *******************************************************************/

/* SCallTip::onPaint
 * Called when the control is to be (re)painted
 *******************************************************************/
void SCallTip::onPaint(wxPaintEvent& e)
{
	// Create device context
	wxAutoBufferedPaintDC dc(this);

	// Determine border colours
	wxColour bg = WXCOL(col_bg);
	wxColour border, border2;
	if (col_bg.greyscale().r < 128)
	{
		rgba_t c = col_bg.amp(50, 50, 50, 0);
		border = WXCOL(c);
		c = col_bg.amp(20, 20, 20, 0);
		border2 = WXCOL(c);
	}
	else
	{
		rgba_t c = col_bg.amp(-50, -50, -50, 0);
		border = WXCOL(c);
		c = col_bg.amp(-20, -20, -20, 0);
		border2 = WXCOL(c);
	}

	// Draw border
	dc.SetBrush(wxBrush(bg));
	dc.SetPen(wxPen(border));
	dc.DrawRectangle(0, 0, GetSize().x, GetSize().y);
	dc.SetPen(wxPen(border2));
	dc.DrawPoint(0, 0);
	dc.DrawPoint(0, GetSize().y - 1);
	dc.DrawPoint(GetSize().x - 1, GetSize().y - 1);
	dc.DrawPoint(GetSize().x - 1, 0);
	dc.DrawPoint(1, 1);
	dc.DrawPoint(1, GetSize().y - 2);
	dc.DrawPoint(GetSize().x - 2, GetSize().y - 2);
	dc.DrawPoint(GetSize().x - 2, 1);

	// Draw text
#ifdef __WXOSX__
	// Not sure if it's an osx or high-dpi issue (or both),
	// but for some reason wx does not properly scale the bitmap when drawing it,
	// so just draw the entire calltip again in this case
	drawCallTip(dc, 12, 8);
#else
	dc.DrawBitmap(buffer, 12, 8, true);
#endif
}

/* SCallTip::onEraseBackground
 * Erase background - overridden to do nothing, to avoid flickering
 *******************************************************************/
void SCallTip::onEraseBackground(wxEraseEvent& e)
{
	// Do nothing
}

/* SCallTip::onMouseMove
 * Called when the mouse pointer is moved within the control
 *******************************************************************/
void SCallTip::onMouseMove(wxMouseEvent& e)
{
	if (rect_btn_down.Contains(e.GetPosition()))
	{
		if (btn_mouse_over != 1)
		{
			btn_mouse_over = 1;
			updateBuffer();
			Refresh();
			Update();
		}
	}

	else if (rect_btn_up.Contains(e.GetPosition()))
	{
		if (btn_mouse_over != 2)
		{
			btn_mouse_over = 2;
			updateBuffer();
			Refresh();
			Update();
		}
	}

	else if (btn_mouse_over != 0)
	{
		btn_mouse_over = 0;
		updateBuffer();
		Refresh();
		Update();
	}
}

/* SCallTip::onMouseDown
 * Called when a mouse button is clicked within the control
 *******************************************************************/
void SCallTip::onMouseDown(wxMouseEvent& e)
{
	if (e.Button(wxMOUSE_BTN_LEFT))
	{
		if (btn_mouse_over == 1)
			nextArgSet();
		else if (btn_mouse_over == 2)
			prevArgSet();
	}
}

/* SCallTip::onShow
 * Called when the control is shown
 *******************************************************************/
void SCallTip::onShow(wxShowEvent& e)
{
	if (e.IsShown())
	{
		// Get screen bounds and window bounds
		int index = wxDisplay::GetFromWindow(this->GetParent());
		wxDisplay display(index);
		wxRect screen_area = display.GetClientArea();
		wxRect ct_area = GetScreenRect();

		// Check if calltip extends off the right of the screen
		if (ct_area.GetRight() > screen_area.GetRight())
		{
			// Move back so we're within the screen
			int offset = ct_area.GetRight() - screen_area.GetRight();
			SetPosition(wxPoint(GetPosition().x - offset, GetPosition().y));
		}
	}

	e.Skip();
}
