
#include "Main.h"
#include "Animator.h"
#include "Widget.h"

using namespace GLUI;

Animator::Animator(Widget* parent, int duration, Easing easing)
	: parent(parent),
	offset(0, 0),
	scale(1, 1),
	alpha(1.0f),
	duration(duration),
	elapsed(0),
	reverse(false),
	reverse_speed(1.0f),
	easing(easing)
{
}

bool Animator::isActive()
{
	return ((reverse && elapsed > 0) || (!reverse && elapsed < duration));
}

void Animator::setReverse(bool reverse, float speed)
{
	this->reverse = reverse;

	if (speed != 0.0f)
		reverse_speed = speed;
}

void Animator::updateElapsed(int time)
{
	if (reverse)
		elapsed -= time * reverse_speed;
	else
		elapsed += time;

	if (elapsed > duration)
		elapsed = duration;
	if (elapsed < 0)
		elapsed = 0;
}

float Animator::animMultiplier()
{
	float multiplier = (float)elapsed / (float)duration;

	// Ease Out
	if (easing == Easing::Out)
		multiplier *= (2.0f - multiplier);

	// Ease In
	else if (easing == Easing::In)
		multiplier *= multiplier;

	return multiplier;
}


FadeAnimator::FadeAnimator(Widget* parent, int duration, float fade_from, float fade_to, Easing easing)
	: Animator(parent, duration, easing), fade_from(fade_from), fade_to(fade_to)
{
}

void FadeAnimator::update(int time)
{
	updateElapsed(time);
	alpha = fade_from + ((fade_to - fade_from) * animMultiplier());
}


SlideAnimator::SlideAnimator(
	Widget* parent,
	int duration,
	int slide_amount,
	Direction slide_dir,
	bool fade_alpha,
	Easing easing
)
	: Animator(parent, duration), slide_amount(slide_amount), slide_dir(slide_dir), fade_alpha(fade_alpha)
{
}

SlideAnimator::~SlideAnimator()
{
}

void SlideAnimator::setSlideAmount(int amount)
{
	slide_amount = amount;
}

void SlideAnimator::update(int time)
{
	// Update elapsed time
	updateElapsed(time);

	float multiplier = animMultiplier();

	switch (slide_dir)
	{
	case Direction::Left:
		offset.y = 0;
		offset.x = (1.0f - multiplier) * slide_amount;
		break;
	case Direction::Right:
		offset.y = 0;
		offset.x = (1.0f - multiplier) * -slide_amount;
		break;
	case Direction::Up:
		offset.y = (1.0f - multiplier) * slide_amount;
		offset.x = 0;
		break;
	case Direction::Down:
		offset.y = (1.0f - multiplier) * -slide_amount;
		offset.x = 0;
		break;
	default:
		break;
	}

	if (fade_alpha)
		alpha = multiplier * multiplier;
}


ScaleAnimator::ScaleAnimator(Widget* parent, int duration, float scale_begin, float scale_end, Easing easing)
	: Animator(parent, duration, easing), scale_begin(scale_begin), scale_end(scale_end)
{
}

ScaleAnimator::~ScaleAnimator()
{
}

void ScaleAnimator::update(int time)
{
	updateElapsed(time);

	float multiplier = animMultiplier();

	float scale_diff = scale_end - scale_begin;
	scale.set(scale_begin + scale_diff * multiplier, scale_begin + scale_diff * multiplier);

	if (parent != nullptr)
	{
		double width = (double)parent->getWidth();
		double height = (double)parent->getHeight();
		offset.x = (width - (width * scale.x)) * 0.5;
		offset.y = (height - (height * scale.y)) * 0.5;
	}
}
