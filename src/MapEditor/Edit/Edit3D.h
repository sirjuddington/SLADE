#pragma once

// Forward declarations
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
	void autoAlignX(Item start) const;
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

	// Helper for autoAlignX
	static void doAlignX(MapSide* side, int offset, string_view tex, vector<Item>& walls_done, int tex_width);
};
} // namespace slade::mapeditor
