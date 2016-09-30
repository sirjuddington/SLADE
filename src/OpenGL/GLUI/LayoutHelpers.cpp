
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    LayoutHelpers.cpp
 * Description: Various functions to help with layout of GLUI widgets
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
#include "Widget.h"
#include "LayoutHelpers.h"

using namespace GLUI;


/*******************************************************************
 * LAYOUTHELPERS NAMESPACE FUNCTIONS
 *******************************************************************/

/* LayoutHelpers::placeWidgetAbove
 * Places [widget] above [base], with [padding] between them. Use
 * [align] to specify how [widget] will be horizontally aligned
 * relative to [base]
 *******************************************************************/
void LayoutHelpers::placeWidgetAbove(Widget* widget, Widget* base, int padding, int align)
{
	if (padding == USE_MARGIN)
		padding = base->getMargin().top + widget->getMargin().bottom;
	else
		padding *= baseScale();

	widget->setPosition(point2_t(
		widget->left(),
		base->top() - base->getHeight() - padding
		));

	switch (align)
	{
	case Align::Left: alignLefts(widget, base); break;
	case Align::Right: alignRights(widget, base); break;
	case Align::Middle: alignMiddlesH(widget, base); break;
	case Align::Fill: alignLefts(widget, base); sameWidth(widget, base); break;
	default: break;
	}
}

/* LayoutHelpers::placeWidgetBelow
 * Places [widget] below [base], with [padding] between them. Use
 * [align] to specify how [widget] will be horizontally aligned
 * relative to [base]
 *******************************************************************/
void LayoutHelpers::placeWidgetBelow(Widget* widget, Widget* base, int padding, int align)
{
	if (padding == USE_MARGIN)
		padding = base->getMargin().bottom + widget->getMargin().top;
	else
		padding *= baseScale();

	widget->setPosition(point2_t(
		widget->left(),
		base->bottom() + padding
		));

	switch (align)
	{
	case Align::Left: alignLefts(widget, base); break;
	case Align::Right: alignRights(widget, base); break;
	case Align::Middle: alignMiddlesH(widget, base); break;
	case Align::Fill: alignLefts(widget, base); sameWidth(widget, base); break;
	default: break;
	}
}

/* LayoutHelpers::placeWidgetToLeft
 * Places [widget] left of [base], with [padding] between them. Use
 * [align] to specify how [widget] will be vertically aligned
 * relative to [base]
 *******************************************************************/
void LayoutHelpers::placeWidgetToLeft(Widget* widget, Widget* base, int padding, int align)
{
	if (padding == USE_MARGIN)
		padding = base->getMargin().left + widget->getMargin().right;
	else
		padding *= baseScale();

	widget->setPosition(point2_t(
		base->left() - widget->getWidth() - padding,
		widget->top()
		));

	switch (align)
	{
	case Align::Top: alignTops(widget, base); break;
	case Align::Bottom: alignBottoms(widget, base); break;
	case Align::Middle: alignMiddlesV(widget, base); break;
	case Align::Fill: alignTops(widget, base); sameHeight(widget, base); break;
	default: break;
	}
}

/* LayoutHelpers::placeWidgetToRight
 * Places [widget] right of [base], with [padding] between them. Use
 * [align] to specify how [widget] will be vertically aligned
 * relative to [base]
 *******************************************************************/
void LayoutHelpers::placeWidgetToRight(Widget* widget, Widget* base, int padding, int align)
{
	if (padding == USE_MARGIN)
		padding = base->getMargin().right + widget->getMargin().left;
	else
		padding *= baseScale();

	widget->setPosition(point2_t(
		base->right() + padding,
		widget->top()
		));

	switch (align)
	{
	case Align::Top: alignTops(widget, base); break;
	case Align::Bottom: alignBottoms(widget, base); break;
	case Align::Middle: alignMiddlesV(widget, base); break;
	case Align::Fill: alignTops(widget, base); sameHeight(widget, base); break;
	default: break;
	}
}

void LayoutHelpers::placeWidgetWithin(Widget* widget, rect_t rect, int align_h, int align_v, padding_t padding)
{
	padding_t margin = widget->getMargin();

	// Place horizontally
	switch (align_h)
	{
	case Align::Left:
		widget->setPosition(point2_t(
			rect.left() + padding.left + margin.left,
			widget->top()
			));
		break;
	case Align::Right:
		widget->setPosition(point2_t(
			rect.right() - padding.right - widget->getWidth() - margin.right,
			widget->top()
			));
		break;
	case Align::Middle:
		widget->setPosition(point2_t(
			rect.middle().x - (widget->getWidth() / 2),
			widget->top()
			));
		break;
	case Align::Fill:
		widget->setPosition(point2_t(
			rect.left() + padding.left + margin.left,
			widget->top()
			));
		widget->setSize(dim2_t(
			rect.width() - padding.horizontal() - margin.horizontal(),
			widget->getHeight()
			));
	default:
		break;
	}

	// Place vertically
	switch (align_v)
	{
	case Align::Top:
		widget->setPosition(point2_t(
			widget->left(),
			rect.top() + padding.top + margin.top
			));
		break;
	case Align::Bottom:
		widget->setPosition(point2_t(
			widget->left(),
			rect.bottom() - padding.bottom - widget->getHeight() - margin.bottom
			));
		break;
	case Align::Middle:
		widget->setPosition(point2_t(
			widget->left(),
			rect.middle().y - (widget->getHeight() / 2)
			));
		break;
	case Align::Fill:
		widget->setPosition(point2_t(
			widget->left(),
			rect.top() + padding.top + margin.top
			));
		widget->setSize(dim2_t(
			widget->getWidth(),
			rect.height() - padding.vertical() - margin.vertical()
			));
		break;
	default:
		break;
	}
}

void LayoutHelpers::placeWidgetWithinParent(Widget* widget, int align_h, int align_v, padding_t padding)
{
	// Get parent widget
	Widget* p = widget->getParent();
	if (!p)
		return;

	placeWidgetWithin(widget, rect_t(0, 0, p->getWidth(), p->getHeight(), 0), align_h, align_v, padding);
}

void LayoutHelpers::alignLefts(Widget* widget, Widget* base)
{
	widget->setPosition(point2_t(
		base->left(),
		widget->top()
		));
}

void LayoutHelpers::alignTops(Widget* widget, Widget* base)
{
	widget->setPosition(point2_t(
		widget->left(),
		base->top()
		));
}

void LayoutHelpers::alignRights(Widget* widget, Widget* base)
{
	widget->setPosition(point2_t(
		base->right() - widget->getWidth(),
		widget->top()
		));
}

void LayoutHelpers::alignBottoms(Widget* widget, Widget* base)
{
	widget->setPosition(point2_t(
		widget->left(),
		base->bottom() - widget->getHeight()
		));
}

void LayoutHelpers::alignMiddlesV(Widget* widget, Widget* base)
{
	widget->setPosition(point2_t(
		widget->left(),
		base->middle().y - (widget->getHeight() / 2)
		));
}

void LayoutHelpers::alignMiddlesH(Widget* widget, Widget* base)
{
	widget->setPosition(point2_t(
		base->middle().x - (widget->getWidth() / 2),
		widget->top()
		));
}

void LayoutHelpers::sameWidth(Widget* widget, Widget* base)
{
	widget->setSize(dim2_t(base->getWidth(), widget->getHeight()));
}

void LayoutHelpers::sameWidthLargest(Widget* w1, Widget* w2)
{
	if (w1->getWidth() > w2->getWidth())
		w2->setSize(dim2_t(w1->getWidth(), w2->getHeight()));
	else
		w1->setSize(dim2_t(w2->getWidth(), w1->getHeight()));
}

void LayoutHelpers::sameHeight(Widget* widget, Widget* base)
{
	widget->setSize(dim2_t(widget->getWidth(), base->getHeight()));
}

void LayoutHelpers::sameHeightLargest(Widget* w1, Widget* w2)
{
	if (w1->getHeight() > w2->getHeight())
		w2->setSize(dim2_t(w2->getWidth(), w1->getHeight()));
	else
		w1->setSize(dim2_t(w1->getWidth(), w2->getHeight()));
}
