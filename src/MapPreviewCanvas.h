
#ifndef __MAP_PREVIEW_CANVAS_H__
#define __MAP_PREVIEW_CANVAS_H__

#include "OGLCanvas.h"
#include "Archive.h"

// Structs for basic map features
struct mep_vertex_t
{
	double x;
	double y;
	mep_vertex_t(double x, double y) { this->x = x; this->y = y; }
};

struct mep_line_t
{
	unsigned	v1;
	unsigned	v2;
	bool		twosided;
	bool		special;
	bool		macro;
	bool		segment;
	mep_line_t(unsigned v1, unsigned v2) { this->v1 = v1; this->v2 = v2; }
};

struct mep_thing_t
{
	double	x;
	double	y;
};

class GLTexture;
class MapPreviewCanvas : public OGLCanvas
{
private:
	vector<mep_vertex_t>	verts;
	vector<mep_line_t>		lines;
	vector<mep_thing_t>		things;
	unsigned				n_sides;
	unsigned				n_sectors;
	double					zoom;
	double					offset_x;
	double					offset_y;
	Archive*				temp_archive;
	GLTexture*				tex_thing;
	bool					tex_loaded;

public:
	MapPreviewCanvas(wxWindow* parent);
	~MapPreviewCanvas();

	void addVertex(double x, double y);
	void addLine(unsigned v1, unsigned v2, bool twosided, bool special, bool macro = false);
	void addThing(double x, double y);
	bool openMap(Archive::mapdesc_t map);
	bool readVertices(ArchiveEntry* map_head, ArchiveEntry* map_end, int map_format);
	bool readLines(ArchiveEntry* map_head, ArchiveEntry* map_end, int map_format);
	bool readThings(ArchiveEntry* map_head, ArchiveEntry* map_end, int map_format);
	void clearMap();
	void showMap();
	void draw();
	void createImage(ArchiveEntry& ae, int width, int height);

	unsigned	nVertices();
	unsigned	nSides();
	unsigned	nLines();
	unsigned	nSectors();
	unsigned	nThings();
	unsigned	getWidth();
	unsigned	getHeight();
};

#endif//__MAP_PREVIEW_CANVAS_H__
