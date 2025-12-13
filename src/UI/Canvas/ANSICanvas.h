#pragma once

#include "General/Sigslot.h"

namespace slade::gfx
{
class ANSIScreen;
}

namespace slade::ui
{
class ANSICanvas : public wxPanel
{
public:
	ANSICanvas(wxWindow* parent);
	~ANSICanvas() override = default;

	void openScreen(gfx::ANSIScreen* screen);

	int  scale() const { return scale_; }
	void setScale(u8 scale);

	void drawCharacter(unsigned index);

	std::optional<unsigned> hitTest(const wxPoint& pt) const;

private:
	gfx::ANSIScreen*     ansi_screen_ = nullptr;
	unsigned             width_       = 0;
	unsigned             height_      = 0;
	vector<u8>           picdata_;
	const u8*            fontdata_ = nullptr;
	unique_ptr<wxBitmap> image_;
	u8                   char_width_  = 8;
	u8                   char_height_ = 8;
	u8                   scale_       = 1;

	ScopedConnectionList sig_connections_;

	void onPaint(wxPaintEvent& e);
};
} // namespace slade::ui
