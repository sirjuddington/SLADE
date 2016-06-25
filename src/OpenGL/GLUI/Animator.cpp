
#include "Main.h"
#include "Animator.h"

using namespace GLUI;


SlideAnimator::SlideAnimator(int duration, int slide_amount, uint8_t slide_dir, bool fade_alpha)
	: elapsed(0),
	duration(duration),
	slide_amount(slide_amount),
	slide_dir(slide_dir),
	fade_alpha(fade_alpha),
	reverse(false),
	reverse_speed(1.0f)
{
}

SlideAnimator::~SlideAnimator()
{
}

void SlideAnimator::setSlideAmount(int amount)
{
	slide_amount = amount;
}

void SlideAnimator::setReverse(bool reverse, float speed)
{
	this->reverse = reverse;
	reverse_speed = speed;
}

void SlideAnimator::update(int time)
{
	// Update elapsed time
	if (reverse)
		elapsed -= time * reverse_speed;
	else
		elapsed += time;
	if (elapsed > duration)
		elapsed = duration;
	if (elapsed < 0)
		elapsed = 0;

	float multiplier = (float)elapsed / (float)duration;
	multiplier *= (2.0f - multiplier);

	switch (slide_dir)
	{
	case SLIDE_LEFT:
		offset.y = 0;
		offset.x = (1.0f - multiplier) * slide_amount;
		break;
	case SLIDE_RIGHT:
		offset.y = 0;
		offset.x = (1.0f - multiplier) * -slide_amount;
		break;
	case SLIDE_UP:
		offset.y = (1.0f - multiplier) * slide_amount;
		offset.x = 0;
		break;
	case SLIDE_DOWN:
		offset.y = (1.0f - multiplier) * -slide_amount;
		offset.x = 0;
		break;
	default:
		break;
	}

	if (fade_alpha)
		alpha = multiplier * multiplier;
}
