#pragma once

#include "OpenGL.h"
#include "Utility/Colour.h"

namespace slade
{
class GLCanvas;

namespace gl
{
	class Shader;
	class View;

	namespace draw2d
	{
		struct RenderOptions
		{
			unsigned texture        = 0;
			ColRGBA  colour         = ColRGBA::WHITE;
			Blend    blend          = Blend::Ignore;
			float    line_thickness = 1.0f;
			float    line_aa_radius = 2.0f;

			RenderOptions(
				unsigned       texture        = 0,
				const ColRGBA& colour         = ColRGBA::WHITE,
				Blend          blend          = Blend::Ignore,
				float          line_thickness = 1.0f) :
				texture{ texture }, colour{ colour }, blend{ blend }, line_thickness{ line_thickness }
			{
			}
		};

		enum class PointSprite
		{
			Textured,
			Circle
		};

		// 2d drawing shaders
		const Shader& defaultShader(bool textured = true);
		const Shader& linesShader();
		const Shader& pointSpriteShader(PointSprite type);

		void drawRect(Rectf rect, const RenderOptions& opt = {}, const View* view = nullptr);
		void drawRectOutline(Rectf rect, const RenderOptions& opt = {}, const View* view = nullptr);
		void drawLines(const vector<Rectf>& lines, const RenderOptions& opt = {}, const View* view = nullptr);

		void drawHud(const View* view = nullptr);

	} // namespace draw2d
} // namespace gl
} // namespace slade