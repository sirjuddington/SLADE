#pragma once

#include "General/ListenerAnnouncer.h"
#include "OGLCanvas.h"

class GLTexture;

class ANSICanvas : public OGLCanvas, Listener
{
public:
	ANSICanvas(wxWindow* parent, int id);
	~ANSICanvas();

	void draw();
	void drawImage();
	void writeRGBAData(uint8_t* dest);
	void loadData(uint8_t* data) { ansidata_ = data; }
	void drawCharacter(size_t index);

private:
	size_t         width_, height_;
	uint8_t*       picdata_;
	const uint8_t* fontdata_;
	uint8_t*       ansidata_;
	GLTexture*     tex_image_;
	int            char_width_;
	int            char_height_;
};
