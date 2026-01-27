#pragma once

// Forward declarations
namespace slade
{
namespace gl
{
	class IndexBuffer;
	struct Vertex3D;
	class Camera;
	class View;
	class Shader;
} // namespace gl
namespace mapeditor
{
	struct Flat3D;
	struct Quad3D;
	class Skybox;
	class MapGeometryBuffer3D;
} // namespace mapeditor
} // namespace slade


namespace slade
{
class MapRenderer3D
{
public:
	MapRenderer3D(SLADEMap* map);
	~MapRenderer3D();

	bool fogEnabled() const;
	bool fullbrightEnabled() const { return fullbright_; }

	void enableHilight(bool enable = true);
	void enableSelection(bool enable = true);
	void enableFog(bool enable = true);
	void enableFullbright(bool enable = true) { fullbright_ = enable; }
	void setSkyTexture(string_view tex1, string_view tex2 = "") const;

	void render(const gl::Camera& camera);

	void clearData();


	// Testing
	unsigned flatsBufferSize() const;
	unsigned quadsBufferSize() const;

private:
	SLADEMap*                     map_ = nullptr;
	unique_ptr<gl::Shader>        shader_3d_;
	unique_ptr<gl::Shader>        shader_3d_alphatest_;
	unique_ptr<mapeditor::Skybox> skybox_;
	bool                          fullbright_ = false;

	vector<mapeditor::Flat3D>                  flats_;
	unique_ptr<mapeditor::MapGeometryBuffer3D> vb_flats_; // Vertex buffer for all flats
	long                                       flats_updated_ = 0;

	vector<mapeditor::Quad3D>                  quads_;
	unique_ptr<mapeditor::MapGeometryBuffer3D> vb_quads_; // Vertex buffer for all wall quads
	long                                       quads_updated_ = 0;

	// Used to organise quads/flats into groups for rendering
	struct RenderGroup
	{
		enum class Transparency : u8
		{
			None     = 0,
			Normal   = 1,
			Additive = 2
		};

		unsigned                    texture;
		glm::vec4                   colour;
		unique_ptr<gl::IndexBuffer> index_buffer;
		bool                        alpha_test  = false;
		bool                        sky         = false;
		Transparency                transparent = Transparency::None;
	};
	vector<RenderGroup> quad_groups_;
	vector<RenderGroup> flat_groups_;

	void updateFlats();
	void updateWalls();

	void renderSkyFlatsQuads() const;
	void renderFlats(gl::Shader& shader, int pass = 0) const;
	void renderWalls(gl::Shader& shader, int pass = 0) const;
};
} // namespace slade
