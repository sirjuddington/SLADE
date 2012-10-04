
#include "Main.h"
#include "WxStuff.h"
#include "MapSector.h"
#include "SectorTextureOverlay.h"
#include "Drawing.h"
#include "ColourConfiguration.h"
#include "MapEditorWindow.h"
#include "MapTextureBrowser.h"

SectorTextureOverlay::SectorTextureOverlay() {
	// Init variables
	hover_ceil = false;
	hover_floor = false;
	middlex = 0;
	middley = 0;
	tex_size = 0;
	border = 0;
	anim_floor = 0;
	anim_ceil = 0;
}

SectorTextureOverlay::~SectorTextureOverlay() {
}

void SectorTextureOverlay::update(long frametime) {
	// Get frame time multiplier
	float mult = (float)frametime / 10.0f;

	// Update animations
	anim_floor += 0.1f*mult;
	if (anim_floor > tex_floor.size())
		anim_floor = 0.0f;
	anim_ceil += 0.1f*mult;
	if (anim_ceil > tex_ceil.size())
		anim_ceil = 0.0f;
}

void SectorTextureOverlay::draw(int width, int height, float fade) {
	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_bg.a *= fade;
	col_fg.a *= fade;

	// Draw background
	glDisable(GL_TEXTURE_2D);
	col_bg.set_gl();
	Drawing::drawFilledRect(0, 0, width, height);

	// Check if any sectors are open
	if (sectors.size() == 0) {
		Drawing::drawText("No sectors are open. Just press escape and pretend this never happened.", width*0.5, height*0.5, COL_WHITE, 0, Drawing::ALIGN_CENTER);
		return;
	}

	// Calculate layout related stuff
	middlex = width * 0.5;
	middley = height * 0.5;
	tex_size = middlex - 64;
	if (tex_size > 256)
		tex_size = 256;
	border = (middlex - tex_size) * 0.5;
	if (border > 48)
		border = 48;
	int cur_size = tex_size;
	if (active) cur_size *= fade;

	// Determine texture name strings
	string ftex = tex_floor[0];
	string ctex = tex_ceil[0];
	string ftex2, ctex2;
	if (tex_floor.size() > 1) {
		ftex = S_FMT("Multiple (%d)", tex_floor.size());
		ftex2 = tex_floor[1];
	}
	if (tex_ceil.size() > 1) {
		ctex = S_FMT("Multiple (%d)", tex_ceil.size());
		ctex2 = tex_ceil[1];
	}

	// Floor texture
	glEnable(GL_LINE_SMOOTH);
	drawTexture(fade, middlex - border - tex_size*0.5 - cur_size*0.5, middley - cur_size*0.5, cur_size, tex_floor, hover_floor);
	Drawing::drawText("Floor:", middlex - border - tex_size*0.5, middley - tex_size*0.5 - 18, col_fg, Drawing::FONT_BOLD, Drawing::ALIGN_CENTER);
	Drawing::drawText(ftex, middlex - border - tex_size*0.5, middley + tex_size*0.5 + 2, col_fg, Drawing::FONT_BOLD, Drawing::ALIGN_CENTER);

	// Ceiling texture
	drawTexture(fade, middlex + border + tex_size*0.5 - cur_size*0.5, middley - cur_size*0.5, cur_size, tex_ceil, hover_ceil);
	Drawing::drawText("Ceiling:", middlex + border + tex_size*0.5, middley - tex_size*0.5 - 18, col_fg, Drawing::FONT_BOLD, Drawing::ALIGN_CENTER);
	Drawing::drawText(ctex, middlex + border + tex_size*0.5, middley + tex_size*0.5 + 2, col_fg, Drawing::FONT_BOLD, Drawing::ALIGN_CENTER);
}

void SectorTextureOverlay::drawTexture(float alpha, int x, int y, int size, vector<string>& textures, bool hover) {
	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	rgba_t col_sel = ColourConfiguration::getColour("map_hilight");
	col_fg.a = col_fg.a*alpha;

	// Draw background
	glEnable(GL_TEXTURE_2D);
	rgba_t(255, 255, 255, 255*alpha, 0).set_gl();
	glPushMatrix();
	glTranslated(x, y, 0);
	GLTexture::bgTex().draw2dTiled(size, size);
	glPopMatrix();

	// Draw first texture
	bool mixed = theGameConfiguration->mixTexFlats();
	rgba_t(255, 255, 255, 255*alpha, 0).set_gl();
	Drawing::drawTextureWithin(theMapEditor->textureManager().getFlat(textures[0], mixed), x, y, x + size, y + size, 0, 100);

	// Draw up to 4 subsequent textures (overlaid)
	rgba_t(255, 255, 255, 127*alpha, 0).set_gl();
	for (unsigned a = 1; a < textures.size() && a < 5; a++)
		Drawing::drawTextureWithin(theMapEditor->textureManager().getFlat(textures[a], mixed), x, y, x + size, y + size, 0, 100);

	glDisable(GL_TEXTURE_2D);

	// Draw outline
	if (hover) {
		rgba_t(col_sel.r, col_sel.g, col_sel.b, 255*alpha, 0).set_gl();
		glLineWidth(3.0f);
	}
	else {
		rgba_t(col_fg.r, col_fg.g, col_fg.b, 255*alpha, 0).set_gl();
		glLineWidth(1.5f);
	}
	Drawing::drawRect(x, y, x+size, y+size);
}

void SectorTextureOverlay::openSectors(vector<MapSector*>& list) {
	// Clear current sectors list (if any)
	sectors.clear();
	tex_ceil.clear();
	tex_floor.clear();

	// Add list to sectors
	for (unsigned a = 0; a < list.size(); a++) {
		// Add sector
		sectors.push_back(list[a]);

		// Get textures
		string ftex = list[a]->stringProperty("texturefloor");
		string ctex = list[a]->stringProperty("textureceiling");

		// Add floor texture if different
		bool exists = false;
		for (unsigned t = 0; t < tex_floor.size(); t++) {
			if (tex_floor[t] == ftex) {
				exists = true;
				break;
			}
		}
		if (!exists)
			tex_floor.push_back(ftex);

		// Add ceiling texture if different
		exists = false;
		for (unsigned t = 0; t < tex_ceil.size(); t++) {
			if (tex_ceil[t] == ctex) {
				exists = true;
				break;
			}
		}
		if (!exists)
			tex_ceil.push_back(ctex);
	}
}

void SectorTextureOverlay::close(bool cancel) {
	// Deactivate
	active = false;

	// Set textures if not cancelled
	if (!cancel) {
		for (unsigned a = 0; a < sectors.size(); a++) {
			if (tex_floor.size() == 1)
				sectors[a]->setStringProperty("texturefloor", tex_floor[0]);
			if (tex_ceil.size() == 1)
				sectors[a]->setStringProperty("textureceiling", tex_ceil[0]);
		}
	}
}

void SectorTextureOverlay::mouseMotion(int x, int y) {
	// Check if the mouse is over the floor texture
	if (x >= middlex - border - tex_size &&
		x <= middlex - border &&
		y >= middley - tex_size*0.5 &&
		y <= middley + tex_size*0.5)
		hover_floor = true;
	else
		hover_floor = false;

	// Check if the mouse is over the ceiling texture
	if (x >= middlex + border &&
		x <= middlex + border + tex_size &&
		y >= middley - tex_size*0.5 &&
		y <= middley + tex_size*0.5)
		hover_ceil = true;
	else
		hover_ceil = false;
}

void SectorTextureOverlay::mouseLeftClick() {
	// Do nothing if no sectors open
	if (sectors.size() == 0)
		return;

	// Left clicked on floor texture
	if (hover_floor)
		browseFloorTexture();

	// Left clicked on ceiling texture
	else if (hover_ceil)
		browseCeilingTexture();
}

void SectorTextureOverlay::mouseRightClick() {
}

void SectorTextureOverlay::keyDown(string key) {
	// Browse floor texture
	if (key == "F" || key == "f")
		browseFloorTexture();
	
	// Browse ceiling texture
	if (key == "C" || key == "c")
		browseCeilingTexture();
}

void SectorTextureOverlay::browseFloorTexture() {
	// Get initial texture
	string texture;
	if (tex_floor.size() == 0)
		texture = sectors[0]->stringProperty("texturefloor");
	else
		texture = tex_floor[0];

	// Open texture browser
	MapTextureBrowser browser(theMapEditor, 1, texture);
	browser.SetTitle("Browse Floor Texture");
	if (browser.ShowModal() == wxID_OK) {
		// Set texture
		tex_floor.clear();
		tex_floor.push_back(browser.getSelectedItem()->getName());
		close(false);
	}
}

void SectorTextureOverlay::browseCeilingTexture() {
	// Get initial texture
	string texture;
	if (tex_ceil.size() == 0)
		texture = sectors[0]->stringProperty("textureceiling");
	else
		texture = tex_ceil[0];

	// Open texture browser
	MapTextureBrowser browser(theMapEditor, 1, texture);
	browser.SetTitle("Browse Ceiling Texture");
	if (browser.ShowModal() == wxID_OK) {
		// Set texture
		tex_ceil.clear();
		tex_ceil.push_back(browser.getSelectedItem()->getName());
		close(false);
	}
}
