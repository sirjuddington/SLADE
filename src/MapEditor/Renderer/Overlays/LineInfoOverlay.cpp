
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    LineInfoOverlay.cpp
 * Description: LineInfoOverlay class - a map editor overlay that
 *              displays information about the currently highlighted
 *              line and its sides in lines mode
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
#include "LineInfoOverlay.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapSector.h"
#include "MapEditor/SLADEMap/MapSide.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLUI/LayoutHelpers.h"
#include "OpenGL/GLUI/Animator.h"
#include "OpenGL/GLUI/TextBox.h"
#include "OpenGL/GLUI/TextureBox.h"
#include "OpenGL/OpenGL.h"
#include "Utility/MathStuff.h"


/*******************************************************************
 * LINESIDEGLPANEL CLASS FUNCTIONS
 *******************************************************************/
using namespace GLUI;

/* LineSideGLPanel::LineSideGLPanel
 * LineSideGLPanel class constructor
 *******************************************************************/
LineSideGLPanel::LineSideGLPanel(Widget* parent, string side) : Panel(parent), side(side), bg_height(0)
{
	tex_lower = new TextureBox(this);
	tex_lower->setMargin(padding_t(4));

	tex_middle = new TextureBox(this);
	tex_middle->setMargin(padding_t(4));

	tex_upper = new TextureBox(this);
	tex_upper->setMargin(padding_t(4));

	text_info = new GLUI::TextBox(this, "INFO\nOFFSETS");

	setMargin(padding_t(4, 0));
	setBGCol(rgba_t(0, 0, 0, 0));
}

/* LineSideGLPanel::~LineSideGLPanel
 * LineSideGLPanel class destructor
 *******************************************************************/
LineSideGLPanel::~LineSideGLPanel()
{
}

/* LineSideGLPanel::update
 * Updates the info overlay with MapSide [s]
 *******************************************************************/
void LineSideGLPanel::update(MapSide* s, bool needs_upper, bool needs_middle, bool needs_lower)
{
	if (!s)
	{
		setVisible(false);
		return;
	}
	
	setVisible(true);

	int xoff = s->intProperty("offsetx");
	int yoff = s->intProperty("offsety");

	string info;
	if (Global::debug)
		info = S_FMT("%s Side #%d (%d) (Sector %d)", CHR(side), s->getIndex(), s->getId(), s->getSector()->getIndex());
	else
		info = S_FMT("%s Side #%d (Sector %d)", CHR(side), s->getIndex(), s->getSector()->getIndex());
	info += S_FMT("\nOffsets: (%d, %d)", xoff, yoff);
	text_info->setText(info);

	tex_lower->setTexture(TextureBox::TEXTURE, s->getTexLower(), "L: ", needs_lower);
	tex_middle->setTexture(TextureBox::TEXTURE, s->getTexMiddle(), "M: ", needs_middle);
	tex_upper->setTexture(TextureBox::TEXTURE, s->getTexUpper(), "U: ", needs_upper);

	updateLayout(dim2_t(-1, -1));
}

/* LineSideGLPanel::updateLayout
 * Update the widget layout to fit within [fit]. A negative [fit]
 * dimension indicates no size limit
 *******************************************************************/
void LineSideGLPanel::updateLayout(dim2_t fit)
{
	// Upper texture
	tex_upper->updateLayout();
	tex_upper->setPosition(point2_t(0, 0));

	// Middle texture
	tex_middle->updateLayout();
	LayoutHelpers::placeWidgetToRight(tex_middle, tex_upper, USE_MARGIN, Align::Top);

	// Lower texture
	tex_lower->updateLayout();
	LayoutHelpers::placeWidgetToRight(tex_lower, tex_middle, USE_MARGIN, Align::Top);

	// Info
	int width = tex_lower->right() - tex_upper->left();
	text_info->updateLayout(dim2_t(width, -1));
	LayoutHelpers::placeWidgetBelow(text_info, tex_upper, USE_MARGIN, Align::Left);

	// Size panel to fit
	fitToChildren(padding_t(0), true);
}


/*******************************************************************
 * LINEINFOOVERLAY CLASS FUNCTIONS
 *******************************************************************/

/* LineInfoOverlay::LineInfoOverlay
 * LineInfoOverlay class constructor
 *******************************************************************/
LineInfoOverlay::LineInfoOverlay()
{
	text_info = new GLUI::TextBox(this, "LINE INFO");
	ls_front = new LineSideGLPanel(this, "Front");
	ls_back = new LineSideGLPanel(this, "Back");

	text_info->setMargin(padding_t(4));
}

/* LineInfoOverlay::~LineInfoOverlay
 * LineInfoOverlay class destructor
 *******************************************************************/
LineInfoOverlay::~LineInfoOverlay()
{
}

/* LineInfoOverlay::update
 * Update the info overlay with MapLine [line]
 *******************************************************************/
void LineInfoOverlay::update(MapLine* line, int map_format)
{
	if (!line)
	{
		// Animate
		activate(false);
		return;
	}

	string info_text;

	// Animate
	activate(true);

	// General line info
	if (Global::debug)
		info_text += (S_FMT("Line #%d (%d)\n", line->getIndex(), line->getId()));
	else
		info_text += (S_FMT("Line #%d\n", line->getIndex()));
	info_text += (S_FMT("Length: %d\n", MathStuff::round(line->getLength())));

	// Line special
	int as_id = line->intProperty("special");
	if (line->props().propertyExists("macro"))
	{
		int macro = line->intProperty("macro");
		info_text += (S_FMT("Macro: #%d\n", macro));
	}
	else
		info_text += (S_FMT("Special: %d (%s)\n", as_id, theGameConfiguration->actionSpecialName(as_id)));

	// Line trigger
	if (map_format == MAP_HEXEN || map_format == MAP_UDMF)
		info_text += (S_FMT("Trigger: %s\n", theGameConfiguration->spacTriggerString(line, map_format)));

	// Line args (or sector tag)
	if (map_format == MAP_HEXEN || map_format == MAP_UDMF)
	{
		int args[5];
		args[0] = line->intProperty("arg0");
		args[1] = line->intProperty("arg1");
		args[2] = line->intProperty("arg2");
		args[3] = line->intProperty("arg3");
		args[4] = line->intProperty("arg4");
		string argstr = theGameConfiguration->actionSpecial(as_id)->getArgsString(args);
		if (!argstr.IsEmpty())
			info_text += (S_FMT("%s", argstr));
		else
			info_text += ("No Args");
	}
	else
		info_text += (S_FMT("Sector Tag: %d", line->intProperty("arg0")));

	// Line flags
	if (map_format != MAP_UDMF)
		info_text += (S_FMT("\nFlags: %s", theGameConfiguration->lineFlagsString(line)));

	text_info->setText(info_text);

	// Check needed textures
	int needed_tex = line->needsTexture();
	
	// Front side
	ls_front->update(line->s1(),
		((needed_tex & TEX_FRONT_UPPER) > 0),
		((needed_tex & TEX_FRONT_MIDDLE) > 0),
		((needed_tex & TEX_FRONT_LOWER) > 0));

	// Back side
	ls_back->update(line->s2(),
		((needed_tex & TEX_BACK_UPPER) > 0),
		((needed_tex & TEX_BACK_MIDDLE) > 0),
		((needed_tex & TEX_BACK_LOWER) > 0));
}

/* LineInfoOverlay::drawWidget
 * Draw the widget at [pos] with [alpha] transparency
 *******************************************************************/
void LineInfoOverlay::drawWidget(fpoint2_t pos, float alpha, fpoint2_t scale)
{
	glDisable(GL_TEXTURE_2D);

	OpenGL::setColour(getBGCol(alpha));
	Drawing::drawFilledRect(
		fpoint2_t(pos.x, pos.y + text_info->top(true)),
		pos + size
		);
}

/* LineInfoOverlay::updateLayout
 * Update the widget layout to fit within [fit]. A negative [fit]
 * dimension indicates no size limit
 *******************************************************************/
void LineInfoOverlay::updateLayout(dim2_t fit)
{
	// Side panels
	LayoutHelpers::placeWidgetWithin(ls_front, rect_t(point2_t(0, 0), point2_t(fit.x, 0)), Align::Right, Align::Top);
	LayoutHelpers::placeWidgetToLeft(ls_back, ls_front, USE_MARGIN, Align::Top);

	// Info text
	int width = ls_back->isVisible() ? ls_back->left(true) : ls_front->left(true);
	text_info->updateLayout(dim2_t(width, -1));
	LayoutHelpers::alignBottoms(text_info, ls_front);

	// Size panel to fit
	fitToChildren();

	ls_front->setBGHeight(text_info->getHeight() + 4);
	ls_back->setBGHeight(text_info->getHeight() + 4);
}
