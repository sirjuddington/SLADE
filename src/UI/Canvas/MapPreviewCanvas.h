#pragma once

#include "Archive/Archive.h"
#include "OGLCanvas.h"

class GLTexture;

class MapPreviewCanvas : public OGLCanvas
{
public:
	MapPreviewCanvas(wxWindow* parent);
	~MapPreviewCanvas();

	void addVertex(double x, double y);
	void addLine(unsigned v1, unsigned v2, bool twosided, bool special, bool macro = false);
	void addThing(double x, double y);
	bool openMap(Archive::MapDesc map);
	bool readVertices(ArchiveEntry* map_head, ArchiveEntry* map_end, MapFormat map_format);
	bool readLines(ArchiveEntry* map_head, ArchiveEntry* map_end, MapFormat map_format);
	bool readThings(ArchiveEntry* map_head, ArchiveEntry* map_end, MapFormat map_format);
	void clearMap();
	void showMap();
	void draw();
	void createImage(ArchiveEntry& ae, int width, int height);

	unsigned nVertices();
	unsigned nSides();
	unsigned nLines();
	unsigned nSectors();
	unsigned nThings();
	unsigned width();
	unsigned height();

private:
	// Structs for basic map features
	struct Vertex
	{
		double x;
		double y;
		Vertex(double x, double y)
		{
			this->x = x;
			this->y = y;
		}
	};

	struct Line
	{
		unsigned v1;
		unsigned v2;
		bool     twosided;
		bool     special;
		bool     macro;
		bool     segment;
		Line(unsigned v1, unsigned v2)
		{
			this->v1 = v1;
			this->v2 = v2;
		}
	};

	struct Thing
	{
		double x;
		double y;
	};

	vector<Vertex> verts_;
	vector<Line>   lines_;
	vector<Thing>  things_;
	unsigned       n_sides_;
	unsigned       n_sectors_;
	double         zoom_;
	double         offset_x_;
	double         offset_y_;
	Archive*       temp_archive_;
	GLTexture*     tex_thing_;
	bool           tex_loaded_;
};
