#pragma once

// Forward declarations
namespace slade
{
namespace gl
{
	struct Vertex3D;
	class Camera;
	class View;
	class VertexBuffer3D;
	class Shader;
} // namespace gl
namespace mapeditor
{
	struct Flat3D;
}
} // namespace slade


namespace slade
{
class MapRenderer3D
{
public:
	MapRenderer3D(SLADEMap* map);
	~MapRenderer3D();

	bool fogEnabled() const;
	bool fullbrightEnabled() const;

	void enableHilight(bool enable = true);
	void enableSelection(bool enable = true);
	void enableFog(bool enable = true);
	void enableFullbright(bool enable = true);

	void render(const gl::Camera& camera);

	void clearData();


	// Testing
	unsigned flatsBufferSize() const;

private:
	SLADEMap*              map_ = nullptr;
	unique_ptr<gl::Shader> shader_3d_;

	vector<mapeditor::Flat3D>      flats_;
	unique_ptr<gl::VertexBuffer3D> vb_flats_; // Vertex buffer for all flats
	long                           flats_updated_ = 0;


	void renderFlats();
};
} // namespace slade
