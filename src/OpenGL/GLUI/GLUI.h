#pragma once

namespace GLUI
{
	double	baseScale();

	enum Align
	{
		None = 0,
		Left = 1,
		Right = 2,
		Top = 1,
		Bottom = 2,
		Middle = 3,
		Fill = 4
	};

	struct padding_t
	{
		int left;
		int top;
		int right;
		int bottom;

		padding_t(int left, int top, int right, int bottom) { set(left, top, right, bottom); }
		padding_t(int horizontal, int vertical) { set(horizontal, vertical, horizontal, vertical); }
		padding_t(int padding = 0) { set(padding, padding, padding, padding); }

		void set(int left, int top, int right, int bottom)
		{
			this->left = left * baseScale();
			this->top = top * baseScale();
			this->right = right * baseScale();
			this->bottom = bottom * baseScale();
		}

		int horizontal() { return left + right; }
		int vertical() { return top + bottom; }
	};
}
