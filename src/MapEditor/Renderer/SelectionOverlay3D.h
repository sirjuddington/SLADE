#pragma once

namespace slade::gl
{
class LineBuffer;
class IndexBuffer;
} // namespace slade::gl

namespace slade::mapeditor
{
// Used for rendering selection and select/deselect animations in 3D mode
struct SelectionOverlay3D
{
	unique_ptr<gl::LineBuffer>  outline;
	unique_ptr<gl::IndexBuffer> fill_flats;
	unique_ptr<gl::IndexBuffer> fill_quads;
};
} // namespace slade::mapeditor
