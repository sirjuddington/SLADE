#pragma once

#include "OpenGL/Buffer.h"

namespace slade
{
namespace game
{
	class ThingType;
}
namespace gl
{
	class View;

	class ThingBuffer2D
	{
	public:
		ThingBuffer2D();
		~ThingBuffer2D() = default;

		const glm::vec4& colour() const { return colour_; }
		float            radius() const { return radius_; }
		unsigned         texture() const { return texture_; }
		bool             showArrow() const { return arrow_; }

		void setup(const game::ThingType& type);
		void setTexture(unsigned texture, bool sprite);
		void setShadowOpacity(float opacity) { shadow_opacity_ = opacity; }

			void add(float x, float y, float angle, float alpha = 1.0f);

		void push();

		void draw(
			const View*      view        = nullptr,
			const glm::vec4& colour      = glm::vec4{ 1.0f },
			bool             square      = false,
			bool             force_arrow = false) const;

	private:
		struct ThingInstance
		{
			glm::vec2 position;
			glm::vec2 direction;
			float     alpha;
			ThingInstance(const glm::vec2& pos, const glm::vec2& dir, float alpha) :
				position{ pos }, direction{ dir }, alpha{ alpha }
			{
			}
		};

		glm::vec4             colour_         = glm::vec4{ 1.0f };
		float                 radius_         = 20.0f;
		unsigned              texture_        = 0;
		bool                  sprite_         = false;
		bool                  arrow_          = false;
		bool                  shrink_on_zoom_ = false;
		float                 shadow_opacity_ = 0.7f;
		glm::vec2             tex_size_       = glm::vec2{ 1.0f };
		vector<ThingInstance> things_;

		unsigned                          vao_ = 0;
		unique_ptr<Buffer<glm::vec2>>     buffer_square_;
		unique_ptr<Buffer<ThingInstance>> buffer_things_;

		void initVAO();
	};
} // namespace gl
} // namespace slade
