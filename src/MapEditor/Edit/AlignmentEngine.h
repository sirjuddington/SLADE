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

enum AlignTarget
{
	OverallAlignment,	// Perform overall alignment
	PerPartAlignment	// Perform alignment per wall part, if possible
};

class AlignmentEngine {
public:
	AlignmentEngine(
		MapEditContext *context,
		AlignType align_type,
		AlignTarget align_target = OverallAlignment);

	bool run(Item start_item);
private:
	struct AlignmentJob
	{
		MapSide* side;
		int      tex_offset_x;
	};

	MapEditContext*	context_ = nullptr;
	AlignType		align_type_;
	AlignTarget     align_target_;

	// Pending alignment jobs
	std::queue<AlignmentJob> jobs;

	// Set of sides already processed
	std::set<unsigned int> processed_sides;

	void alignX(const AlignmentJob& job, int tex_width);
	void alignY(const AlignmentJob& job, ItemType wall_type, int first_tex_top_height, int texture_height);

	// Helper functions
	void enqueueConnectedLines(
		const MapVertex*	common_vertex,
		int 				tex_offset_x);
	void enqueueSide(
		MapSide*            side,
		const MapVertex*    common_vertex,
		int                 tex_offset_x);
};
}; // namespace slade::mapeditor
