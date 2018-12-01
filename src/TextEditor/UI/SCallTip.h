#pragma once

#include "TextEditor/TextLanguage.h"

class SCallTip : public wxPopupWindow
{
public:
	SCallTip(wxWindow* parent);
	~SCallTip();

	void setBackgroundColour(ColRGBA col) { col_bg_ = col; }
	void setTextColour(ColRGBA col) { col_fg_ = col_func_ = col_type_ = col_keyword_ = col; }
	void setTextHighlightColour(ColRGBA col) { col_fg_hl = col; }
	void setFunctionColour(ColRGBA col) { col_func_ = col; }
	void setTypeColour(ColRGBA col) { col_type_ = col; }
	void setKeywordColour(ColRGBA col) { col_keyword_ = col; }
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

	ColRGBA        col_bg_;
	ColRGBA        col_fg_;
	ColRGBA        col_fg_hl;
	ColRGBA        col_func_;
	ColRGBA        col_type_;
	ColRGBA        col_keyword_;
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
