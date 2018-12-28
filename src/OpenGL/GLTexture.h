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

	GLTexture(bool allow_split = true) : allow_split_{ allow_split } {}
	~GLTexture();

	bool     isLoaded() const { return loaded_; }
	uint32_t width() const { return width_; }
	uint32_t height() const { return height_; }
	Filter   filter() const { return filter_; }
	double   scaleX() const { return scale_.x; }
	double   scaleY() const { return scale_.y; }
	bool     isTiling() const { return tiling_; }
	unsigned glId() { return !tex_.empty() ? tex_[0].id : 0; }
	bool     worldPanning() const { return world_panning_; }

	void setWorldPanning(bool wp) { world_panning_ = wp; }
	void setFilter(Filter filter) { filter_ = filter; }
	void setTiling(bool tiling) { tiling_ = tiling; }
	void setScale(double sx, double sy) { scale_.set(sx, sy); }

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

	typedef std::unique_ptr<GLTexture> UPtr;

private:
	struct SubTex
	{
		unsigned id;
		uint32_t width;
		uint32_t height;
	};

	uint32_t       width_  = 0;
	uint32_t       height_ = 0;
	vector<SubTex> tex_;
	Filter         filter_        = Filter::Nearest;
	bool           loaded_        = false;
	bool           allow_split_   = false;
	bool           tiling_        = true;
	Vec2f          scale_         = { 1., 1. };
	bool           world_panning_ = false;

	// Some generic/global textures
	static GLTexture tex_background_; // Checkerboard background texture
	static GLTexture tex_missing_;    // Checkerboard 'missing' texture

	// Stuff used internally
	bool loadData(const uint8_t* data, uint32_t width, uint32_t height, bool add = false);
	bool loadImagePortion(SImage* image, Recti rect, Palette* pal = nullptr, bool add = false);
};
