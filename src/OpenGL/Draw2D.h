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

		// Context struct - handles general 2d drawing using a given view
		struct Context
		{
			Context() = default;
			Context(const View* view) : view{ view } {}

			const View* view;
			glm::mat4   model_matrix = glm::mat4{ 1.0f };

			unsigned  texture                = 0;
			ColRGBA   colour                 = ColRGBA::WHITE;
			ColRGBA   outline_colour         = ColRGBA::BLACK;
			Blend     blend                  = Blend::Ignore;
			float     line_thickness         = 1.0f;
			float     line_aa_radius         = 2.0f;
			Font      font                   = Font::Normal;
			int       text_size              = 18;
			Align     text_alignment         = Align::Left;
			TextStyle text_style             = TextStyle::Normal;
			bool      text_dropshadow        = false;
			ColRGBA   text_dropshadow_colour = ColRGBA::BLACK;

			void resetModel() { model_matrix = glm::mat4{ 1.0f }; }
			void translate(float x, float y);
			void scale(float x, float y);

			float textLineHeight() const;
			Vec2f textExtents(const string& text) const;

			void drawRect(const Rectf& rect) const;
			void drawRectOutline(const Rectf& rect) const;
			void drawLines(const vector<Rectf>& lines) const;
			void drawText(const string& text, const Vec2f& pos) const;
			void drawHud() const;
		};

		// TextBox class
		class TextBox
		{
		public:
			TextBox(string_view text, float width, Font font, int font_size = 18, float line_height = 1.0f);
			~TextBox() = default;

			float height() const { return height_; }
			float width() const { return width_; }
			void  setText(string_view text);
			void  setWidth(float width);
			void  setLineHeight(float height) { line_height_ = height; }
			void  draw(Vec2f& pos, Context& dc) const;

		private:
			string         text_;
			vector<string> lines_;
			Font           font_        = Font::Normal;
			int            font_size_   = 18;
			float          width_       = 0;
			float          height_      = 0;
			float          line_height_ = -1;

			void split(string_view text);
		};

		// Font/text metrics
		float lineHeight(Font font, int size = 18);
		Vec2f textExtents(const string& text, Font font, int size = 18);

		// 2d drawing shaders
		const Shader& defaultShader(bool textured = true);
		const Shader& linesShader();
		const Shader& pointSpriteShader(PointSprite type);
	} // namespace draw2d
} // namespace gl
} // namespace slade
