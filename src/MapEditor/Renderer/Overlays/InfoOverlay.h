#pragma once

#include "OpenGL/GLUI/Panel.h"
#include "OpenGL/GLUI/Animator.h"

class InfoOverlay : public GLUI::Panel
{
public:
	InfoOverlay() : GLUI::Panel(nullptr)
	{
		anim_activate = new GLUI::SlideAnimator(
			this,
			100,
			0,
			GLUI::SlideAnimator::Direction::Up,
			true,
			GLUI::Animator::Easing::Out
		);
		animators.push_back(GLUI::Animator::Ptr(anim_activate));

		evt_size_changed.bind(this, GLUI::EventFunc([this](auto e)
		{
			anim_activate->setSlideAmount(getHeight());
		}));
	}
	~InfoOverlay() {}

	void activate(bool activate)
	{
		if (activate)
			anim_activate->setReverse(false);
		else
			anim_activate->setReverse(true, 0.4f);
	}

protected:
	GLUI::SlideAnimator*	anim_activate;
};
