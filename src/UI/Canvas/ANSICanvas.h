#pragma once

#include "OGLCanvas.h"

class GLTexture;

class ANSICanvas : public OGLCanvas
{
public:
	ANSICanvas(wxWindow* parent, int id);
	~ANSICanvas();

	void draw() override;
	void drawImage() const;
	void writeRGBAData(uint8_t* dest) const;
	void loadData(uint8_t* data) { ansidata_ = data; }
	void drawCharacter(size_t index) const;

private:
	size_t         width_       = 0;
	size_t         height_      = 0;
	uint8_t*       picdata_     = nullptr;
	const uint8_t* fontdata_    = nullptr;
	uint8_t*       ansidata_    = nullptr;
	GLTexture*     tex_image_   = nullptr;
	int            char_width_  = 8;
	int            char_height_ = 8;
};
