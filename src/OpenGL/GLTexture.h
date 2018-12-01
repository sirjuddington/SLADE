#pragma once

class SImage;
class Palette;

class GLTexture
{
public:
	enum class Filter
	{
		// Filter types
		Nearest,
		Linear,
		Mipmap,
		LinearMipmap, // (same as Mipmap)
		NearestLinearMin,
		NearestMipmap,
	};

	GLTexture(bool allow_split = true);
	~GLTexture();

	bool     isLoaded() { return loaded_; }
	uint32_t width() { return width_; }
	uint32_t height() { return height_; }
	Filter   filter() { return filter_; }
	double   scaleX() { return scale_x_; }
	double   scaleY() { return scale_y_; }
	bool     isTiling() { return tiling_; }
	unsigned glId()
	{
		if (!tex_.empty())
			return tex_[0].id;
		else
			return 0;
	}
	bool worldPanning() { return world_panning_; }

	void setWorldPanning(bool wp) { world_panning_ = wp; }
	void setFilter(Filter filter) { this->filter_ = filter; }
	void setTiling(bool tiling) { this->tiling_ = tiling; }
	void setScale(double sx, double sy)
	{
		this->scale_x_ = sx;
		this->scale_y_ = sy;
	}

	bool loadImage(SImage* image, Palette* pal = nullptr);
	bool loadRawData(const uint8_t* data, uint32_t width, uint32_t height);

	bool clear();
	bool genChequeredTexture(uint8_t block_size, ColRGBA col1, ColRGBA col2);

	bool bind();
	bool draw2d(double x = 0, double y = 0, bool flipx = false, bool flipy = false);
	bool draw2dTiled(uint32_t width, uint32_t height);

	ColRGBA averageColour(Recti area);

	static GLTexture& bgTex();
	static GLTexture& missingTex();
	static void       resetBgTex();

private:
	struct SubTex
	{
		unsigned id;
		uint32_t width;
		uint32_t height;
	};

	uint32_t       width_;
	uint32_t       height_;
	vector<SubTex> tex_;
	Filter         filter_;
	bool           loaded_;
	bool           allow_split_;
	bool           tiling_;
	double         scale_x_;
	double         scale_y_;
	bool           world_panning_;

	// Some generic/global textures
	static GLTexture tex_background_; // Checkerboard background texture
	static GLTexture tex_missing_;    // Checkerboard 'missing' texture

	// Stuff used internally
	bool loadData(const uint8_t* data, uint32_t width, uint32_t height, bool add = false);
	bool loadImagePortion(SImage* image, Recti rect, Palette* pal = nullptr, bool add = false);
};
