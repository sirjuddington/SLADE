#pragma once

namespace GLUI
{
	class Widget;
	class Animator
	{
	public:
		Animator() : alpha(1.0f) {}
		~Animator() {}

		point2_t	getOffset() { return offset; }
		float		getAlpha() { return alpha; }

		virtual void	update(int time) {}
		virtual void	applyAnimation(Widget* target) {}

	protected:
		point2_t	offset;
		float		alpha;
	};

	class SlideAnimator : public Animator
	{
	public:
		SlideAnimator(int duration, int slide_amount, uint8_t slide_dir, bool fade_alpha = true);
		~SlideAnimator();

		enum
		{
			SLIDE_LEFT = 0,
			SLIDE_UP,
			SLIDE_RIGHT,
			SLIDE_DOWN
		};

		void	setSlideAmount(int amount);
		void	setReverse(bool reverse, float speed = 1.0f);
		void	update(int time);

	private:
		int		elapsed;
		int		duration;
		int		slide_amount;
		uint8_t	slide_dir;
		bool	fade_alpha;
		bool	reverse;
		float	reverse_speed;
	};
}
