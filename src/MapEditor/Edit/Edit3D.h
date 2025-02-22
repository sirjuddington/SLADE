#pragma once

#include "MapEditor/MapEditor.h"
#include "SLADEMap/MapObject/MapThing.h"

namespace slade
{
class MapEditContext;
class MapSide;
class UndoManager;

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
		AlignX = 1,
		AlignY = 2,
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

	void selectAdjacent(mapeditor::Item item) const;
	void changeSectorLight(int amount) const;
	void changeOffset(int amount, bool x) const;
	void changeSectorHeight(int amount) const;
	void autoAlign(mapeditor::Item start, AlignType alignType = AlignType::AlignX) const;
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
	MapEditContext&         context_;
	unique_ptr<UndoManager> undo_manager_;
	bool                    link_light_;
	bool                    link_offset_;
	string                  copy_texture_;
	MapThing                copy_thing_;

	vector<mapeditor::Item> getAdjacent(mapeditor::Item item) const;

	// Helper for selectAdjacent
	static bool wallMatches(MapSide* side, mapeditor::ItemType part, string_view tex);
	void        getAdjacentWalls(mapeditor::Item item, vector<mapeditor::Item>& list) const;
	void        getAdjacentFlats(mapeditor::Item item, vector<mapeditor::Item>& list) const;

	// Helper for texture auto-alighment
	static void doAlign(
		MapSide*                 side,
		AlignType                alignType,
		int                      offsetX,
		int                      offsetY,
		string_view              tex,
		vector<mapeditor::Item>& walls_done,
		int                      tex_width);
};
} // namespace slade
