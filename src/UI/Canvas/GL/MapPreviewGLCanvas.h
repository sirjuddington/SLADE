#pragma once

#include "GLCanvas.h"

namespace slade
{
struct MapPreviewData;
namespace gl
{
	class PointSpriteBuffer;
	class LineBuffer;
} // namespace gl

class MapPreviewGLCanvas : public GLCanvas
{
public:
	MapPreviewGLCanvas(wxWindow* parent, MapPreviewData* data, bool allow_zoom = false, bool allow_pan = false);
	~MapPreviewGLCanvas() override = default;

	void draw() override;

private:
	MapPreviewData*                   data_      = nullptr;
	bool                              view_init_ = false;
	unique_ptr<gl::LineBuffer>        lines_buffer_;
	unique_ptr<gl::PointSpriteBuffer> things_buffer_;
	long                              buffer_updated_time_ = 0;

	void updateLinesBuffer();
	void updateThingsBuffer();
};
} // namespace slade
