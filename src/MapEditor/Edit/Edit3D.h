#pragma once

#include "MapEditor/MapEditor.h"

class MapEditContext;
class MapSide;
class UndoManager;

class Edit3D
{
public:
	explicit Edit3D(MapEditContext& context);

	UndoManager*	undoManager() const { return undo_manager.get(); }
	bool			lightLinked() const { return link_light; }
	bool			offsetsLinked() const { return link_offset; }
	bool			toggleLightLink() { link_light = !link_light; return link_light; }
	bool			toggleOffsetLink() { link_offset = !link_offset; return link_offset; }

	void	setLinked(bool light, bool offsets) { link_light = light; link_offset = offsets; }

	void	selectAdjacent(MapEditor::Item item) const;
	void	changeSectorLight(int amount) const;
	void	changeOffset(int amount, bool x) const;
	void	changeSectorHeight(int amount) const;
	void	autoAlignX(MapEditor::Item start) const;
	void	resetOffsets() const;
	void	toggleUnpegged(bool lower) const;
	//void	copy(int type);
	//void	paste(int type);
	//void	floodFill(int type);
	void	changeThingZ(int amount) const;
	void	deleteThing() const;
	void	changeScale(double amount, bool x) const;
	void	changeHeight(int amount) const;

	// TODO: Change back to private once floodFill is moved here
	vector<MapEditor::Item>	getAdjacent(MapEditor::Item item) const;

private:
	MapEditContext&					context;
	std::unique_ptr<UndoManager>	undo_manager;
	bool							link_light;
	bool							link_offset;

	// Helper for selectAdjacent
	static bool wallMatches(MapSide* side, MapEditor::ItemType part, string tex);
	void		getAdjacentWalls(MapEditor::Item item, vector<MapEditor::Item>& list) const;
	void		getAdjacentFlats(MapEditor::Item item, vector<MapEditor::Item>& list) const;

	// Helper for autoAlignX3d
	static void doAlignX(MapSide* side, int offset, string tex, vector<MapEditor::Item>& walls_done, int tex_width);
};
