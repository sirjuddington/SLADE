#pragma once

namespace slade::gl
{
class LineBuffer;
class IndexBuffer;
class VertexBuffer3D;
} // namespace slade::gl

namespace slade::mapeditor
{
// Used for rendering selection and select/deselect animations in 3D mode
struct SelectionOverlay3D
{
	unique_ptr<gl::LineBuffer>     outline;
	unique_ptr<gl::IndexBuffer>    fill_flats;
	unique_ptr<gl::IndexBuffer>    fill_quads;
	unique_ptr<gl::VertexBuffer3D> fill_things;
};
} // namespace slade::mapeditor
