#pragma once

namespace slade
{
class PaletteCanvas;

class PaletteDialog : public wxDialog
{
public:
	PaletteDialog(const Palette* palette);
	~PaletteDialog() override = default;

	ColRGBA selectedColour() const;

private:
	PaletteCanvas* pal_canvas_ = nullptr;
};
} // namespace slade
