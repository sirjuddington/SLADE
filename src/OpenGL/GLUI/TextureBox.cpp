
#include "Main.h"
#include "TextureBox.h"
#include "ImageBox.h"
#include "LayoutHelpers.h"
#include "OpenGL/Drawing.h"
#include "TextBox.h"

// Really need to move the texture manager out of the map editor window,
// shouldn't need WxStuff.h just to get the map texture manager
#include "MapEditor/MapEditorWindow.h"

using namespace GLUI;

TextureBox::TextureBox(Widget* parent)
	: Panel(parent),
	box_size(80),
	image_texture(new ImageBox(this)),
	text_name(new TextBox(this, "")),
	show_always(false)
{
	text_name->setMargin(padding_t(0, 2, 0, 0));
	image_texture->setSize(dim2_t(box_size * baseScale(), box_size * baseScale()));
	setBGCol(rgba_t(0, 0, 0, 0));
}

void TextureBox::setTexture(int type, string texname, string prefix, bool required)
{
	image_texture->setVisible(true);
	image_texture->setSize(dim2_t(box_size * baseScale(), box_size * baseScale()));
	image_texture->setImageColour(COL_WHITE);

	text_name->setText(prefix + texname);
	text_name->setColour(TextBox::defaultColour());

	// Get texture
	bool mixed = theGameConfiguration->mixTexFlats();
	GLTexture* tex;
	if (type == FLAT)
		tex = theMapEditor->textureManager().getFlat(texname, mixed);
	else if (type == SPRITE)
		tex = theMapEditor->textureManager().getSprite(texname, sprite_translation, sprite_palette);
	else
		tex = theMapEditor->textureManager().getTexture(texname, mixed);

	// Valid texture
	if (texname != "-" && tex != &(GLTexture::missingTex()))
	{
		image_texture->setTexture(tex);
		image_texture->setBackgroundStyle(ImageBox::BACKGROUND_CHECKERBOARD);
		image_texture->setBorderStyle(Border::Line);
		image_texture->setMaxImageScale(1);
	}

	// Unknown texture
	else if (tex == &(GLTexture::missingTex()) && texname != "-")
	{
		image_texture->setTexture(theMapEditor->textureManager().getEditorImage("thing/unknown"));
		image_texture->setBackgroundStyle(ImageBox::BACKGROUND_NONE);
		image_texture->setBorderStyle(Border::None);
		image_texture->setMaxImageScale(0.15);
	}

	// Missing texture
	else if (required)
	{
		image_texture->setTexture(theMapEditor->textureManager().getEditorImage("thing/minus"));
		image_texture->setBackgroundStyle(ImageBox::BACKGROUND_NONE);
		image_texture->setBorderStyle(Border::None);
		image_texture->setMaxImageScale(0.15);
		image_texture->setImageColour(rgba_t(180, 0, 0));
		text_name->setColour(COL_RED);
		text_name->setText(prefix + ": MISSING");
	}

	// No texture
	else
	{
		image_texture->setTexture(NULL);
		image_texture->setVisible(show_always);
	}

	text_name->updateLayout();
}

void TextureBox::setTexture(GLTexture* texture, string texname)
{
	image_texture->setVisible(true);
	image_texture->setSize(dim2_t(box_size * baseScale(), box_size * baseScale()));
	image_texture->setImageColour(COL_WHITE);

	text_name->setText(texname);
	text_name->setColour(TextBox::defaultColour());

	// Valid texture
	if (texname != "-" && texture != &(GLTexture::missingTex()))
	{
		image_texture->setTexture(texture);
		image_texture->setBackgroundStyle(ImageBox::BACKGROUND_CHECKERBOARD);
		image_texture->setBorderStyle(Border::Line);
		image_texture->setMaxImageScale(1);
	}

	// Unknown texture
	else if (texture == &(GLTexture::missingTex()) && texname != "-")
	{
		image_texture->setTexture(theMapEditor->textureManager().getEditorImage("thing/unknown"));
		image_texture->setBackgroundStyle(ImageBox::BACKGROUND_NONE);
		image_texture->setBorderStyle(Border::None);
		image_texture->setMaxImageScale(0.15);
	}

	// No texture
	else
	{
		image_texture->setTexture(NULL);
		image_texture->setVisible(show_always);
	}

	text_name->updateLayout();
}

void TextureBox::updateLayout(dim2_t fit)
{
	LayoutHelpers::placeWidgetBelow(text_name, image_texture, USE_MARGIN, ALIGN_MIDDLE);
	fitToChildren(padding_t(0), true);
}
