#pragma once

#include <queue>

// Forward Declarations
namespace slade
{
class UndoManager;

namespace mapeditor
{
	enum class ItemType;
	struct Item;
	class MapEditContext;
} // namespace mapeditor
} // namespace slade

namespace slade::mapeditor
{
class Edit3D
{
public:
	enum class CopyType
	{
		TexType,
		Offsets,
		Scale
	};

	enum class AlignType
	{
		AlignX  = 1,
		AlignY  = 2,
		AlignXY = AlignX | AlignY
	};

	explicit Edit3D(MapEditContext& context);

	UndoManager* undoManager() const { return undo_manager_.get(); }
	bool         lightLinked() const { return link_light_; }
	bool         offsetsLinked() const { return link_offset_; }
	bool         toggleLightLink()
	{
		link_light_ = !link_light_;
		return link_light_;
	}
	bool toggleOffsetLink()
	{
		link_offset_ = !link_offset_;
		return link_offset_;
	}

	void setLinked(bool light, bool offsets)
	{
		link_light_  = light;
		link_offset_ = offsets;
	}

	void selectAdjacent(Item item) const;
	void changeSectorLight(int amount) const;
	void changeOffset(int amount, bool x) const;
	void changeSectorHeight(int amount) const;
	void autoAlign(Item start, AlignType alignType = AlignType::AlignX) const;
	void resetOffsets() const;
	void toggleUnpegged(bool lower) const;
	void copy(CopyType type);
	void paste(CopyType type) const;
	void floodFill(CopyType type) const;
	void changeThingZ(int amount) const;
	void deleteThing() const;
	void changeScale(double amount, bool x) const;
	void changeHeight(int amount) const;
	void changeTexture() const;
	void deleteTexture() const;

private:
	MapEditContext*         context_ = nullptr;
	unique_ptr<UndoManager> undo_manager_;
	bool                    link_light_  = false;
	bool                    link_offset_ = false;
	string                  copy_texture_;
	unique_ptr<MapThing>    copy_thing_;

	vector<Item> getAdjacent(Item item) const;

	// Helper for selectAdjacent
	static bool wallMatches(const MapSide* side, ItemType part, string_view tex);
	void        getAdjacentWalls(Item item, vector<Item>& list) const;
	void        getAdjacentFlats(Item item, vector<Item>& list) const;

	// Helper type for texture auto-alignment
	struct AlignmentJob
	{
		MapSide* side;
		int      tex_offset_x;
	};

	// Helper functions for texture auto-alignment
	int         getTextureTopHeight(MapLine* firstLine, mapeditor::ItemType wall_type, int tex_height) const;
	static void enqueueConnectedLines(std::queue<AlignmentJob>& jobs, MapVertex* common_vertex, int tex_offset_x);
	static void enqueueSide(std::queue<AlignmentJob>& jobs, MapSide* side, MapVertex* common_vertex, int tex_offset_x);
};
} // namespace slade::mapeditor
