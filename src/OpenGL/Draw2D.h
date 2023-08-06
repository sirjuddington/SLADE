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
		// Point Sprite types
		enum class PointSprite
		{
			Textured,
			Circle
		};

		// Font styles
		enum class Font
		{
			Normal,
			Bold,
			Condensed,
			CondensedBold,
			Monospace,
			MonospaceBold,
		};

		// Text styles
		enum class TextStyle
		{
			Normal,
			Outline,
			DropShadow
		};

		// Text alignment
		enum class Align
		{
			Left,
			Right,
			Center
		};

		// Options for rendering shapes
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

		// Options for rendering text
		struct TextOptions
		{
			Font      font                  = Font::Normal;
			ColRGBA   colour                = ColRGBA::WHITE;
			int       size                  = 18;
			Align     alignment             = Align::Left;
			TextStyle style                 = TextStyle::Normal;
			ColRGBA   outline_shadow_colour = ColRGBA::BLACK;
		};

		// Font/text metrics
		float lineHeight(Font font, int size = 18);
		Vec2f textExtents(const string& text, Font font, int size = 18);

		// 2d drawing shaders
		const Shader& defaultShader(bool textured = true);
		const Shader& linesShader();
		const Shader& pointSpriteShader(PointSprite type);

		// General drawing
		void drawRect(Rectf rect, const RenderOptions& opt = {}, const View* view = nullptr);
		void drawRectOutline(Rectf rect, const RenderOptions& opt = {}, const View* view = nullptr);
		void drawLines(const vector<Rectf>& lines, const RenderOptions& opt = {}, const View* view = nullptr);
		void drawText(const string& text, const Vec2f& pos, const TextOptions& opt = {}, const View* view = nullptr);

		// Specialized drawing
		void drawHud(const View* view = nullptr);
	} // namespace draw2d
} // namespace gl
} // namespace slade
