#pragma once

namespace slade
{
struct MapPreviewData;

class MapPreviewCanvas : public wxPanel
{
public:
	MapPreviewCanvas(wxWindow* parent, MapPreviewData* data);
	~MapPreviewCanvas() override = default;

	void updateBuffer();

private:
	MapPreviewData* data_ = nullptr;
	wxBitmap        buffer_;
	long            buffer_updated_time = 0;
	bool            buffer_things       = false;

	bool shouldUpdateBuffer() const;

	void onPaint(wxPaintEvent& e);
};
} // namespace slade
