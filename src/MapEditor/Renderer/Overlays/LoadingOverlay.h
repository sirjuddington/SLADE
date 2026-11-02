#pragma once

#include "MCOverlay.h"

// Forward declarations
namespace slade::gl::draw2d
{
struct Context;
}

namespace slade::mapeditor
{
class LoadingOverlay : public MCOverlay
{
public:
	LoadingOverlay();
	~LoadingOverlay() override;

	void setMessage(string_view message, bool activate = true);

	void close(bool cancel) override { active_ = false; }

	void draw(gl::draw2d::Context& dc, float fade = 1.0f) override;

private:
	string message_;
};
} // namespace slade::mapeditor
