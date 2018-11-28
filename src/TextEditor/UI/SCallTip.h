#pragma once

#include "TextEditor/TextLanguage.h"

class SCallTip : public wxPopupWindow
{
public:
	SCallTip(wxWindow* parent);
	~SCallTip();

	void setBackgroundColour(rgba_t col) { col_bg_ = col; }
	void setTextColour(rgba_t col) { col_fg_ = col_func_ = col_type_ = col_keyword_ = col; }
	void setTextHighlightColour(rgba_t col) { col_fg_hl = col; }
	void setFunctionColour(rgba_t col) { col_func_ = col; }
	void setTypeColour(rgba_t col) { col_type_ = col; }
	void setKeywordColour(rgba_t col) { col_keyword_ = col; }
	void setCurrentArg(int arg)
	{
		arg_current_ = arg;
		updateSize();
	}
	void enableArgSwitch(bool enable) { switch_contexts_ = enable; }
	void setFont(string face, int size);

	void openFunction(TLFunction* function, int arg = -1);
	void nextArgSet();
	void prevArgSet();

	static const int MAX_WIDTH = 800;

private:
	TLFunction*         function_;
	TLFunction::Context context_;

	rgba_t        col_bg_;
	rgba_t        col_fg_;
	rgba_t        col_fg_hl;
	rgba_t        col_func_;
	rgba_t        col_type_;
	rgba_t        col_keyword_;
	int           arg_current_;
	bool          switch_contexts_;
	unsigned long context_current_;
	wxRect        rect_btn_up_;
	wxRect        rect_btn_down_;
	int           btn_mouse_over_;
	wxBitmap      buffer_;
	wxFont        font_;

	void loadContext(unsigned long index);
	void updateSize();

	int    drawText(wxDC& dc, string text, int left, int top, wxRect* bounds);
	wxRect drawFunctionSpec(wxDC& dc, const TLFunction::Context& context, int left, int top);
	wxRect drawArgs(wxDC& dc, const TLFunction::Context& context, int left, int top, wxColour& col_faded, wxFont& bold);
	wxRect drawFunctionContext(
		wxDC&                      dc,
		const TLFunction::Context& context,
		int                        left,
		int                        top,
		wxColour&                  col_faded,
		wxFont&                    bold);
	wxRect drawFunctionDescription(wxDC& dc, string desc, int left, int top);
	wxSize drawCallTip(wxDC& dc, int xoff = 0, int yoff = 0);
	void   updateBuffer();

	// Events
	void onPaint(wxPaintEvent& e);
	void onEraseBackground(wxEraseEvent& e);
	void onMouseMove(wxMouseEvent& e);
	void onMouseDown(wxMouseEvent& e);
	void onShow(wxShowEvent& e);
};
