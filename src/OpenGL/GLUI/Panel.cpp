
#include "Main.h"
#include "Panel.h"
#include "General/ColourConfiguration.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"

using namespace GLUI;

Panel::Panel(Widget* parent) : Widget(parent)
{
	col_bg.set(ColourConfiguration::getColour("map_overlay_background"));
}

Panel::~Panel()
{
}

void Panel::drawWidget(point2_t pos, float alpha)
{
	glDisable(GL_TEXTURE_2D);

	OpenGL::setColour(getBGCol(alpha));
	Drawing::drawFilledRect(pos, pos + getSize());
}

rgba_t Panel::getDefaultBGCol()
{
	return ColourConfiguration::getColour("map_overlay_background");
}
