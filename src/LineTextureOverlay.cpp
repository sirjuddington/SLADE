
#include "Main.h"
#include "WxStuff.h"
#include "LineTextureOverlay.h"
#include "MapLine.h"
#include "MapSide.h"
#include "ColourConfiguration.h"
#include "Drawing.h"
#include "MapEditorWindow.h"
#include "MapTextureBrowser.h"


LineTextureOverlay::LineTextureOverlay()
{
	// Init variables
	tex_size = 0;
	last_width = 0;
	last_height = 0;
	selected_side = 0;
}

LineTextureOverlay::~LineTextureOverlay()
{
}

void LineTextureOverlay::addTexture(tex_inf_t& inf, string texture)
{
	// Ignore if texture is blank ("-")
	if (texture == "-")
		return;

	// Add texture if it doesn't exist already
	bool exists = false;
	for (unsigned a = 0; a < inf.textures.size(); a++)
	{
		if (inf.textures[a] == texture)
		{
			exists = true;
			break;
		}
	}
	if (!exists)
		inf.textures.push_back(texture);
}

void LineTextureOverlay::openLines(vector<MapLine*>& list)
{
	// Clear current lines
	lines.clear();
	this->side1 = false;
	this->side2 = false;
	selected_side = 0;
	for (unsigned a = 0; a < 6; a++)
	{
		textures[a].textures.clear();
		textures[a].hover = false;
		textures[a].changed = false;
	}

	// Go through list
	for (unsigned a = 0; a < list.size(); a++)
	{
		// Add to lines list
		lines.push_back(list[a]);

		// Process first side
		MapSide* side1 = list[a]->s1();
		if (side1)
		{
			// Add textures
			addTexture(textures[FRONT_UPPER], side1->getTexUpper());
			addTexture(textures[FRONT_MIDDLE], side1->getTexMiddle());
			addTexture(textures[FRONT_LOWER], side1->getTexLower());

			this->side1 = true;
		}

		// Process second side
		MapSide* side2 = list[a]->s2();
		if (side2)
		{
			// Add textures
			addTexture(textures[BACK_UPPER], side2->getTexUpper());
			addTexture(textures[BACK_MIDDLE], side2->getTexMiddle());
			addTexture(textures[BACK_LOWER], side2->getTexLower());

			this->side2 = true;
		}
	}

	if (!side1)
		selected_side = 1;
}

void LineTextureOverlay::close(bool cancel)
{
	// Apply texture changes if not cancelled
	if (!cancel)
	{
		theMapEditor->mapEditor().beginUndoRecord("Change Line Texture", true, false, false);

		// Go through lines
		for (unsigned a = 0; a < lines.size(); a++)
		{
			// Front Upper
			if (textures[FRONT_UPPER].changed)
				lines[a]->setStringProperty("side1.texturetop", textures[FRONT_UPPER].textures[0]);

			// Front Middle
			if (textures[FRONT_MIDDLE].changed)
				lines[a]->setStringProperty("side1.texturemiddle", textures[FRONT_MIDDLE].textures[0]);

			// Front Lower
			if (textures[FRONT_LOWER].changed)
				lines[a]->setStringProperty("side1.texturebottom", textures[FRONT_LOWER].textures[0]);


			// Back Upper
			if (textures[BACK_UPPER].changed)
				lines[a]->setStringProperty("side2.texturetop", textures[BACK_UPPER].textures[0]);

			// Back Middle
			if (textures[BACK_MIDDLE].changed)
				lines[a]->setStringProperty("side2.texturemiddle", textures[BACK_MIDDLE].textures[0]);

			// Back Lower
			if (textures[BACK_LOWER].changed)
				lines[a]->setStringProperty("side2.texturebottom", textures[BACK_LOWER].textures[0]);
		}

		theMapEditor->mapEditor().endUndoRecord();
	}

	// Deactivate
	active = false;
}

void LineTextureOverlay::update(long frametime)
{
}

void LineTextureOverlay::updateLayout(int width, int height)
{
	// Determine layout stuff
	int rows = 1;
	if (side1 && side2)
		rows = 2;
	int middlex = width * 0.5;
	int middley = height * 0.5;
	int maxsize = min(width / 3, height / rows);
	tex_size = maxsize - 64;
	if (tex_size > 256)
		tex_size = 256;
	int border = (maxsize - tex_size) * 0.5;
	if (border > 48)
		border = 48;

	// Set texture positions
	int ymid = middley;
	if (rows == 2)
		ymid = middley - (border*0.5) - (tex_size*0.5);
	if (side1)
	{
		// Front side textures
		int xmid = middlex - border - tex_size;
		textures[FRONT_UPPER].position.x = xmid;
		textures[FRONT_UPPER].position.y = ymid;

		xmid += border + tex_size;
		textures[FRONT_MIDDLE].position.x = xmid;
		textures[FRONT_MIDDLE].position.y = ymid;

		xmid += border + tex_size;
		textures[FRONT_LOWER].position.x = xmid;
		textures[FRONT_LOWER].position.y = ymid;

		ymid += border + tex_size;
	}
	if (side2)
	{
		// Back side textures
		int xmid = middlex - border - tex_size;
		textures[BACK_UPPER].position.x = xmid;
		textures[BACK_UPPER].position.y = ymid;

		xmid += border + tex_size;
		textures[BACK_MIDDLE].position.x = xmid;
		textures[BACK_MIDDLE].position.y = ymid;

		xmid += border + tex_size;
		textures[BACK_LOWER].position.x = xmid;
		textures[BACK_LOWER].position.y = ymid;
	}

	last_width = width;
	last_height = height;
}

void LineTextureOverlay::draw(int width, int height, float fade)
{
	// Update layout if needed
	if (width != last_width || height != last_height)
		updateLayout(width, height);

	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_bg.a *= fade;
	col_fg.a *= fade;

	// Draw background
	glDisable(GL_TEXTURE_2D);
	OpenGL::setColour(col_bg);
	Drawing::drawFilledRect(0, 0, width, height);

	// Draw textures
	glEnable(GL_LINE_SMOOTH);
	int cur_size = tex_size*fade;
	if (!active)
		cur_size = tex_size;
	if (side1)
	{
		drawTexture(fade, cur_size, textures[FRONT_LOWER], "Front Lower");
		drawTexture(fade, cur_size, textures[FRONT_MIDDLE], "Front Middle");
		drawTexture(fade, cur_size, textures[FRONT_UPPER], "Front Upper");
	}
	if (side2)
	{
		drawTexture(fade, cur_size, textures[BACK_LOWER], "Back Lower");
		drawTexture(fade, cur_size, textures[BACK_MIDDLE], "Back Middle");
		drawTexture(fade, cur_size, textures[BACK_UPPER], "Back Upper");
	}
}

void LineTextureOverlay::drawTexture(float alpha, int size, tex_inf_t& tex, string position)
{
	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	rgba_t col_sel = ColourConfiguration::getColour("map_hilight");
	col_fg.a = col_fg.a*alpha;

	// Draw background
	int halfsize = size*0.5;
	glEnable(GL_TEXTURE_2D);
	OpenGL::setColour(255, 255, 255, 255*alpha, 0);
	glPushMatrix();
	glTranslated(tex.position.x - halfsize, tex.position.y - halfsize, 0);
	GLTexture::bgTex().draw2dTiled(size, size);
	glPopMatrix();

	GLTexture* tex_first = NULL;
	if (tex.textures.size() > 0)
	{
		// Draw first texture
		OpenGL::setColour(255, 255, 255, 255*alpha, 0);
		tex_first = theMapEditor->textureManager().getTexture(tex.textures[0], theGameConfiguration->mixTexFlats());
		Drawing::drawTextureWithin(tex_first, tex.position.x - halfsize, tex.position.y - halfsize,
		                           tex.position.x + halfsize, tex.position.y + halfsize, 0, 2);

		// Draw up to 4 subsequent textures (overlaid)
		OpenGL::setColour(255, 255, 255, 127*alpha, 0);
		for (unsigned a = 1; a < tex.textures.size() && a < 5; a++)
		{
			Drawing::drawTextureWithin(theMapEditor->textureManager().getTexture(tex.textures[a], theGameConfiguration->mixTexFlats()),
			                           tex.position.x - halfsize, tex.position.y - halfsize,
			                           tex.position.x + halfsize, tex.position.y + halfsize, 0, 2);
		}
	}
	else
		return;

	glDisable(GL_TEXTURE_2D);

	// Draw outline
	if (tex.hover)
	{
		OpenGL::setColour(col_sel.r, col_sel.g, col_sel.b, 255*alpha, 0);
		glLineWidth(3.0f);
	}
	else
	{
		OpenGL::setColour(col_fg.r, col_fg.g, col_fg.b, 255*alpha, 0);
		glLineWidth(1.5f);
	}
	Drawing::drawRect(tex.position.x-halfsize, tex.position.y-halfsize, tex.position.x+halfsize, tex.position.y+halfsize);

	// Draw position text
	Drawing::drawText(position + ":", tex.position.x, tex.position.y - halfsize - 18, col_fg, Drawing::FONT_BOLD, Drawing::ALIGN_CENTER);

	// Determine texture name text
	string str_texture;
	if (tex.textures.size() == 1)
		str_texture = S_FMT("%s (%dx%d)", tex.textures[0], tex_first->getWidth(), tex_first->getHeight());
	else if (tex.textures.size() > 1)
		str_texture = S_FMT("Multiple (%d)", tex.textures.size());
	else
		str_texture = "- (None)";

	// Draw texture name
	Drawing::drawText(str_texture, tex.position.x, tex.position.y + halfsize + 2, col_fg, Drawing::FONT_BOLD, Drawing::ALIGN_CENTER);
}

void LineTextureOverlay::mouseMotion(int x, int y)
{
	// Check textures for hover
	int halfsize = tex_size*0.5;
	if (side1)
	{
		textures[FRONT_UPPER].checkHover(x, y, halfsize);
		textures[FRONT_MIDDLE].checkHover(x, y, halfsize);
		textures[FRONT_LOWER].checkHover(x, y, halfsize);
	}
	if (side2)
	{
		textures[BACK_UPPER].checkHover(x, y, halfsize);
		textures[BACK_MIDDLE].checkHover(x, y, halfsize);
		textures[BACK_LOWER].checkHover(x, y, halfsize);
	}
}

void LineTextureOverlay::mouseLeftClick()
{
	// Check for any hover
	if (textures[FRONT_UPPER].hover)		browseTexture(textures[FRONT_UPPER], "Front Upper");
	else if (textures[FRONT_MIDDLE].hover)	browseTexture(textures[FRONT_MIDDLE], "Front Middle");
	else if (textures[FRONT_LOWER].hover)	browseTexture(textures[FRONT_LOWER], "Front Lower");
	else if (textures[BACK_UPPER].hover)	browseTexture(textures[BACK_UPPER], "Back Upper");
	else if (textures[BACK_MIDDLE].hover)	browseTexture(textures[BACK_MIDDLE], "Back Middle");
	else if (textures[BACK_LOWER].hover)	browseTexture(textures[BACK_LOWER], "Back Lower");
}

void LineTextureOverlay::mouseRightClick()
{
}

void LineTextureOverlay::keyDown(string key)
{
	// 'Select' front side
	if (key == "F" || key == "f" && side1)
		selected_side = 0;

	// 'Select' back side
	if (key == "B" || key == "b" && side2)
		selected_side = 1;

	// Browse upper texture
	if (key == "U" || key == "u")
	{
		if (selected_side == 0)
			browseTexture(textures[FRONT_UPPER], "Front Upper");
		else
			browseTexture(textures[BACK_UPPER], "Back Upper");
	}

	// Browse middle texture
	if (key == "M" || key == "m")
	{
		if (selected_side == 0)
			browseTexture(textures[FRONT_MIDDLE], "Front Middle");
		else
			browseTexture(textures[BACK_MIDDLE], "Back Middle");
	}

	// Browse lower texture
	if (key == "L" || key == "l")
	{
		if (selected_side == 0)
			browseTexture(textures[FRONT_LOWER], "Front Lower");
		else
			browseTexture(textures[BACK_LOWER], "Back Lower");
	}
}

void LineTextureOverlay::browseTexture(tex_inf_t& tex, string position)
{
	// Get initial texture
	string texture;
	if (tex.textures.size() > 0)
		texture = tex.textures[0];
	else
		texture = "-";

	// Open texture browser
	MapTextureBrowser browser(theMapEditor, 0, texture, &(theMapEditor->mapEditor().getMap()));
	browser.SetTitle(S_FMT("Browse %s Texture", position));
	if (browser.ShowModal() == wxID_OK)
	{
		// Set texture
		tex.textures.clear();
		tex.textures.push_back(browser.getSelectedItem()->getName());
		tex.changed = true;
		close(false);
	}
}
