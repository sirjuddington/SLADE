#pragma once

#include "OGLCanvas.h"

namespace slade
{
class ANSICanvas : public OGLCanvas
{
public:
	ANSICanvas(wxWindow* parent, int id);
	~ANSICanvas();

	void draw() override;
	void drawImage();
	void writeRGBAData(uint8_t* dest) const;
	void loadData(uint8_t* data);
	void drawCharacter(size_t index) const;

private:
	size_t         width_       = 0;
	size_t         height_      = 0;
	uint8_t*       picdata_     = nullptr;
	const uint8_t* fontdata_    = nullptr;
	uint8_t*       ansidata_    = nullptr;
	unsigned       tex_image_   = 0;
	int            char_width_  = 8;
	int            char_height_ = 8;
};
} // namespace slade
