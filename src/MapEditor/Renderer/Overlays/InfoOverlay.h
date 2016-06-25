#pragma once

#include "OpenGL/GLUI/Panel.h"
#include "OpenGL/GLUI/Animator.h"

class InfoOverlay : public GLUI::Panel
{
public:
	InfoOverlay() : GLUI::Panel(nullptr)
	{
		anim_activate = new GLUI::SlideAnimator(100, 0, GLUI::SlideAnimator::SLIDE_UP);
		animators.push_back(anim_activate);
	}
	~InfoOverlay() {}

	void activate(bool activate)
	{
		if (activate)
			anim_activate->setReverse(false);
		else
			anim_activate->setReverse(true, 0.4f);
	}

	void onSizeChanged() override
	{
		anim_activate->setSlideAmount(getHeight());
	}

protected:
	GLUI::SlideAnimator*	anim_activate;
};
