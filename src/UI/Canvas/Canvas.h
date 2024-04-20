#pragma once

namespace slade
{
struct MapPreviewData;

namespace ui
{
	wxWindow* createMapPreviewCanvas(
		wxWindow*       parent,
		MapPreviewData* data,
		bool            allow_zoom = false,
		bool            allow_pan  = false);
}
} // namespace slade
