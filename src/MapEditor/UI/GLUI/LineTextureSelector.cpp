
#include "Main.h"
#include "LineTextureSelector.h"
#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapSide.h"
#include "OpenGL/GLUI/ImageBox.h"
#include "OpenGL/GLUI/LayoutHelpers.h"
#include "OpenGL/GLUI/TextureBox.h"

using namespace GLUI;
using namespace std::placeholders;

class LTSTextureBox : public TextureBox
{
public:
	LTSTextureBox(Widget* parent) : TextureBox(parent), anim_visible(nullptr)
	{
		setBoxSize(192);
		setMargin(padding_t(8));
		image_texture->setMaxImageScale(4.0);
		show_always = true;

		// Visible animation
		setStandardAnimation(
			Widget::StdAnim::Visible,
			std::move(std::make_unique<ScaleAnimator>(this, 200, 0.8f, 1.0f, Animator::Easing::Out))
		);

		// Mouseover animation
		setStandardAnimation(
			Widget::StdAnim::MouseOver,
			std::make_unique<ScaleAnimator>(this, 100, 1.0f, 1.05f, Animator::Easing::Out)
		);

		evt_mouse_enter.bind(this, EventFunc([this](EventInfo& e)
		{
			//image_texture->setBorderWidth(3.0f);
		}
		));

		evt_mouse_leave.bind(this, EventFunc([this](EventInfo& e)
		{
			//image_texture->setBorderWidth(1.0f);
		}
		));
	}

	~LTSTextureBox() {}

private:
	ScaleAnimator*	anim_visible;
};



LTSPanel::LTSPanel(Widget* parent) : Panel(parent)
{
	tex_front_lower = new LTSTextureBox(this);
	tex_front_middle = new LTSTextureBox(this);
	tex_front_upper = new LTSTextureBox(this);
	tex_back_lower = new LTSTextureBox(this);
	tex_back_middle = new LTSTextureBox(this);
	tex_back_upper = new LTSTextureBox(this);

	setBGCol(rgba_t(0, 0, 0, 0));
}

LTSPanel::~LTSPanel()
{
}

void LTSPanel::setLine(MapLine* line)
{
	tex_front_lower->setVisible(false, false);
	tex_front_middle->setVisible(false, false);
	tex_front_upper->setVisible(false, false);
	tex_back_lower->setVisible(false, false);
	tex_back_middle->setVisible(false, false);
	tex_back_upper->setVisible(false, false);

	if (line->s1())
	{
		tex_front_lower->setVisible(true);
		tex_front_middle->setVisible(true);
		tex_front_upper->setVisible(true);
		tex_front_lower->setTexture(TextureBox::TEXTURE, line->s1()->getTexLower(), "Front Lower: ");
		tex_front_middle->setTexture(TextureBox::TEXTURE, line->s1()->getTexMiddle(), "Front Middle: ");
		tex_front_upper->setTexture(TextureBox::TEXTURE, line->s1()->getTexUpper(), "Front Upper: ");
	}

	if (line->s2())
	{
		tex_back_lower->setVisible(true);
		tex_back_middle->setVisible(true);
		tex_back_upper->setVisible(true);
		tex_back_lower->setTexture(TextureBox::TEXTURE, line->s2()->getTexLower(), "Back Lower: ");
		tex_back_middle->setTexture(TextureBox::TEXTURE, line->s2()->getTexMiddle(), "Back Middle: ");
		tex_back_upper->setTexture(TextureBox::TEXTURE, line->s2()->getTexUpper(), "Back Upper: ");
	}
}

void LTSPanel::updateLayout(dim2_t fit)
{
	tex_front_lower->updateLayout(fit);
	tex_front_middle->updateLayout(fit);
	tex_front_upper->updateLayout(fit);
	tex_back_lower->updateLayout(fit);
	tex_back_middle->updateLayout(fit);
	tex_back_upper->updateLayout(fit);

	// TODO: Scale to fit within [fit]?

	// Front
	tex_front_lower->setPosition(point2_t(0, 0));
	LayoutHelpers::placeWidgetToRight(tex_front_middle, tex_front_lower, -1, Align::Middle);
	LayoutHelpers::placeWidgetToRight(tex_front_upper, tex_front_middle, -1, Align::Middle);

	// Back
	LayoutHelpers::placeWidgetBelow(tex_back_lower, tex_front_lower, -1, Align::Middle);
	LayoutHelpers::placeWidgetToRight(tex_back_middle, tex_back_lower, -1, Align::Middle);
	LayoutHelpers::placeWidgetToRight(tex_back_upper, tex_back_middle, -1, Align::Middle);

	fitToChildren();
}




LineTextureSelector::LineTextureSelector() : Panel(NULL), panel_textures(new LTSPanel(this))
{
	setBGCol(rgba_t(0, 0, 0, 128));
	evt_key_down.bind(this, std::bind(&LineTextureSelector::onKeyDown, this, _1));

	setStandardAnimation(StdAnim::Visible, std::make_unique<FadeAnimator>(this, 200, 0.0f, 1.0f, Animator::Easing::Out));
}

LineTextureSelector::~LineTextureSelector()
{
}

void LineTextureSelector::updateLayout(dim2_t fit)
{
	// Layout texture panel
	panel_textures->updateLayout(dim2_t(-1, -1));

	// Place in middle
	LayoutHelpers::placeWidgetWithinParent(panel_textures, Align::Middle, Align::Middle);
}

void LineTextureSelector::onKeyDown(KeyEventInfo& e)
{
	if (e.key == "escape")
	{
		setVisible(false);
		e.handled = true;
	}
}
