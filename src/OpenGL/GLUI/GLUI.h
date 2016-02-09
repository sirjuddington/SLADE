#pragma once

namespace GLUI
{
	enum
	{
		ALIGN_NONE = 0,
		ALIGN_LEFT = 1,
		ALIGN_RIGHT = 2,
		ALIGN_TOP = 1,
		ALIGN_BOTTOM = 2,
		ALIGN_MIDDLE = 3,
		ALIGN_FILL = 4
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
			this->left = left;
			this->top = top;
			this->right = right;
			this->bottom = bottom;
		}

		int horizontal() { return left + right; }
		int vertical() { return top + bottom; }
	};
}
