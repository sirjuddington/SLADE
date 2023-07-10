#pragma once

#include "Archive/Archive.h"
#include "GLCanvas.h"
#include "OpenGL/VertexBuffer2D.h"

namespace slade
{
class GLTexture;

class MapPreviewCanvas : public GLCanvas
{
public:
	MapPreviewCanvas(wxWindow* parent, bool allow_zoom = false, bool allow_pan = false);
	~MapPreviewCanvas() override = default;

	void addVertex(double x, double y);
	void addLine(unsigned v1, unsigned v2, bool twosided, bool special, bool macro = false);
	void addThing(double x, double y);
	bool openMap(Archive::MapDesc map);
	bool readVertices(ArchiveEntry* map_head, const ArchiveEntry* map_end, MapFormat map_format);
	bool readLines(ArchiveEntry* map_head, const ArchiveEntry* map_end, MapFormat map_format);
	bool readThings(ArchiveEntry* map_head, const ArchiveEntry* map_end, MapFormat map_format);
	void clearMap();
	void showMap();
	void draw() override;
	void createImage(ArchiveEntry& ae, int width, int height) const;

	unsigned nVertices() const;
	unsigned nSides() const { return n_sides_; }
	unsigned nLines() const { return lines_.size(); }
	unsigned nSectors() const { return n_sectors_; }
	unsigned nThings() const { return things_.size(); }
	unsigned width() const;
	unsigned height() const;

private:
	// Structs for basic map features
	struct Vertex
	{
		double x;
		double y;
		Vertex(double x, double y) : x{ x }, y{ y } {}
	};

	struct Line
	{
		unsigned v1       = 0;
		unsigned v2       = 0;
		bool     twosided = false;
		bool     special  = false;
		bool     macro    = false;
		bool     segment  = false;

		Line(
			unsigned v1,
			unsigned v2,
			bool     twosided = false,
			bool     special  = false,
			bool     macro    = false,
			bool     segment  = false) :
			v1{ v1 }, v2{ v2 }, twosided{ twosided }, special{ special }, macro{ macro }, segment{ segment }
		{
		}
	};

	struct Thing
	{
		double x;
		double y;
		Thing(double x, double y) : x{ x }, y{ y } {}
	};

	vector<Vertex>      verts_;
	vector<Line>        lines_;
	vector<Thing>       things_;
	unsigned            n_sides_   = 0;
	unsigned            n_sectors_ = 0;
	unique_ptr<Archive> temp_archive_;
	unsigned            tex_thing_ = 0;
	bool                panning_   = false;
	bool                view_init_ = false;

	unique_ptr<gl::VertexBuffer2D> vb_lines_;
	unique_ptr<gl::VertexBuffer2D> vb_things_;

	void updateLinesBuffer();
	void updateThingsBuffer();
};
} // namespace slade
