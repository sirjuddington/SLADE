#pragma once

// Forward declarations
namespace slade
{
struct MapPreviewData;
class GfxCanvasBase;
} // namespace slade

namespace slade::ui
{
class Canvas : public wxControl
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
} // namespace slade::ui
