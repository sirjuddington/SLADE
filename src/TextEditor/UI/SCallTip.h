#pragma once

#include "common.h"
#include "TextEditor/TextLanguage.h"

#define SCALLTIP_MAX_WIDTH 800

class SCallTip : public wxPopupWindow
{
public:
	SCallTip(wxWindow* parent);
	~SCallTip();

	void	setBackgroundColour(rgba_t col) { col_bg_ = col; }
	void	setTextColour(rgba_t col) { col_fg_ = col_func_ = col_type_ = col_keyword_ = col; }
	void	setTextHighlightColour(rgba_t col) { col_fg_hl = col; }
	void	setFunctionColour(rgba_t col) { col_func_ = col; }
	void	setTypeColour(rgba_t col) { col_type_ = col; }
	void	setKeywordColour(rgba_t col) { col_keyword_ = col; }
	void	setCurrentArg(int arg) { arg_current_ = arg; updateSize(); }
	void	enableArgSwitch(bool enable) { switch_contexts_ = enable; }
	void	setFont(string face, int size);

	void	openFunction(TLFunction* function, int arg = -1);
	void	nextArgSet();
	void	prevArgSet();

private:
	TLFunction*			function_;
	TLFunction::Context	context_;

	rgba_t			col_bg_;
	rgba_t			col_fg_;
	rgba_t			col_fg_hl;
	rgba_t			col_func_;
	rgba_t			col_type_;
	rgba_t			col_keyword_;
	int				arg_current_;
	bool			switch_contexts_;
	int				context_current_;
	wxRect			rect_btn_up_;
	wxRect			rect_btn_down_;
	int				btn_mouse_over_;
	wxBitmap		buffer_;
	wxFont			font_;

	void	loadContext(int index);
	void	updateSize();

	int		drawText(wxDC& dc, string text, int left, int top, wxRect* bounds);
	wxSize	drawCallTip(wxDC& dc, int xoff = 0, int yoff = 0);
	void	updateBuffer();

	// Events
	void	onPaint(wxPaintEvent& e);
	void	onEraseBackground(wxEraseEvent& e);
	void	onMouseMove(wxMouseEvent& e);
	void	onMouseDown(wxMouseEvent& e);
	void	onShow(wxShowEvent& e);
};
