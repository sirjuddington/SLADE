
#ifndef __ANSICANVAS_H__
#define	__ANSICANVAS_H__

#include "OGLCanvas.h"
#include "GLTexture.h"

class ANSICanvas : public OGLCanvas, Listener
{
private:
	size_t			width, height;
	uint8_t*		picdata;
	const uint8_t*	fontdata;
	uint8_t*		ansidata;
	GLTexture*		tex_image;
	int				char_width;
	int				char_height;

public:
	ANSICanvas(wxWindow* parent, int id);
	~ANSICanvas();

	void	draw();
	void	drawImage();
	void	writeRGBAData(uint8_t* dest);
	void	loadData(uint8_t* data) { ansidata = data; }
	void	drawCharacter(size_t index);
	// Events
};

#endif //__ANSICANVAS_H__
