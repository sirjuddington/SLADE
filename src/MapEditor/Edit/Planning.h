#pragma once

#include "MapEditor/MapEditor.h"
#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapVertex.h"

class MapEditContext;

namespace MapEditor
{

class PlanNote : public MapObject
{
public:
	PlanNote() : MapObject(Type::PlanNote) {}
	PlanNote(double x, double y, string text = "") : MapObject(Type::PlanNote), position_{ x, y }, text_{ text } {}

	fpoint2_t		pos() const { return position_; }
	const string&	text() const { return text_; }
	const string&	detail() const { return detail_; }
	rgba_t			colour() const { return colour_; }
	const string&	icon() const { return icon_; }
	fpoint2_t		target() const { return target_; }

	fpoint2_t	point(Point point) override;

	void	setText(const string& text) { text_ = text; }
	void	setDetail(const string& detail) { detail_ = detail; }
	void	setColour(rgba_t colour) { colour_ = colour; }
	void	setIcon(const string& icon) { icon_ = icon; }
	void	move(fpoint2_t offset) { position_ = position_ + offset; }

	void	writeBackup(Backup* backup) override {}
	void	readBackup(Backup* backup) override {}

	typedef std::unique_ptr<PlanNote> UPtr;

private:
	fpoint2_t	position_;
	string		text_;
	string		detail_;
	rgba_t		colour_ = { 180, 180, 180, 255 };
	string		icon_;
	fpoint2_t	target_;
};
	
class Planning
{
public:
	Planning(MapEditContext& context);

	const vector<MapVertex::UPtr>&	vertices() const { return vertices_; }
	const vector<MapLine::UPtr>&	lines() const { return lines_; }
	const vector<PlanNote::UPtr>&	notes() const { return notes_; }

	MapLine*	line(unsigned index) const { return index < lines_.size() ? lines_[index].get() : nullptr; }
	MapVertex*	vertex(unsigned index) const { return index < vertices_.size() ? vertices_[index].get() : nullptr; }
	PlanNote*	note(unsigned index) const { return index < notes_.size() ? notes_[index].get() : nullptr; }
	MapObject*	object(ItemType type, unsigned index) const;

	void		createLines(vector<fpoint2_t>& points);
	MapVertex*	createVertex(double x, double y);
	PlanNote*	createNote(double x, double y);

	bool	deleteVertex(MapVertex* vertex);
	bool	deleteLine(MapLine* line);

	MapObject*	nearestObject(fpoint2_t point, double min);
	void		updateIndices();
	void		deleteDetachedVertices();
	void		clearFilter();

private:
	MapEditContext&			context_;
	vector<MapVertex::UPtr>	vertices_;
	vector<MapLine::UPtr>	lines_;
	vector<PlanNote::UPtr>	notes_;
};

}
