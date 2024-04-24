#pragma once

#include "Geometry/BBox.h"

namespace slade
{
struct MapDesc;

struct MapPreviewData
{
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
			v1{ v1 },
			v2{ v2 },
			twosided{ twosided },
			special{ special },
			macro{ macro },
			segment{ segment }
		{
		}
	};
	struct Thing
	{
		double x;
		double y;
		Thing(double x, double y) : x{ x }, y{ y } {}
	};

	vector<Vertex> vertices;
	vector<Line>   lines;
	vector<Thing>  things;
	unsigned       n_sides      = 0;
	unsigned       n_sectors    = 0;
	long           updated_time = 0;
	BBox           bounds;

	bool openMap(MapDesc map);
	void clear();
};

bool createMapImage(const MapPreviewData& data, const string& filename, int width, int height);
} // namespace slade
