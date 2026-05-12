#pragma once

#include "MapEditor/Item.h"

// Forward declarations
namespace slade
{
struct Ray;
}
namespace slade::game
{
class ThingType;
}
namespace slade::gl
{
class VertexBuffer3D;
class LineBuffer;
class View;
class Camera;
class Shader;
} // namespace slade::gl
namespace slade::mapeditor
{
struct SelectionOverlay3D;
class SpriteBuffer3D;
class MapRenderer3D;
} // namespace slade::mapeditor

namespace slade::mapeditor
{
class ThingRenderer3D
{
public:
	ThingRenderer3D(MapRenderer3D* renderer);
	~ThingRenderer3D();

	void clear();

	void  updateVisibility(const gl::Camera& camera, float max_dist);
	bool  update(bool vis_check = false);
	float updateProgress() const;

	void renderSprites(const gl::Shader& shader, bool icons = false) const;
	void renderThingBoxes(const gl::Camera& camera, const gl::View& view, float max_dist = 0.0f) const;

	void renderHighlight(
		const Item&       item,
		const gl::Camera& camera,
		const gl::View&   view,
		const glm::vec4&  colour,
		bool              outline = false,
		bool              fill    = true);

	void addToSelectionOverlay(SelectionOverlay3D& overlay, const Item& item) const;

	optional<Item> nearestIntersectingThing(const gl::Camera& camera, const Ray& ray, double max_dist) const;

private:
	// TODO: Move these structs out of the class
	struct Thing
	{
		unsigned         index  = 0;
		float            z      = 0.0f;
		const MapSector* sector = nullptr;
	};
	struct ThingGroup
	{
		int                        type    = -1;
		unsigned                   texture = 0;
		Vec2f                      sprite_size;
		bool                       icon = false;
		const game::ThingType*     type_info;
		vector<Thing>              things;
		unique_ptr<SpriteBuffer3D> sprite_buffer;
		unique_ptr<gl::LineBuffer> line_buffer;

		ThingGroup() = default;
		ThingGroup(int type_id, const game::ThingType& type_info);

		void             addThing(const MapThing& thing);
		float            thingZ(unsigned index) const;
		const MapSector* thingSector(unsigned index) const;
		float            height() const;
	};

	MapRenderer3D* renderer_ = nullptr;
	vector<u8>     thing_visibility_;

	vector<ThingGroup> groups_;
	long               groups_updated_      = 0;
	int                things_processed_    = -1;
	bool               force_update_groups_ = false;

	Item                       prev_highlight_;
	unique_ptr<gl::LineBuffer> highlight_lines_;
	unique_ptr<SpriteBuffer3D> highlight_fill_;

	const ThingGroup* thingGroup(unsigned type) const;
	ThingGroup*       thingGroup(unsigned type);
};
} // namespace slade::mapeditor
