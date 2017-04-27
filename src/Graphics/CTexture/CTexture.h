
#ifndef __CTEXTURE_H__
#define __CTEXTURE_H__

#include "Utility/Tokenizer.h"
#include "Archive/ArchiveEntry.h"
#include "General/ListenerAnnouncer.h"
#include "Graphics/Translation.h"

class SImage;

// Basic patch
class CTPatch
{
protected:
	string			name;
	int16_t			offset_x;
	int16_t			offset_y;

public:
	CTPatch();
	CTPatch(string name, int16_t offset_x = 0, int16_t offset_y = 0);
	CTPatch(CTPatch* copy);
	virtual ~CTPatch();

	string			getName() { return name; }
	int16_t			xOffset() { return offset_x; }
	int16_t			yOffset() { return offset_y; }

	void	setName(string name) { this->name = name; }
	void	setOffsetX(int16_t offset) { offset_x = offset; }
	void	setOffsetY(int16_t offset) { offset_y = offset; }

	virtual ArchiveEntry*	getPatchEntry(Archive* parent = NULL);
};

// Extended patch (for TEXTURES)
#define	PTYPE_PATCH		0
#define PTYPE_GRAPHIC	1
class CTPatchEx : public CTPatch
{
private:
	uint8_t			type;			// 0=patch, 1=graphic
	bool			flip_x;
	bool			flip_y;
	bool			use_offsets;
	int16_t			rotation;
	Translation		translation;
	rgba_t			colour;
	float			alpha;
	string			style;
	uint8_t			blendtype;		// 0=none, 1=translation, 2=blend, 3=tint

public:
	CTPatchEx();
	CTPatchEx(string name, int16_t offset_x = 0, int16_t offset_y = 0, uint8_t type = 0);
	CTPatchEx(CTPatch* copy);
	CTPatchEx(CTPatchEx* copy);
	~CTPatchEx();

	bool			flipX() { return flip_x; }
	bool			flipY() { return flip_y; }
	bool			useOffsets() { return use_offsets; }
	int16_t			getRotation() { return rotation; }
	rgba_t			getColour() { return colour; }
	float			getAlpha() { return alpha; }
	string			getStyle() { return style; }
	uint8_t			getBlendType() { return blendtype; }
	Translation&	getTranslation() { return translation; }

	void	flipX(bool flip) { flip_x = flip; }
	void	flipY(bool flip) { flip_y = flip; }
	void	useOffsets(bool use) { use_offsets = use; }
	void	setRotation(int16_t rot) { rotation = rot; }
	void	setColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { colour.set(r, g, b, a); }
	void	setAlpha(float a) { alpha = a; }
	void	setStyle(string s) { style = s; }
	void	setBlendType(uint8_t type) { blendtype = type; }

	ArchiveEntry*	getPatchEntry(Archive* parent = NULL);

	bool	parse(Tokenizer& tz, uint8_t type = 0);
	string	asText();
};

class TextureXList;
class SImage;
class Palette8bit;

#define TEXTYPE_TEXTURE		0
#define TEXTYPE_SPRITE		1
#define TEXTYPE_GRAPHIC		2
#define TEXTYPE_WALLTEXTURE	3
#define TEXTYPE_FLAT		4
#define TEXTYPE_HIRES		5

class CTexture : public Announcer
{
	friend class TextureXList;
private:
	// Basic info
	string				name;
	uint16_t			width;
	uint16_t			height;
	double				scale_x;
	double				scale_y;
	bool				world_panning;
	vector<CTPatch*>	patches;
	int					index;

	// Extended (TEXTURES) info
	string	type;
	bool	extended;
	bool	defined;
	bool	optional;
	bool	no_decals;
	bool	null_texture;
	int16_t	offset_x;
	int16_t	offset_y;
	int16_t	def_width;
	int16_t	def_height;

	// Editor info
	uint8_t			state;
	TextureXList*	in_list;

public:
	CTexture(bool extended = false);
	~CTexture();

	void	copyTexture(CTexture* copy, bool keep_type = false);

	string		getName() { return name; }
	uint16_t	getWidth() { return width; }
	uint16_t	getHeight() { return height; }
	double		getScaleX() { return scale_x; }
	double		getScaleY() { return scale_y; }
	int16_t		getOffsetX() { return offset_x; }
	int16_t		getOffsetY() { return offset_y; }
	bool		worldPanning() { return world_panning; }
	string		getType() { return type; }
	bool		isExtended() { return extended; }
	bool		isOptional() { return optional; }
	bool		noDecals() { return no_decals; }
	bool		nullTexture() { return null_texture; }
	size_t		nPatches() { return patches.size(); }
	CTPatch*	getPatch(size_t index);
	uint8_t		getState() { return state; }
	int			getIndex();

	void	setName(string name) { this->name = name; }
	void	setWidth(uint16_t width) { this->width = width; }
	void	setHeight(uint16_t height) { this->height = height; }
	void	setScaleX(double scale) { this->scale_x = scale; }
	void	setScaleY(double scale) { this->scale_y = scale; }
	void	setScale(double x, double y) { this->scale_x = x; this->scale_y = y; }
	void	setOffsetX(int16_t offset) { this->offset_x = offset; }
	void	setOffsetY(int16_t offset) { this->offset_y = offset; }
	void	setWorldPanning(bool wp) { this->world_panning = wp; }
	void	setType(string type) { this->type = type; }
	void	setExtended(bool ext) { this->extended = ext; }
	void	setOptional(bool opt) { this->optional = opt; }
	void	setNoDecals(bool nd) { this->no_decals = nd; }
	void	setNullTexture(bool nt) { this->null_texture = nt; }
	void	setState(uint8_t state) { this->state = state; }
	void	setList(TextureXList* list) { this->in_list = list; }

	void	clear();

	bool	addPatch(string patch, int16_t offset_x = 0, int16_t offset_y = 0, int index = -1);
	bool	removePatch(size_t index);
	bool	removePatch(string patch);
	bool	replacePatch(size_t index, string newpatch);
	bool	duplicatePatch(size_t index, int16_t offset_x = 8, int16_t offset_y = 8);
	bool	swapPatches(size_t p1, size_t p2);

	bool	parse(Tokenizer& tz, string type);
	bool	parseDefine(Tokenizer& tz);
	string	asText();

	bool	convertExtended();
	bool	convertRegular();
	bool	loadPatchImage(unsigned pindex, SImage& image, Archive* parent = NULL, Palette8bit* pal = NULL);
	bool	toImage(SImage& image, Archive* parent = NULL, Palette8bit* pal = NULL, bool force_rgba = false);

	typedef std::unique_ptr<CTexture>	UPtr;
	typedef std::shared_ptr<CTexture>	SPtr;
};

#endif//__CTEXTURE_H__
