#pragma once

#include "Geometry/RectFwd.h"

namespace slade
{
class SImage;

namespace gl
{
	enum class TexFilter
	{
		// Filter types
		Nearest,
		Linear,
		Mipmap,
		LinearMipmap, // (same as Mipmap)
		NearestLinearMin,
		NearestMipmap,
	};

	struct Texture
	{
		unsigned  id     = 0;
		Vec2i     size   = { 0, 0 };
		TexFilter filter = TexFilter::Nearest;
		bool      tiling = true;

		static bool isCreated(unsigned id); // const { return id > 0; }
		static bool isLoaded(unsigned id);  // const { return id > 0 && size.x > 0 && size.y > 0; }

		static const Texture& info(unsigned id);
		static ColRGBA        averageColour(unsigned id, Recti area);
		static void           bind(unsigned id, bool force = true);

		static unsigned whiteTexture();
		static unsigned missingTexture();
		static unsigned backgroundTexture();
		static void     resetBackgroundTexture();

		static unsigned create(TexFilter filter = TexFilter::Nearest, bool tiling = true);
		static unsigned createFromData(
			const uint8_t* data,
			unsigned       width,
			unsigned       height,
			TexFilter      filter = TexFilter::Nearest,
			bool           tiling = true);
		static unsigned createFromImage(
			const SImage&  image,
			const Palette* pal    = nullptr,
			TexFilter      filter = TexFilter::Nearest,
			bool           tiling = true);
		static bool loadData(unsigned id, const uint8_t* data, unsigned width, unsigned height);
		static bool loadAlphaData(unsigned id, const uint8_t* data, unsigned width, unsigned height);
		static bool loadImage(unsigned id, const SImage& image, const Palette* pal = nullptr);
		static bool genChequeredTexture(unsigned id, uint8_t block_size, ColRGBA col1, ColRGBA col2);
		static void clear(unsigned id);
		static void clearAll();
		static void setTiling(unsigned id, bool tiling);
	};

} // namespace gl
} // namespace slade
