
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
	border_style = Border::Line;
	max_scale = baseScale();
	image_colour = COL_WHITE;
}

ImageBox::~ImageBox()
{
}

void ImageBox::setTexture(GLTexture* texture)
{
	this->texture = texture;
}

void ImageBox::setMaxImageScale(double scale)
{
	max_scale = scale * baseScale();
}

void ImageBox::setSizeFromImage()
{
	if (texture)
		setSize(dim2_t(texture->getWidth() * baseScale(), texture->getHeight() * baseScale()));
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
		texture = new_tex;
		return true;
	}
}

void ImageBox::drawWidget(point2_t pos, float alpha)
{
	rgba_t col_fg = COL_WHITE;
	col_fg.a *= alpha;

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
}
