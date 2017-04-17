#pragma once

#include "MapEditor/SLADEMap/MapThing.h"
#include "MapEditor/MapEditor.h"

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

	explicit Edit3D(MapEditContext& context);

	UndoManager*	undoManager() const { return undo_manager_.get(); }
	bool			lightLinked() const { return link_light_; }
	bool			offsetsLinked() const { return link_offset_; }
	bool			toggleLightLink() { link_light_ = !link_light_; return link_light_; }
	bool			toggleOffsetLink() { link_offset_ = !link_offset_; return link_offset_; }

	void	setLinked(bool light, bool offsets) { link_light_ = light; link_offset_ = offsets; }

	void	selectAdjacent(MapEditor::Item item) const;
	void	changeSectorLight(int amount) const;
	void	changeOffset(int amount, bool x) const;
	void	changeSectorHeight(int amount) const;
	void	autoAlignX(MapEditor::Item start) const;
	void	resetOffsets() const;
	void	toggleUnpegged(bool lower) const;
	void	copy(CopyType type);
	void	paste(CopyType type);
	void	floodFill(CopyType type);
	void	changeThingZ(int amount) const;
	void	deleteThing() const;
	void	changeScale(double amount, bool x) const;
	void	changeHeight(int amount) const;
	void 	changeTexture() const;

private:
	MapEditContext&					context_;
	std::unique_ptr<UndoManager>	undo_manager_;
	bool							link_light_;
	bool							link_offset_;
	string							copy_texture_;
	MapThing						copy_thing_;

	vector<MapEditor::Item>	getAdjacent(MapEditor::Item item) const;

	// Helper for selectAdjacent
	static bool wallMatches(MapSide* side, MapEditor::ItemType part, string tex);
	void		getAdjacentWalls(MapEditor::Item item, vector<MapEditor::Item>& list) const;
	void		getAdjacentFlats(MapEditor::Item item, vector<MapEditor::Item>& list) const;

	// Helper for autoAlignX3d
	static void doAlignX(MapSide* side, int offset, string tex, vector<MapEditor::Item>& walls_done, int tex_width);
};
