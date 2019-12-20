#pragma once

#include "OpenGL.h"
#include "Utility/Colour.h"

class GLCanvas;

namespace OpenGL
{
class Shader;

namespace Draw2D
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

	const Shader& defaultShader(bool textured = true);
	const Shader& linesShader();
	const Shader& pointSpriteShader();
	void          setupFor2D(const Shader& shader, const GLCanvas& canvas);

	void drawRect(Rectf rect, const RenderOptions& opt = {});
	void drawRectOutline(Rectf rect, const RenderOptions& opt = {});

} // namespace Draw2D
} // namespace OpenGL
