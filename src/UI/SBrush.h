#pragma once

#include "Graphics/SImage/SImage.h"

namespace slade
{
class SAction;

class SBrush
{
public:
	SBrush(const string& name);
	~SBrush() = default;

	// SAction getAction(); // Returns an action ready to be inserted in a menu or toolbar (NYI)

	string  name() const { return name_; } // Returns the brush's name ("pgfx_brush_xyz")
	string  icon() const { return icon_; } // Returns the brush's icon name ("brush_xyz")
	uint8_t pixel(int x, int y) const;

	static SBrush* get(const string& name);
	static bool    initBrushes();

private:
	unique_ptr<SImage> image_ = nullptr; // The cursor graphic
	string             name_;
	string             icon_;
	Vec2i              center_;
};
} // namespace slade
