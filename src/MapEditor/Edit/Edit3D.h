#pragma once

class MapEditContext;
class MapSide;
class UndoManager;

class Edit3D
{
public:
	enum class SelectionType
	{
		WallTop,
		WallMiddle,
		WallBottom,
		Floor,
		Ceiling,
		Thing,
	};

	struct Selection
	{
		int				index;
		SelectionType	type;

		bool operator<(const Selection& other) const {
			return (this->type == other.type) ?
				this->index < other.index :
				this->type < other.type;
		}
	};

	explicit Edit3D(MapEditContext& context);

	UndoManager*	undoManager() const { return undo_manager.get(); }
	bool			lightLinked() const { return link_light; }
	bool			offsetsLinked() const { return link_offset; }
	bool			toggleLightLink() { link_light = !link_light; return link_light; }
	bool			toggleOffsetLink() { link_offset = !link_offset; return link_offset; }

	void	setLinked(bool light, bool offsets) { link_light = light; link_offset = offsets; }

	void	selectAdjacent(Selection item) const;
	void	changeSectorLight(int amount) const;
	void	changeOffset(int amount, bool x) const;
	void	changeSectorHeight(int amount) const;
	void	autoAlignX(Selection start) const;
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
	vector<Selection>	getAdjacent(Selection item) const;

private:
	MapEditContext&					context;
	std::unique_ptr<UndoManager>	undo_manager;
	bool							link_light;
	bool							link_offset;

	// Helper for selectAdjacent
	static bool wallMatches(MapSide* side, SelectionType part, string tex);
	void		getAdjacentWalls(Selection item, vector<Selection>& list) const;
	void		getAdjacentFlats(Selection item, vector<Selection>& list) const;

	// Helper for autoAlignX3d
	static void doAlignX(MapSide* side, int offset, string tex, vector<Selection>& walls_done, int tex_width);
};
