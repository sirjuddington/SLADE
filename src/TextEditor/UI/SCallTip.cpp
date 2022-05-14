
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SCallTip.cpp
// Description: Custom calltip implementation for the text editor
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
#include "SCallTip.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
wxColour wxcol_fg;
wxColour wxcol_fg_hl;
wxColour wxcol_type;
} // namespace
CVAR(Bool, txed_calltips_dim_optional, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// SCallTip Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SCallTip class constructor
// -----------------------------------------------------------------------------
SCallTip::SCallTip(wxWindow* parent) : wxPopupWindow(parent), buffer_{ 1000, 1000, 32 }, font_{ GetFont() }
{
	wxPopupWindow::Show(false);

#ifndef __WXOSX__
	wxWindow::SetDoubleBuffered(true);
#endif // !__WXOSX__
	wxWindowBase::SetBackgroundStyle(wxBG_STYLE_PAINT);

	// Bind events
	Bind(wxEVT_PAINT, &SCallTip::onPaint, this);
	Bind(wxEVT_ERASE_BACKGROUND, &SCallTip::onEraseBackground, this);
	Bind(wxEVT_MOTION, &SCallTip::onMouseMove, this);
	Bind(wxEVT_LEFT_DOWN, &SCallTip::onMouseDown, this);
	Bind(wxEVT_SHOW, &SCallTip::onShow, this);
}

// -----------------------------------------------------------------------------
// Sets the font [face] and [size]
// -----------------------------------------------------------------------------
void SCallTip::setFont(const wxString& face, int size)
{
	if (face.empty())
	{
		font_.SetFaceName(GetFont().GetFaceName());
#if wxCHECK_VERSION(3, 1, 6)
		font_.SetPointSize(FromDIP(GetFont().GetPointSize()));
#else
		font_.SetPointSize(GetFont().GetPointSize());
#endif
	}
	else
	{
		font_.SetFaceName(face);
		font_.SetPointSize(size);
	}
}

// -----------------------------------------------------------------------------
// Opens and displays the context [index] from the current function
// -----------------------------------------------------------------------------
void SCallTip::loadContext(unsigned long index)
{
	context_ = function_->context(index);

	updateSize();

	Update();
	Refresh();
}

// -----------------------------------------------------------------------------
// Opens [function] in the call tip, with [arg] highlighted
// -----------------------------------------------------------------------------
void SCallTip::openFunction(TLFunction* function, int arg)
{
	// Set current function
	function_ = function;
	if (!function)
		return;

	// Init with first arg set
	context_current_ = 0;
	arg_current_     = arg;
	loadContext(context_current_);
}

// -----------------------------------------------------------------------------
// Open the next (cyclic) arg set in the current function
// -----------------------------------------------------------------------------
void SCallTip::nextArgSet()
{
	if (++context_current_ >= function_->contexts().size())
		context_current_ = 0;
	loadContext(context_current_);
}

// -----------------------------------------------------------------------------
// Open the previous (cyclic) arg set in the current function
// -----------------------------------------------------------------------------
void SCallTip::prevArgSet()
{
	if (context_current_ == 0)
		context_current_ = function_->contexts().size() - 1;
	else
		--context_current_;
	loadContext(context_current_);
}

// -----------------------------------------------------------------------------
// Recalculates the calltip text and size
// -----------------------------------------------------------------------------
void SCallTip::updateSize()
{
	updateBuffer();
	SetSize(buffer_.GetWidth() + ui::scalePx(24), buffer_.GetHeight() + ui::scalePx(16));

	// Get screen bounds and window bounds
	auto      index = (unsigned)wxDisplay::GetFromWindow(this->GetParent());
	wxDisplay display(index);
	auto      screen_area = display.GetClientArea();
	auto      ct_area     = GetScreenRect();

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

// -----------------------------------------------------------------------------
// Using [dc], draw [text] at [left,top], writing the bounds of the drawn text
// to [bounds]
// -----------------------------------------------------------------------------
int SCallTip::drawText(wxDC& dc, const wxString& text, int left, int top, wxRect* bounds) const
{
	dc.DrawLabel(text, wxNullBitmap, wxRect(left, top, 900, 900), 0, -1, bounds);
	return bounds->GetRight() + 1;
}

// -----------------------------------------------------------------------------
// Draws function 'spec' text for [context] at [left,top].
// Returns a rect of the bounds of the drawn text
// -----------------------------------------------------------------------------
wxRect SCallTip::drawFunctionSpec(wxDC& dc, const TLFunction::Context& context, int left, int top) const
{
	wxRect rect{ left, top, 0, 0 };
	int    rect_left = left;

	// Draw deprecated message
	if (!context.deprecated_v.empty() || !context.deprecated_f.empty())
	{
		wxString deprecated_msg = "[ Deprecated";
		if (!context.deprecated_v.empty())
			deprecated_msg = wxString::Format("%s v%s", deprecated_msg, context.deprecated_v);
		if (!context.deprecated_f.empty())
			deprecated_msg = wxString::Format("%s, use \'%s()\'", deprecated_msg, context.deprecated_f);
		deprecated_msg = wxString::Format("%s ] ", deprecated_msg);

		dc.SetTextForeground(*wxRED);
		drawText(dc, deprecated_msg, left, top, &rect);

		top    = rect.GetBottom();
		rect.y = top;
	}

	// Draw function qualifiers
	if (!context.qualifiers.empty())
	{
		dc.SetTextForeground(col_keyword_.toWx());
		left = drawText(dc, context.qualifiers, left, top, &rect);
	}

	// Draw function return type
	if (!context.return_type.empty())
	{
		wxString ftype = wxString::Format("%s ", context.return_type);
		dc.SetTextForeground(wxcol_type);
		left = drawText(dc, ftype, left, top, &rect);
	}

	// Draw function context (if any)
	if (!context.context.empty())
	{
		dc.SetTextForeground(wxcol_fg);
		left = drawText(dc, wxString::Format("%s.", context.context), left, top, &rect);
	}

	// Draw function name
	wxString fname = function_->name();
	dc.SetTextForeground(col_func_.toWx());
	left = drawText(dc, fname, left, top, &rect);

	// Draw opening bracket
	dc.SetTextForeground(wxcol_fg);
	left = drawText(dc, " (", left, top, &rect);

	return { { rect_left, top }, rect.GetBottomRight() };
}

// -----------------------------------------------------------------------------
// Draws function args text for [context] at [left,top].
// Returns a rect of the bounds of the drawn text
// -----------------------------------------------------------------------------
wxRect SCallTip::drawArgs(
	wxDC&                      dc,
	const TLFunction::Context& context,
	int                        left,
	int                        top,
	wxColour&                  col_faded,
	wxFont&                    bold) const
{
	wxRect rect{ left, top, 0, 0 };

	int max_right = left;
	int args_left = left;
	int args_top  = top;

	size_t params_lenght = 0;

	for (const auto& param : context.params)
	{
		// Count param name + space length
		if (!param.type.empty())
			params_lenght += param.name.size() + 1;

		// Count param type + space length
		if (!param.type.empty())
			params_lenght += param.type.size() + 1;

		// Count param default_value + " = " length
		if (!param.default_value.empty())
			params_lenght += param.default_value.size() + 3;

		// Count brackets for optional params
		if (param.optional)
			params_lenght += 2;
	}

	if (context.params.empty())
		params_lenght = 4; // void

	params_lenght *= ui::scalePx(font_.GetPixelSize().GetWidth());

	bool long_params = (args_left + params_lenght) > MAX_WIDTH;

	for (int a = 0; a < (int)context.params.size(); a++)
	{
		auto& arg = context.params[a];

		// Go down to next line if current is too long
		if ((a != 0 && (long_params && !(a % 2))) || left > MAX_WIDTH)
		{
			left = args_left;
			top  = rect.GetBottom() + ui::scalePx(2);
		}

		// Set highlight colour if current arg
		if (a == arg_current_)
		{
			dc.SetTextForeground(wxcol_fg_hl);
			dc.SetFont(bold);
		}

		// Optional opening bracket
		if (arg.optional && !txed_calltips_dim_optional)
			left = drawText(dc, "[", left, top, &rect);

		// Type
		if (!arg.type.empty())
		{
			wxString arg_type = arg.type == "void" ? "void" : wxString::Format("%s ", arg.type);
			if (a != arg_current_)
				dc.SetTextForeground(wxcol_type);
			left = drawText(dc, arg_type, left, top, &rect);
		}

		// Name
		if (a != arg_current_)
			dc.SetTextForeground(arg.optional ? col_faded : wxcol_fg); // Faded text if optional
		left = drawText(dc, arg.name, left, top, &rect);

		// Default value
		if (!arg.default_value.empty())
			left = drawText(dc, wxString::Format(" = %s", arg.default_value), left, top, &rect);

		// Optional closing bracket
		if (arg.optional && !txed_calltips_dim_optional)
			left = drawText(dc, "]", left, top, &rect);

		// Comma (if needed)
		dc.SetFont(font_);
		dc.SetTextForeground(wxcol_fg);
		if (a < (int)context.params.size() - 1)
			left = drawText(dc, ", ", left, top, &rect);

		// Update max width
		max_right = std::max(max_right, rect.GetRight());
	}

	// Draw closing bracket
	drawText(dc, ")", left, top, &rect);
	max_right = std::max(max_right, rect.GetRight());

	return { args_left, args_top, max_right - args_left, rect.GetBottom() - args_top };
}

// -----------------------------------------------------------------------------
// Draws function text (spec+args) for [context] at [left,top].
// Returns a rect of the bounds of the drawn text
// -----------------------------------------------------------------------------
wxRect SCallTip::drawFunctionContext(
	wxDC&                      dc,
	const TLFunction::Context& context,
	int                        left,
	int                        top,
	wxColour&                  col_faded,
	wxFont&                    bold) const
{
	auto rect_func = drawFunctionSpec(dc, context, left, top);
	auto rect_args = drawArgs(dc, context, rect_func.GetRight() + 1, rect_func.GetTop(), col_faded, bold);

	return wxRect{ rect_func.GetTopLeft(),
				   wxPoint{ std::max(rect_func.GetRight(), rect_args.GetRight()),
							std::max(rect_func.GetBottom(), rect_args.GetBottom()) } };
}

// -----------------------------------------------------------------------------
// Draws function description text [desc] at [left,top].
// Returns a rect of the bounds of the drawn text
// -----------------------------------------------------------------------------
wxRect SCallTip::drawFunctionDescription(wxDC& dc, const wxString& desc, int left, int top) const
{
	auto   italic = font_.Italic();
	wxRect rect(left, top, 0, 0);
	dc.SetFont(italic);
	int max_right = 0;
	if (dc.GetTextExtent(desc).x > MAX_WIDTH)
	{
		// Description is too long, split into multiple lines
		vector<wxString> desc_lines;
		wxString         line = desc;
		wxArrayInt       extents;
		while (true)
		{
			dc.GetPartialTextExtents(line, extents);
			bool split = false;
			for (unsigned a = 0; a < extents.size(); a++)
			{
				if (extents[a] > MAX_WIDTH)
				{
					// Try to split in phrases first.
					size_t eol = (size_t)line.SubString(0, a).Last('.') + 1;
					eol        = line[eol] == ' ' ? eol : -1;
					if (eol <= 0 || eol > MAX_WIDTH)
						eol = (size_t)line.SubString(0, a).Last(',') + 1;
					if (eol <= 0 || eol > MAX_WIDTH)
						eol = (size_t)line.SubString(0, a).Last(' ');
					desc_lines.push_back(line.SubString(0, eol));
					line  = line.SubString(eol + 1, line.Length());
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

		int bottom = rect.GetBottom() + ui::scalePx(font_.GetPixelSize().GetHeight());
		for (const auto& desc_line : desc_lines)
		{
			drawText(dc, desc_line, 0, bottom, &rect);
			bottom = rect.GetBottom();
			if (rect.GetRight() > max_right)
				max_right = rect.GetRight();
		}
	}
	else
	{
		drawText(dc, desc, 0, rect.GetBottom() + ui::scalePx(font_.GetPixelSize().GetHeight()), &rect);
		if (rect.GetRight() > max_right)
			max_right = rect.GetRight();
	}

	return { left, top, max_right - left, rect.GetBottom() - top };
}

// -----------------------------------------------------------------------------
// Using [dc], draw the calltip contents at [xoff,yoff].
// Returns the dimensions of the drawn calltip text
// -----------------------------------------------------------------------------
wxSize SCallTip::drawCallTip(wxDC& dc, int xoff, int yoff)
{
	wxSize ct_size;
	auto   bold = font_.Bold();

	// Setup faded text colour
	ColRGBA faded;
	if (txed_calltips_dim_optional)
		faded = ColRGBA(
			(uint8_t)round((col_fg_.r + col_bg_.r) * 0.5),
			(uint8_t)round((col_fg_.g + col_bg_.g) * 0.5),
			(uint8_t)round((col_fg_.b + col_bg_.b) * 0.5));
	else
		faded = col_fg_;

	// Clear
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(col_bg_.toWx()));
	dc.DrawRectangle(0, 0, 1000, 1000);

	// Wx Colours (to avoid creating them multiple times)
	wxcol_fg         = col_fg_.toWx();
	wxcol_fg_hl      = col_fg_hl.toWx();
	wxcol_type       = col_type_.toWx();
	auto wxcol_faded = faded.toWx();

	if (function_)
	{
		dc.SetFont(font_);
		dc.SetTextForeground(wxcol_fg);

		wxRect rect;
		int    left      = xoff;
		int    max_right = 0;
		int    bottom    = yoff;

		// Context switching calltip
		if (switch_contexts_)
		{
			// up-filled	\xE2\x96\xB2
			// up-empty		\xE2\x96\xB3
			// down-filled	\xE2\x96\xBC
			// down-empty	\xE2\x96\xBD

			// Up arrow
			dc.SetTextForeground((btn_mouse_over_ == 2) ? wxcol_fg_hl : wxcol_fg);
			dc.DrawLabel(
				wxString::FromUTF8("\xE2\x96\xB2"), wxNullBitmap, wxRect(xoff, yoff, 100, 100), 0, -1, &rect_btn_up_);

			// Arg set
			int width = dc.GetTextExtent("X/X").x;
			dc.SetTextForeground(wxcol_fg);
			dc.DrawLabel(
				wxString::Format("%lu/%lu", context_current_ + 1, function_->contexts().size()),
				wxNullBitmap,
				wxRect(rect_btn_up_.GetRight() + ui::scalePx(4), yoff, width, 900),
				wxALIGN_CENTER_HORIZONTAL);

			// Down arrow
			dc.SetTextForeground((btn_mouse_over_ == 1) ? wxcol_fg_hl : wxcol_fg);
			dc.DrawLabel(
				wxString::FromUTF8("\xE2\x96\xBC"),
				wxNullBitmap,
				wxRect(rect_btn_up_.GetRight() + width + ui::scalePx(8), yoff, 900, 900),
				0,
				-1,
				&rect_btn_down_);

			left = rect_btn_down_.GetRight() + ui::scalePx(8);
			rect_btn_up_.Offset(wxutil::scaledPoint(12, 8));
			rect_btn_down_.Offset(wxutil::scaledPoint(12, 8));

			// Draw function (current context)
			rect      = drawFunctionContext(dc, context_, left, yoff, wxcol_faded, bold);
			max_right = rect.GetRight();
			bottom    = rect.GetBottom();
		}

		// Normal calltip - show (potentially) multiple contexts
		else
		{
			// Determine separator colour
			wxColour col_sep;
			if (col_bg_.greyscale().r < 128)
				col_sep = col_bg_.amp(30, 30, 30, 0).toWx();
			else
				col_sep = col_bg_.amp(-30, -30, -30, 0).toWx();

			bool first = true;
			auto num   = std::min<unsigned long>(function_->contexts().size(), 12u);
			for (auto a = 0u; a < num; a++)
			{
				auto& context = function_->contexts()[a];

				if (!first)
				{
					dc.SetPen(wxPen(col_sep));
					dc.DrawLine(xoff, bottom + 5, 2000, bottom + 5);
				}

				rect = drawFunctionContext(
					dc, context, xoff, bottom + (first ? 0 : ui::scalePx(11)), wxcol_faded, bold);
				bottom    = (int)round(rect.GetBottom() + ui::scaleFactor());
				max_right = std::max(max_right, rect.GetRight());
				first     = false;
			}

			// Show '... # more' if there are too many contexts
			if (function_->contexts().size() > num)
			{
				dc.SetTextForeground(wxcol_faded);
				drawText(
					dc,
					wxString::Format("... %lu more", function_->contexts().size() - num),
					xoff,
					bottom + ui::scalePx(11),
					&rect);
				bottom = (int)round(rect.GetBottom() + ui::scaleFactor());
			}

			if (num > 1)
				bottom--;
		}

		if (!rect.IsEmpty() && !context_.description.empty())
		{
			auto rect_desc = drawFunctionDescription(dc, context_.description, left, rect.GetBottom());
			max_right      = std::max(max_right, rect_desc.GetRight());
			bottom         = rect_desc.GetBottom();
		}

		// Size buffer bitmap to fit
		ct_size.SetWidth((int)round(max_right + ui::scaleFactor()));
		ct_size.SetHeight((int)round(bottom + ui::scaleFactor()));
	}
	else
	{
		// No function, empty buffer
		ct_size.SetWidth(16);
		ct_size.SetHeight(16);
	}

	return ct_size;
}

// -----------------------------------------------------------------------------
// Redraws the calltip text to the buffer image, setting the buffer image size
// to the exact dimensions of the text
// -----------------------------------------------------------------------------
void SCallTip::updateBuffer()
{
	buffer_.SetWidth(1000);
	buffer_.SetHeight(1000);

	wxMemoryDC dc(buffer_);
	auto       size = drawCallTip(dc);
	buffer_.SetWidth(size.GetWidth());
	buffer_.SetHeight(size.GetHeight());
}


// -----------------------------------------------------------------------------
//
// SCallTip Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the control is to be (re)painted
// -----------------------------------------------------------------------------
void SCallTip::onPaint(wxPaintEvent& e)
{
	// Create device context
	wxAutoBufferedPaintDC dc(this);

	// Determine border colours
	wxColour bg = col_bg_.toWx();
	wxColour border, border2;
	if (col_bg_.greyscale().r < 128)
	{
		auto c  = col_bg_.amp(50, 50, 50, 0);
		border  = c.toWx();
		c       = col_bg_.amp(20, 20, 20, 0);
		border2 = c.toWx();
	}
	else
	{
		auto c  = col_bg_.amp(-50, -50, -50, 0);
		border  = c.toWx();
		c       = col_bg_.amp(-20, -20, -20, 0);
		border2 = c.toWx();
	}

	// Draw background
	dc.SetBrush(wxBrush(bg));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(0, 0, GetSize().x, GetSize().y);

	// Draw text
#ifdef __WXOSX__
	// Not sure if it's an osx or high-dpi issue (or both),
	// but for some reason wx does not properly scale the bitmap when drawing it,
	// so just draw the entire calltip again in this case
	drawCallTip(dc, 12, 8);
#else
	dc.DrawBitmap(buffer_, ui::scalePx(12), ui::scalePx(8), true);
#endif

	// Draw border
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
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
}

// -----------------------------------------------------------------------------
// Erase background - overridden to do nothing, to avoid flickering
// -----------------------------------------------------------------------------
void SCallTip::onEraseBackground(wxEraseEvent& e)
{
	// Do nothing
}

// -----------------------------------------------------------------------------
// Called when the mouse pointer is moved within the control
// -----------------------------------------------------------------------------
void SCallTip::onMouseMove(wxMouseEvent& e)
{
	if (rect_btn_down_.Contains(e.GetPosition()))
	{
		if (btn_mouse_over_ != 1)
		{
			btn_mouse_over_ = 1;
			updateBuffer();
			Refresh();
			Update();
		}
	}

	else if (rect_btn_up_.Contains(e.GetPosition()))
	{
		if (btn_mouse_over_ != 2)
		{
			btn_mouse_over_ = 2;
			updateBuffer();
			Refresh();
			Update();
		}
	}

	else if (btn_mouse_over_ != 0)
	{
		btn_mouse_over_ = 0;
		updateBuffer();
		Refresh();
		Update();
	}
}

// -----------------------------------------------------------------------------
// Called when a mouse button is clicked within the control
// -----------------------------------------------------------------------------
void SCallTip::onMouseDown(wxMouseEvent& e)
{
	if (e.Button(wxMOUSE_BTN_LEFT))
	{
		if (btn_mouse_over_ == 1)
			nextArgSet();
		else if (btn_mouse_over_ == 2)
			prevArgSet();
	}
}

// -----------------------------------------------------------------------------
// Called when the control is shown
// -----------------------------------------------------------------------------
void SCallTip::onShow(wxShowEvent& e)
{
	if (e.IsShown())
	{
		// Get screen bounds and window bounds
		int       index = wxDisplay::GetFromWindow(this->GetParent());
		wxDisplay display((unsigned)index);
		auto      screen_area = display.GetClientArea();
		auto      ct_area     = GetScreenRect();

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
