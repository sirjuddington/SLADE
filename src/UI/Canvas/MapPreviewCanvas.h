#pragma once

#include "Canvas.h"

namespace slade
{
struct MapPreviewData;

class MapPreviewCanvas : public ui::Canvas
{
public:
	MapPreviewCanvas(wxWindow* parent, MapPreviewData* data);
	~MapPreviewCanvas() override = default;

	void updateBuffer();

private:
	MapPreviewData* data_ = nullptr;
	wxBitmap        buffer_;
	long            buffer_updated_time_ = 0;
	bool            buffer_things_       = false;

	sigslot::scoped_connection sc_ccfg_changed_;

	bool shouldUpdateBuffer() const;

	void onPaint(wxPaintEvent& e);
};
} // namespace slade
