
#include "Main.h"
#include "Widget.h"
#include "General/CVar.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"

CVAR(Float, gl_ui_scale, 1.0f, CVAR_SAVE)


double GLUI::baseScale()
{
	return gl_ui_scale;
}


using namespace GLUI;

Widget::Widget(Widget* parent)
: parent(parent),
	visible(true),
	border_style(Widget::BORDER_NONE),
	border_width(1.0f),
	border_colour(COL_WHITE),
	alpha(1.0f)
{
	if (parent)
		parent->children.push_back(this);
}

Widget::~Widget()
{
	for (auto child : children)
		delete child;
}

point2_t Widget::getAbsolutePosition()
{
	if (parent)
		return parent->getAbsolutePosition() + position;
	else
		return position;
}

point2_t Widget::middle()
{
	return point2_t(position.x + (size.x / 2), position.y + (size.y / 2));
}

void Widget::setBorder(float width, int style, rgba_t colour)
{
	border_width = width;
	border_style = style;
	border_colour = colour;
}

void Widget::draw(point2_t pos, float alpha)
{
	// Ignore if not visible
	if (!visible)
		return;

	// Draw this widget
	point2_t p = pos + position;
	drawWidget(p, alpha * this->alpha);

	// Draw all children
	for (auto child : children)
		child->draw(p, alpha * this->alpha);

	// Draw border
	if (border_style == BORDER_LINE)
	{
		OpenGL::setColour(border_colour);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_TEXTURE_2D);
		glLineWidth(border_width);
		Drawing::drawRect(p.x, p.y, p.x + getWidth(), p.y + getHeight());
	}
}

void Widget::fitToChildren(padding_t padding)
{
	if (children.empty())
		return;

	// Get min/max extents of child widgets
	int min_x = children[0]->left(true);
	int min_y = children[0]->top(true);
	int max_x = children[0]->right(true);
	int max_y = children[0]->bottom(true);
	for (unsigned a = 1; a < children.size(); a++)
	{
		Widget* child = children[a];

		if (child->left(true) < min_x)
			min_x = child->left(true);
		if (child->right(true) > max_x)
			max_x = child->right(true);
		if (child->top(true) < min_y)
			min_y = child->top(true);
		if (child->bottom(true) > max_y)
			max_y = child->bottom(true);
	}

	// Apply padding
	min_x -= padding.left;
	min_y -= padding.top;
	max_x += padding.right;
	max_y += padding.bottom;

	// Offset child widgets
	point2_t offset(min_x, min_y);
	for (auto child : children)
		child->position = child->position - offset;

	// Size to fit
	setSize(dim2_t(max_x - min_x, max_y - min_y));
}
