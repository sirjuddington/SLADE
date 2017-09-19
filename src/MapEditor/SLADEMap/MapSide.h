#pragma once

#include "MapObject.h"

class MapSector;
class MapLine;

class MapSide : public MapObject
{
	friend class SLADEMap;
	friend class MapLine;
public:
	struct DoomData
	{
		short	x_offset;
		short	y_offset;
		char	tex_upper[8];
		char	tex_lower[8];
		char	tex_middle[8];
		short	sector;
	};

	struct Doom64Data
	{
		short		x_offset;
		short		y_offset;
		uint16_t	tex_upper;
		uint16_t	tex_lower;
		uint16_t	tex_middle;
		short		sector;
	};

	MapSide(MapSector* sector = nullptr, SLADEMap* parent = nullptr);
	MapSide(SLADEMap* parent);
	~MapSide();

	void	copy(MapObject* c) override;

	bool	isOk() const { return !!sector_; }

	MapSector*	sector() const { return sector_; }
	MapLine*	parentLine() const { return parent_line_; }
	string		texUpper() const { return tex_upper_; }
	string		texMiddle() const { return tex_middle_; }
	string		texLower() const { return tex_lower_; }
	int			offsetX() const { return offset_.x; }
	int			offsetY() const { return offset_.y; }
	point2_t	offset() const { return offset_; }
	uint8_t		light();

	void	setSector(MapSector* sector);
	void	changeLight(int amount);

	int		intProperty(const string& key) override;
	void	setIntProperty(const string& key, int value) override;
	string	stringProperty(const string& key) override;
	void	setStringProperty(const string& key, const string& value) override;
	bool	scriptCanModifyProp(const string& key) override;

	void	writeBackup(Backup* backup) override;
	void	readBackup(Backup* backup) override;

private:
	// Basic data
	MapSector*	sector_			= nullptr;
	MapLine*	parent_line_	= nullptr;
	string		tex_upper_;
	string		tex_middle_;
	string		tex_lower_;
	point2_t	offset_;
};
