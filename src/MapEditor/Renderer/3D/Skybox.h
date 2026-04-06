#pragma once

// Forward declarations
namespace slade
{
namespace gl
{
	class Camera;
	class Shader;
	class VertexBuffer3D;
} // namespace gl
} // namespace slade

namespace slade::mapeditor
{
class Skybox
{
public:
	Skybox();
	~Skybox();

	void setSkyTextures(string_view tex1, string_view tex2 = "");

	void render(const gl::Camera& camera);

private:
	unique_ptr<gl::VertexBuffer3D> vertex_buffer_;
	unique_ptr<gl::Shader>         shader_;
	unsigned                       vertex_index_caps_ = 0; // Vertex index where skybox cap quads start
	string                         skytex1_           = "SKY1";
	string                         skytex2_;
	unsigned                       sky_tex_id_ = 0;
	glm::vec4                      skycol_top_;
	glm::vec4                      skycol_bottom_;
	std::array<Vec2d, 32>          sky_circle_;

	void buildSkySlice(float top, float bottom, float alpha_top, float alpha_bottom, float size, float tx, float ty);
	void buildVertexBuffer();
};
} // namespace slade::mapeditor
