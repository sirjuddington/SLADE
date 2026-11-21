#pragma once

#include <queue>

// Forward declarations
namespace slade {
	namespace mapeditor {
		class MapEditContext;
		class Item;
		enum class ItemType;
	}; // namespace mapeditor
	class MapSide;
}; // namespace slade

namespace slade::mapeditor {
enum class AlignType
{
	AlignX  = 1,
	AlignY  = 2,
	AlignXY = AlignX | AlignY
};

class AlignmentEngine {
public:
	AlignmentEngine(
		MapEditContext *context,
		AlignType align_type);

	bool run(Item start_item);
private:
	struct AlignmentJob
	{
		MapSide* side;
		int      tex_offset_x;
	};

	MapEditContext*	context_ = nullptr;
	AlignType		align_type_;

	// Pending alignment jobs
	std::queue<AlignmentJob> jobs;

	// Set of sides already processed
	std::set<unsigned int> processed_sides;

	// Helper functions
	int  getTextureTopHeight(
		const MapLine* first_line,
		ItemType wall_type,
		int tex_height) const;
	void enqueueConnectedLines(
		const MapVertex* common_vertex,
		int tex_offset_x);
	void enqueueSide(
		MapSide*                  side,
		const MapVertex*          common_vertex,
		int                       tex_offset_x);
};
}; // namespace slade::mapeditor
