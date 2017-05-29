
#ifndef __MAPLINE_H__
#define __MAPLINE_H__

#include "common.h"
#include "MapObject.h"

class MapVertex;
class MapSide;
class MapSector;

// Line texture ids
enum
{
	TEX_FRONT_MIDDLE	= 0x01,
	TEX_FRONT_UPPER		= 0x02,
	TEX_FRONT_LOWER		= 0x04,
	TEX_BACK_MIDDLE		= 0x08,
	TEX_BACK_UPPER		= 0x10,
	TEX_BACK_LOWER		= 0x20,
};

struct doomline_t
{
	uint16_t vertex1;
	uint16_t vertex2;
	uint16_t flags;
	uint16_t type;
	uint16_t sector_tag;
	uint16_t side1;
	uint16_t side2;
};

struct hexenline_t
{
	uint16_t	vertex1;
	uint16_t	vertex2;
	uint16_t	flags;
	uint8_t		type;
	uint8_t		args[5];
	uint16_t	side1;
	uint16_t	side2;
};

struct doom64line_t
{
	uint16_t vertex1;
	uint16_t vertex2;
	uint32_t flags;
	uint16_t type;
	uint16_t sector_tag;
	uint16_t side1;
	uint16_t side2;
};

class MapLine : public MapObject
{
	friend class SLADEMap;
private:
	// Basic data
	MapVertex*	vertex1;
	MapVertex*	vertex2;
	MapSide*	side1;
	MapSide*	side2;
	int			special;

	// Internally used info
	double		length;
	double		ca;	// Used for intersection calculations
	double		sa;	// ^^
	fpoint2_t	front_vec;

public:
	MapLine(SLADEMap* parent = nullptr);
	MapLine(MapVertex* v1, MapVertex* v2, MapSide* s1, MapSide* s2, SLADEMap* parent = nullptr);
	~MapLine();

	bool	isOk() const { return vertex1 && vertex2; }

	MapVertex*		v1() const { return vertex1; }
	MapVertex*		v2() const { return vertex2; }
	MapSide*		s1() const { return side1; }
	MapSide*		s2() const { return side2; }
	int				getSpecial() const { return special; }

	MapSector*	frontSector();
	MapSector*	backSector();

	double	x1();
	double	y1();
	double	x2();
	double	y2();

	int	v1Index();
	int	v2Index();
	int	s1Index();
	int	s2Index();

	bool	boolProperty(string key) override;
	int		intProperty(string key) override;
	double	floatProperty(string key) override;
	string	stringProperty(string key) override;
	void	setBoolProperty(string key, bool value) override;
	void	setIntProperty(string key, int value) override;
	void	setFloatProperty(string key, double value) override;
	void	setStringProperty(string key, string value) override;
	bool	scriptCanModifyProp(const string& key) override;

	void	setS1(MapSide* side);
	void	setS2(MapSide* side);

	fpoint2_t	getPoint(uint8_t point) override;
	fpoint2_t	point1();
	fpoint2_t	point2();
	fseg2_t		seg();
	double		getLength();
	bool		doubleSector();
	fpoint2_t	frontVector();
	fpoint2_t	dirTabPoint(double length = 0);
	double		distanceTo(fpoint2_t point);
	int			needsTexture();

	void	clearUnneededTextures();
	void	resetInternals();
	void	flip(bool sides = true);

	void	writeBackup(mobj_backup_t* backup) override;
	void	readBackup(mobj_backup_t* backup) override;
	void	copy(MapObject*) override;

	operator Debuggable() const {
		if (!this)
			return "<line NULL>";

		return Debuggable(S_FMT("<line %u>", index));
	}
};

#endif //__MAPLINE_H__
