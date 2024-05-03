#pragma once

#include "TextEditor/TextLanguage.h"

namespace slade
{
class SCallTip : public wxPopupWindow
{
public:
	SCallTip(wxWindow* parent);
	~SCallTip() override = default;

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
	void setFont(const wxString& face, int size);

	void openFunction(TLFunction* function, int arg = -1);
	void nextArgSet();
	void prevArgSet();

	static constexpr int MAX_WIDTH = 800;

private:
	TLFunction*         function_ = nullptr;
	TLFunction::Context context_;

	ColRGBA       col_bg_ = { 240, 240, 240 };
	ColRGBA       col_fg_ = { 240, 240, 240 };
	ColRGBA       col_fg_hl;
	ColRGBA       col_func_;
	ColRGBA       col_type_;
	ColRGBA       col_keyword_;
	int           arg_current_     = -1;
	bool          switch_contexts_ = false;
	unsigned long context_current_ = 0;
	wxRect        rect_btn_up_;
	wxRect        rect_btn_down_;
	int           btn_mouse_over_ = 0;
	wxBitmap      scratch_, buffer_;
	wxFont        font_;

	void loadContext(unsigned long index);
	void updateSize();

	int    drawText(wxDC& dc, const wxString& text, int left, int top, wxRect* bounds) const;
	wxRect drawFunctionSpec(wxDC& dc, const TLFunction::Context& context, int left, int top) const;
	wxRect drawArgs(
		wxDC&                      dc,
		const TLFunction::Context& context,
		int                        left,
		int                        top,
		const wxColour&            col_faded,
		const wxFont&              bold) const;
	wxRect drawFunctionContext(
		wxDC&                      dc,
		const TLFunction::Context& context,
		int                        left,
		int                        top,
		const wxColour&            col_faded,
		const wxFont&              bold) const;
	wxRect drawFunctionDescription(wxDC& dc, const wxString& desc, int left, int top) const;
	wxSize drawCallTip(wxDC& dc, int xoff = 0, int yoff = 0);
	void   updateBuffer();

	// Events
	void onPaint(wxPaintEvent& e);
	void onEraseBackground(wxEraseEvent& e);
	void onMouseMove(wxMouseEvent& e);
	void onMouseDown(wxMouseEvent& e);
	void onShow(wxShowEvent& e);
};
} // namespace slade
