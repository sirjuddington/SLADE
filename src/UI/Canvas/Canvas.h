#pragma once

namespace slade
{
class CTextureCanvasBase;
class GfxCanvasBase;
struct MapPreviewData;

namespace ui
{
	class Canvas : public wxPanel
	{
	public:
		Canvas(wxWindow* parent);
		~Canvas() override = default;

		static double scaleFactor();

	private:
		double GetContentScaleFactor() const override;
	};

	wxWindow* createMapPreviewCanvas(
		wxWindow*       parent,
		MapPreviewData* data,
		bool            allow_zoom = false,
		bool            allow_pan  = false);

	GfxCanvasBase* createGfxCanvas(wxWindow* parent);

	CTextureCanvasBase* createCTextureCanvas(wxWindow* parent);
} // namespace ui
} // namespace slade
