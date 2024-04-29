#pragma once

namespace slade
{
class CTextureCanvasBase;
class GfxCanvasBase;
struct MapPreviewData;

namespace ui
{
	wxWindow* createMapPreviewCanvas(
		wxWindow*       parent,
		MapPreviewData* data,
		bool            allow_zoom = false,
		bool            allow_pan  = false);

	GfxCanvasBase* createGfxCanvas(wxWindow* parent);

	CTextureCanvasBase* createCTextureCanvas(wxWindow* parent);
} // namespace ui
} // namespace slade
