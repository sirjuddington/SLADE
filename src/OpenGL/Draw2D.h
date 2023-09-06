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

			// Properties ------------------------------------------------------

			// General
			const View* view;
			glm::mat4   model_matrix   = glm::mat4{ 1.0f };
			unsigned    texture        = 0;
			ColRGBA     colour         = ColRGBA::WHITE;
			ColRGBA     outline_colour = ColRGBA::BLACK;
			Blend       blend          = Blend::Ignore;

			// Line drawing
			float line_thickness    = 1.0f;
			float line_aa_radius    = 2.0f;
			float line_arrow_length = 0.0f;
			float line_arrow_angle  = 45.0f;

			// Text drawing
			Font      font                   = Font::Normal;
			int       text_size              = 18;
			Align     text_alignment         = Align::Left;
			TextStyle text_style             = TextStyle::Normal;
			bool      text_dropshadow        = false;
			ColRGBA   text_dropshadow_colour = ColRGBA::BLACK;

			// PointSprite drawing
			PointSpriteType pointsprite_type          = PointSpriteType::Textured;
			float           pointsprite_radius        = 1.0f;
			float           pointsprite_outline_width = 0.05f;
			float           pointsprite_fill_opacity  = 0.0f;

			// Functions -------------------------------------------------------

			Vec2f viewSize() const;

			void resetModel() { model_matrix = glm::mat4{ 1.0f }; }
			void translate(float x, float y);
			void scale(float x, float y);
			void setColourFromConfig(const string& def_name, float alpha = 1.0f, bool blend = true);

			float textLineHeight() const;
			Vec2f textExtents(const string& text) const;
			float textScale() const { return static_cast<float>(text_size) / 18.0f; }

			void setupToDraw(const Shader& shader, bool mvp = true) const;

			void drawRect(const Rectf& rect) const;
			void drawRectOutline(const Rectf& rect) const;
			void drawLines(const vector<Rectf>& lines) const;
			void drawPointSprites(const vector<Vec2f>& points) const;
			void drawPointSprites(const vector<Vec2d>& points) const;
			void drawText(const string& text, const Vec2f& pos) const;
			void drawTextureTiled(const Rectf& rect) const;
			void drawTextureWithin(const Rectf& rect, float pad, float max_scale = 1.0f) const;
			void drawHud() const;
		};

		// TextBox class
		class TextBox
		{
		public:
			TextBox(string_view text, float width, Font font, int font_size = 18, float line_height = 1.0f);
			~TextBox() = default;

			float height();
			float width() const { return width_; }
			void  setText(string_view text);
			void  setWidth(float width);
			void  setFont(Font font, int size = 18);
			void  setLineHeight(float height) { line_height_ = height; }
			void  draw(const Vec2f& pos, Context& dc);

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
		const Shader& lineStippleShader(uint16_t pattern, float factor = 1.0f);
	} // namespace draw2d
} // namespace gl
} // namespace slade
