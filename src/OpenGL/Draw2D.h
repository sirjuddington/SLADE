#pragma once

#include "OpenGL.h"
#include "Utility/Colour.h"

namespace slade
{
class GLCanvas;

namespace gl
{
	class Shader;

	namespace draw2d
	{
		struct RenderOptions
		{
			unsigned texture        = 0;
			ColRGBA  colour         = ColRGBA::WHITE;
			Blend    blend          = Blend::Ignore;
			float    line_thickness = 1.f;

			RenderOptions(
				unsigned       texture        = 0,
				const ColRGBA& colour         = ColRGBA::WHITE,
				Blend          blend          = Blend::Ignore,
				float          line_thickness = 1.f) :
				texture{ texture }, colour{ colour }, blend{ blend }, line_thickness{ line_thickness }
			{
			}
		};

		enum class PointSprite
		{
			Textured,
			Circle
		};

		const Shader& defaultShader(bool textured = true);
		const Shader& linesShader();
		const Shader& pointSpriteShader(PointSprite type);

		void drawRect(Rectf rect, const RenderOptions& opt = {});
		void drawRectOutline(Rectf rect, const RenderOptions& opt = {});

	} // namespace draw2d
} // namespace gl
} // namespace slade
