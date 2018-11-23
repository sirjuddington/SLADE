
#ifndef __GLTEXTURE_H__
#define	__GLTEXTURE_H__

struct gl_tex_t
{
	unsigned	id;
	uint32_t	width;
	uint32_t	height;
};

class SImage;
class Palette;

class GLTexture
{
private:
	uint32_t			width;
	uint32_t			height;
	vector<gl_tex_t>	tex;
	int					filter;
	bool				loaded;
	bool				allow_split;
	bool				tiling;
	double				scale_x;
	double				scale_y;
	bool 				world_panning;

	// Some generic/global textures
	static GLTexture	tex_background;	// Checkerboard background texture
	static GLTexture	tex_missing;	// Checkerboard 'missing' texture

	// Stuff used internally
	bool	loadData(const uint8_t* data, uint32_t width, uint32_t height, bool add = false);
	bool	loadImagePortion(SImage* image, rect_t rect, Palette* pal = nullptr, bool add = false);

public:
	enum
	{
	    // Filter types
	    NEAREST,
	    LINEAR,
	    MIPMAP,
	    LINEAR_MIPMAP,	// (same as MIPMAP)
	    NEAREST_LINEAR_MIN,
	    NEAREST_MIPMAP,
	};

	GLTexture(bool allow_split = true);
	~GLTexture();

	bool		isLoaded() { return loaded; }
	uint32_t	getWidth() { return width; }
	uint32_t	getHeight() { return height; }
	int			getFilter() { return filter; }
	double		getScaleX() { return scale_x; }
	double		getScaleY() { return scale_y; }
	bool		isTiling() { return tiling; }
	unsigned	glId() { if (!tex.empty()) return tex[0].id; else return 0; }
	bool		worldPanning() { return world_panning; }

	void		setWorldPanning(bool wp) { world_panning = wp; }
	void		setFilter(int filter) { this->filter = filter; }
	void		setTiling(bool tiling) { this->tiling = tiling; }
	void		setScale(double sx, double sy) { this->scale_x = sx; this->scale_y = sy; }

	bool	loadImage(SImage* image, Palette* pal = nullptr);
	bool	loadRawData(const uint8_t* data, uint32_t width, uint32_t height);

	bool	clear();
	bool	genChequeredTexture(uint8_t block_size, rgba_t col1, rgba_t col2);

	bool	bind();
	bool	draw2d(double x = 0, double y = 0, bool flipx = false, bool flipy = false);
	bool	draw2dTiled(uint32_t width, uint32_t height);

	rgba_t	averageColour(rect_t area);

	static GLTexture&	bgTex();
	static GLTexture&	missingTex();
	static void			resetBgTex();
};

#endif//__GLTEXTURE_H__
