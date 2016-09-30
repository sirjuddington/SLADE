#pragma once

namespace GLUI
{
	class Widget;
	class Animator
	{
	public:
		typedef std::unique_ptr<Animator> Ptr;

		enum class Easing
		{
			None,
			In,
			Out,
		};

		Animator(Widget* parent, int duration = 0, Easing easing = Easing::None);
		~Animator() {}

		fpoint2_t	getOffset() { return offset; }
		fpoint2_t	getScale() { return scale; }
		float		getAlpha() { return alpha; }

		void	setReverse(bool reverse, float speed = 0.0f);
		void	setEasing(Easing easing) { this->easing = easing; }
		void	reset() { elapsed = reverse ? duration : 0; }

		virtual void	update(int time) {}
		virtual void	applyAnimation(Widget* target) {}
		virtual bool	isActive();

	protected:
		Widget*		parent;
		fpoint2_t	offset;
		fpoint2_t	scale;
		float		alpha;
		int			elapsed;
		int			duration;
		bool		reverse;
		float		reverse_speed;
		Easing		easing;

		void	updateElapsed(int time);
		float	animMultiplier();
	};

	class FadeAnimator : public Animator
	{
	public:
		FadeAnimator(
			Widget* parent,
			int duration,
			float fade_from = 0.0f,
			float fade_to = 1.0f,
			Easing easing = Easing::None
		);
		~FadeAnimator() {}

		void	update(int time) override;

	private:
		float	fade_from;
		float	fade_to;
	};

	class SlideAnimator : public Animator
	{
	public:
		enum class Direction
		{
			Left = 0,
			Up,
			Right,
			Down
		};

		SlideAnimator(
			Widget* parent,
			int duration,
			int slide_amount,
			Direction slide_dir,
			bool fade_alpha = true,
			Easing easing = Easing::None
		);
		~SlideAnimator();

		void	setSlideAmount(int amount);
		void	update(int time) override;

	private:
		int			slide_amount;
		Direction	slide_dir;
		bool		fade_alpha;
	};

	class ScaleAnimator : public Animator
	{
	public:
		ScaleAnimator(
			Widget* parent,
			int duration,
			float scale_begin,
			float scale_end,
			Easing easing = Easing::None
		);
		~ScaleAnimator();

		void update(int time) override;

	private:
		float	scale_begin;
		float	scale_end;
	};
}
