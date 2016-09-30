
#include "Main.h"
#include "Widget.h"
#include "Animator.h"
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
	border_style(Border::None),
	border_width(1.0f),
	border_colour(COL_WHITE),
	alpha(1.0f),
	mouse_over(false)
{
	if (parent)
		parent->children.push_back(Ptr(this));
}

vector<Widget*>	Widget::getChildren()
{
	vector<Widget*> ret;
	for (auto& child : children)
		ret.push_back(child.get());
	return ret;
}

bool Widget::isVisible()
{
	// Check if (in)visible animation is running
	if (!visible && anim_standard[StdAnim::Visible])
		return anim_standard[StdAnim::Visible]->isActive();

	return visible;
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

void Widget::setPosition(point2_t pos)
{
	position = pos;
	evt_pos_changed.invoke(EventInfo{ this });
}

void Widget::setSize(dim2_t dim)
{
	size = dim;
	evt_size_changed.invoke(EventInfo{ this });
}

void Widget::setVisible(bool vis, bool animate)
{
	visible = vis;

	if (animate && anim_standard[StdAnim::Visible])
	{
		anim_standard[StdAnim::Visible]->setReverse(!vis);
		anim_standard[StdAnim::Visible]->reset();
	}

	evt_visible_changed.invoke(EventInfo{ this });
}

void Widget::setBorder(float width, Border style, rgba_t colour)
{
	border_width = width;
	border_style = style;
	border_colour = colour;
}

void Widget::draw(fpoint2_t pos, float alpha, fpoint2_t scale)
{
	// Ignore if not visible
	if (!isVisible())
		return;

	// Get scale
	fpoint2_t draw_scale = getAnimatedScale();
	draw_scale.x *= scale.x;
	draw_scale.y *= scale.y;
	//bool is_scaled = (scale.x != 1.0 || scale.y != 1.0);

	//// Apply scale
	//if (is_scaled)
	//{
	//	glPushMatrix();
	//	glScaled(scale.x, scale.y, 1.0);
	//}

	// Draw this widget
	//fpoint2_t p = pos + position + getAnimatedOffset();
	//fpoint2_t p = getAnimatedOffset();
	//p.x = pos.x + ((p.x + position.x) * draw_scale.x);
	//p.y = pos.y + ((p.y + position.y) * draw_scale.y);
	fpoint2_t ofs = getAnimatedOffset();
	fpoint2_t p = ofs + pos + fpoint2_t(position.x * scale.x, position.y * scale.y);

	drawWidget(p, alpha * this->alpha * getAnimatedAlpha(), draw_scale);

	// Draw all children
	for (auto& child : children)
		child->draw(p, alpha * this->alpha * getAnimatedAlpha(), draw_scale);

	// Draw border
	if (border_style == Border::Line)
	{
		rgba_t bc = border_colour;
		bc.a *= alpha * this->alpha * getAnimatedAlpha();
		OpenGL::setColour(bc);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_TEXTURE_2D);
		glLineWidth(border_width);
		Drawing::drawRect(p.x, p.y, p.x + (getWidth() * draw_scale.x), p.y + (getHeight() * draw_scale.y));
	}

	//// Reset scale
	//if (is_scaled)
	//	glPopMatrix();
}

void Widget::fitToChildren(padding_t padding, bool include_invisible)
{
	if (children.empty())
		return;

	// Get min/max extents of visible child widgets
	int min_x = 99999999;
	int min_y = 99999999;
	int max_x = -99999999;
	int max_y = -99999999;
	bool has_visible = false;
	for (auto& child : children)
	{
		if (child->isVisible() || include_invisible)
		{
			if (child->left(true) < min_x)
				min_x = child->left(true);
			if (child->right(true) > max_x)
				max_x = child->right(true);
			if (child->top(true) < min_y)
				min_y = child->top(true);
			if (child->bottom(true) > max_y)
				max_y = child->bottom(true);

			has_visible = true;
		}
	}

	if (!has_visible)
		min_x = min_y = max_x = max_y = 0;

	// Apply padding
	min_x -= padding.left;
	min_y -= padding.top;
	max_x += padding.right;
	max_y += padding.bottom;

	// Offset child widgets
	if (has_visible)
	{
		point2_t offset(min_x, min_y);
		for (auto& child : children)
			child->position = child->position - offset;
	}

	// Size to fit
	setSize(dim2_t(max_x - min_x, max_y - min_y));
}

fpoint2_t Widget::getAnimatedOffset()
{
	fpoint2_t offset;
	for (auto& animator : animators)
		offset = offset + animator->getOffset();
	return offset;
}

fpoint2_t Widget::getAnimatedScale()
{
	fpoint2_t scale(1.0, 1.0);
	for (auto& animator : animators)
	{
		scale.x *= animator->getScale().x;
		scale.y *= animator->getScale().y;
	}
	return scale;
}

float Widget::getAnimatedAlpha()
{
	float alpha = 1.0f;
	for (auto& animator : animators)
		alpha = alpha * animator->getAlpha();
	return alpha;
}

void Widget::animate(int time)
{
	for (auto& animator : animators)
		animator->update(time);

	for (auto& child : children)
		child->animate(time);
}

void Widget::setStandardAnimation(StdAnim anim, Animator::Ptr animator)
{
	Animator* a = animator.get();
	animators.push_back(std::move(animator));
	anim_standard[anim] = a;

	if (anim == StdAnim::MouseOver)
		a->setReverse(true);
}

void Widget::mouseMove(int x, int y)
{
	// Handle mouse move on children first
	for (auto& child : children)
	{
		// Check position is within child widget
		if (x >= child->left() && x <= child->right() &&
			y >= child->top() && y <= child->bottom())
		{
			child->mouseOver(true);
			child->mouseMove(x - child->left(), y - child->top());
		}
		else
			child->mouseOver(false);
	}

	evt_mouse_move.invoke(MouseEventInfo{ this, x, y, MouseBtn::None });
}

void Widget::mouseOver(bool is_over)
{
	// Mouse enter/leave event
	if (!mouse_over && is_over)
	{
		evt_mouse_enter.invoke(EventInfo{ this });

		if (anim_standard[StdAnim::MouseOver])
			anim_standard[StdAnim::MouseOver]->setReverse(false);
	}
	if (mouse_over && !is_over)
	{
		evt_mouse_leave.invoke(EventInfo{ this });

		if (anim_standard[StdAnim::MouseOver])
			anim_standard[StdAnim::MouseOver]->setReverse(true);
	}

	mouse_over = is_over;

	// If the mouse isn't over this widget, it can't be over any children either
	if (!is_over)
		for (auto& child : children)
			child->mouseOver(is_over);
}

void Widget::mouseButtonDown(MouseBtn button, int x, int y)
{
	// Handle mouse click on children first
	for (auto& child : children)
	{
		if (child->mouse_over)
			child->mouseButtonDown(button, x - child->left(), y - child->top());
	}

	// Do event
	evt_mouse_down.invoke(MouseEventInfo{ this, x, y, button });
}

void Widget::mouseButtonUp(MouseBtn button, int x, int y)
{
	// Handle mouse click on children first
	for (auto& child : children)
	{
		if (child->mouse_over)
			child->mouseButtonUp(button, x - child->left(), y - child->top());
	}

	// Do event
	evt_mouse_up.invoke(MouseEventInfo{ this, x, y, button });
}

bool Widget::keyDown(string key, bool shift, bool ctrl, bool alt)
{
	// Handle key down on children first
	bool handled = false;
	for (auto& child : children)
		if (child->keyDown(key, shift, ctrl, alt))
		{
			handled = true;
			break;
		}

	// Do event
	if (!handled)
	{
		KeyEventInfo evt(this, key, shift, ctrl, alt);
		evt_key_down.invoke(evt);
		handled = evt.handled;
	}

	return handled;
}

bool Widget::keyUp(string key, bool shift, bool ctrl, bool alt)
{
	// Handle key up on children first
	bool handled = false;
	for (auto& child : children)
		if (child->keyUp(key, shift, ctrl, alt))
		{
			handled = true;
			break;
		}

	// Do event
	if (!handled)
	{
		KeyEventInfo evt(this, key, shift, ctrl, alt);
		evt_key_up.invoke(evt);
		handled = evt.handled;
	}

	return handled;
}
