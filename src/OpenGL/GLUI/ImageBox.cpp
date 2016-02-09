
#include "Main.h"
#include "ImageBox.h"
#include "Graphics/SImage/SImage.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/OpenGL.h"

using namespace GLUI;

ImageBox::ImageBox(Widget* parent) : Widget(parent)
{
	texture = NULL;
	background = true;
	border_style = BORDER_LINE;
	max_scale = 1;
	image_colour = COL_WHITE;
}

ImageBox::~ImageBox()
{
	//if (!texture)
	//	delete texture;
}

void ImageBox::setTexture(GLTexture* texture)
{
	// Delete existing texture if necessary
	//if (this->texture)
	//	delete this->texture;

	this->texture = texture;
}

void ImageBox::setSizeFromImage()
{
	if (texture)
		setSize(dim2_t(texture->getWidth(), texture->getHeight()));
}

bool ImageBox::loadImage(SImage* image, Palette8bit* palette)
{
	// Load image as new texture
	GLTexture* new_tex = new GLTexture();
	if (!new_tex->loadImage(image, palette))
	{
		delete new_tex;
		return false;
	}
	else
	{
		// Delete existing texture if necessary
		//if (texture)
		//	delete texture;

		texture = new_tex;
		return true;
	}
}

void ImageBox::drawWidget(point2_t pos)
{
	float alpha = 1.0f;
	rgba_t col_fg = COL_WHITE;

	glEnable(GL_TEXTURE_2D);

	// Draw background
	if (background == BACKGROUND_CHECKERBOARD)
	{
		OpenGL::setColour(255, 255, 255, 255 * alpha, 0);
		glPushMatrix();
		glTranslated(pos.x, pos.y, 0);
		GLTexture::bgTex().draw2dTiled(getWidth(), getHeight());
		glPopMatrix();
	}

	// Draw texture
	if (texture)
	{
		OpenGL::setColour(image_colour.r, image_colour.g, image_colour.b, 255 * alpha, 0);
		Drawing::drawTextureWithin(texture, pos.x, pos.y, pos.x + getWidth(), pos.y + getHeight(), 0, max_scale);
	}

	glDisable(GL_TEXTURE_2D);

	// Draw outline
	/*OpenGL::setColour(col_fg.r, col_fg.g, col_fg.b, 255 * alpha, 0);
	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);
	Drawing::drawRect(pos.x, pos.y, pos.x + getWidth(), pos.y + getHeight());*/
}
