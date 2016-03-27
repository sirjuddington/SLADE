#pragma once

#include <wx/popupwin.h>
#include <wx/dcbuffer.h>

class TLFunction;
class SCallTip : public wxPopupWindow
{
public:
	SCallTip(wxWindow* parent);
	~SCallTip();

	void	setBackgroundColour(rgba_t col) { col_bg = col; }
	void	setTextColour(rgba_t col) { col_fg = col_func = col_type = col; }
	void	setTextHighlightColour(rgba_t col) { col_fg_hl = col; }
	void	setFunctionColour(rgba_t col) { col_func = col; }
	void	setTypeColour(rgba_t col) { col_type = col; }
	void	setCurrentArg(int arg) { arg_current = arg; updateSize(); }
	void	enableArgSwitch(bool enable) { switch_args = enable; }

	void	openFunction(TLFunction* function, int arg = -1);
	void	nextArgSet();
	void	prevArgSet();


	struct arg_t
	{
		string	type;
		string	name;
		bool	optional;
		arg_t() : optional(false) {}
	};

private:
	rgba_t			col_bg;
	rgba_t			col_fg;
	rgba_t			col_fg_hl;
	rgba_t			col_func;
	rgba_t			col_type;
	TLFunction*		function;
	vector<arg_t>	args;
	int				arg_current;
	bool			switch_args;
	int				arg_set_current;
	wxRect			rect_btn_up;
	wxRect			rect_btn_down;
	int				btn_mouse_over;
	wxBitmap		buffer;

	void	addArg(vector<string>& tokens);
	void	loadArgSet(int set);
	void	updateSize();

	int		drawText(wxDC& dc, string text, int left, int top, wxRect* bounds);
	void	updateBuffer();

	// Events
	void	onPaint(wxPaintEvent& e);
	void	onEraseBackground(wxEraseEvent& e);
	void	onMouseMove(wxMouseEvent& e);
	void	onMouseDown(wxMouseEvent& e);
};
