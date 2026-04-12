#pragma once

#include "OpenGL/UniformBuffer.h"
#include "SelectionOverlay3D.h"

// Point lights are stored in a UBO with the following structure
inline constexpr int MAX_POINT_LIGHTS = 512; // Maximum point lights in the UBO
struct PointLightsUBOData
{
	struct alignas(16) Light
	{
		glm::vec4 position_type; // .xyz = world position, .w = type (cast to float)
		glm::vec4 colour_radius; // .xyz = RGB [0,1],      .w = radius
	};

	Light lights[MAX_POINT_LIGHTS];
};

// Forward declarations
namespace slade
{
class MCA3dSelection;
}
namespace slade::game
{
class ThingType;
}
namespace slade::gl
{
class Camera;
class IndexBuffer;
class LineBuffer;
class Shader;
class View;
struct Vertex3D;
} // namespace slade::gl
namespace slade::mapeditor
{
class FlatRenderer3D;
class MapEditContext;
class ItemSelection;
class Renderer;
class Skybox;
enum class RenderPass : u8;
struct Item;
class ThingRenderer3D;
class WallRenderer3D;
} // namespace slade::mapeditor


namespace slade::mapeditor
{
class MapRenderer3D
{
public:
	MapRenderer3D(MapEditContext* context, Renderer* renderer);
	~MapRenderer3D();

	SLADEMap* map() const { return map_; }
	Renderer* parentRenderer() const { return renderer_; }
	bool      fogEnabled() const { return fog_; }
	bool      fullbrightEnabled() const { return fullbright_; }

	gl::Shader* shader(bool alpha_test = false) const { return (alpha_test ? shader_3d_alphatest_ : shader_3d_).get(); }
	gl::Shader* spriteShader(bool icon = false) const { return (icon ? shader_3d_icon_ : shader_3d_sprite_).get(); }

	void enableHighlight(bool enable = true) { highlight_enabled_ = enable; }
	void enableSelection(bool enable = true) { selection_enabled_ = enable; }
	void enableFog(bool enable = true) { fog_ = enable; }
	void enableFullbright(bool enable = true) { fullbright_ = enable; }
	void setSkyTexture(string_view tex1, string_view tex2 = "") const;

	void render(const gl::Camera& camera, const gl::View& view);
	void renderHighlight(const Item& item, const gl::Camera& camera, const gl::View& view, float alpha = 1.0f);

	void updateSelection(const ItemSelection& selection);
	void populateSelectionOverlay(SelectionOverlay3D& overlay, const vector<Item>& items) const;
	void renderSelectionOverlay(
		const gl::Camera&         camera,
		const gl::View&           view,
		const SelectionOverlay3D& overlay,
		glm::vec4                 colour,
		float                     alpha = 1.0f) const;
	void renderSelection(const gl::Camera& camera, const gl::View& view);

	void clearData();

	Item findHighlightedItem(const gl::Camera& camera, const gl::View& view, const Vec2i& cursor_pos) const;

	// Testing
	unsigned flatsBufferSize() const;
	unsigned quadsBufferSize() const;

private:
	// References
	SLADEMap*       map_      = nullptr;
	MapEditContext* context_  = nullptr;
	Renderer*       renderer_ = nullptr;

	// Shaders
	unique_ptr<gl::Shader> shader_3d_;
	unique_ptr<gl::Shader> shader_3d_alphatest_;
	unique_ptr<gl::Shader> shader_3d_sprite_;
	unique_ptr<gl::Shader> shader_3d_icon_;

	// Point lights UBO
	gl::UniformBuffer<PointLightsUBOData> point_lights_ubo_;

	// Render handlers
	unique_ptr<Skybox>          skybox_;
	unique_ptr<ThingRenderer3D> thing_renderer_;
	unique_ptr<WallRenderer3D>  wall_renderer_;
	unique_ptr<FlatRenderer3D>  flat_renderer_;

	// Render options
	bool fullbright_ = false;
	bool fog_        = true;

	// Highlighted/selected items
	bool                        highlight_enabled_ = true;
	bool                        selection_enabled_ = true;
	unique_ptr<gl::LineBuffer>  highlight_lines_;
	unique_ptr<gl::IndexBuffer> highlight_fill_;
	SelectionOverlay3D          selection_overlay_;
	long                        selection_overlay_updated_ = 0;

	void renderSkyFlatsQuads() const;
};
} // namespace slade::mapeditor
