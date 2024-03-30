#pragma once

namespace slade
{
class SImage;
class SAction;

class SBrush
{
public:
	SBrush(const wxString& name);
	~SBrush() = default;

	// SAction getAction(); // Returns an action ready to be inserted in a menu or toolbar (NYI)

	wxString name() const { return name_; } // Returns the brush's name ("pgfx_brush_xyz")
	wxString icon() const { return icon_; } // Returns the brush's icon name ("brush_xyz")
	uint8_t  pixel(int x, int y) const;

	static SBrush* get(const wxString& name);
	static bool    initBrushes();

private:
	unique_ptr<SImage> image_ = nullptr; // The cursor graphic
	wxString           name_;
	wxString           icon_;
	Vec2i              center_;
};
} // namespace slade
