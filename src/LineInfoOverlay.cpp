
#include "Main.h"
#include "WxStuff.h"
#include "LineInfoOverlay.h"
#include "MapLine.h"
#include "MapSide.h"
#include "MapSector.h"
#include "Drawing.h"
#include "MathStuff.h"
#include "MapEditorWindow.h"
#include "ColourConfiguration.h"
#include "GameConfiguration.h"

LineInfoOverlay::LineInfoOverlay()
{
}

LineInfoOverlay::~LineInfoOverlay()
{
}

void LineInfoOverlay::update(MapLine* line)
{
	if (!line)
		return;

	info.clear();
	int map_format = theMapEditor->currentMapDesc().format;

	// General line info
	if (Global::debug)
		info.push_back(S_FMT("Line #%d (%d)", line->getIndex(), line->getId()));
	else
		info.push_back(S_FMT("Line #%d", line->getIndex()));
	info.push_back(S_FMT("Length: %d", MathStuff::round(line->getLength())));

	// Line special
	int as_id = line->intProperty("special");
	if (line->props().propertyExists("macro"))
	{
		int macro = line->intProperty("macro");
		info.push_back(S_FMT("Macro: #%d", macro));
	}
	else
		info.push_back(S_FMT("Special: %d (%s)", as_id, theGameConfiguration->actionSpecialName(as_id)));

	// Line trigger
	if (map_format == MAP_HEXEN || map_format == MAP_UDMF)
		info.push_back(S_FMT("Trigger: %s", theGameConfiguration->spacTriggerString(line, map_format)));

	// Line args (or sector tag)
	if (map_format == MAP_HEXEN || map_format == MAP_UDMF)
	{
		int args[5];
		args[0] = line->intProperty("arg0");
		args[1] = line->intProperty("arg1");
		args[2] = line->intProperty("arg2");
		args[3] = line->intProperty("arg3");
		args[4] = line->intProperty("arg4");
		string argstr = theGameConfiguration->actionSpecial(as_id)->getArgsString(args);
		if (!argstr.IsEmpty())
			info.push_back(S_FMT("%s", argstr));
		else
			info.push_back("No Args");
	}
	else
		info.push_back(S_FMT("Sector Tag: %d", line->intProperty("arg0")));

	// Line flags
	if (map_format != MAP_UDMF)
		info.push_back(S_FMT("Flags: %s", theGameConfiguration->lineFlagsString(line)));

	// Front side
	MapSide* s = line->s1();
	if (s)
	{
		int xoff = s->intProperty("offsetx");
		int yoff = s->intProperty("offsety");
		side_front.exists = true;
		if (Global::debug)
			side_front.info = S_FMT("Front Side #%d (%d) (Sector %d)", s->getIndex(), s->getId(), s->getSector()->getIndex());
		else
			side_front.info = S_FMT("Front Side #%d (Sector %d)", s->getIndex(), s->getSector()->getIndex());
		side_front.offsets = S_FMT("Offsets: (%d, %d)", xoff, yoff);
		side_front.tex_upper = s->getTexUpper();
		side_front.tex_middle = s->getTexMiddle();
		side_front.tex_lower = s->getTexLower();
	}
	else side_front.exists = false;

	// Back side
	s = line->s2();
	if (s)
	{
		int xoff = s->intProperty("offsetx");
		int yoff = s->intProperty("offsety");
		side_back.exists = true;
		if (Global::debug)
			side_back.info = S_FMT("Back Side #%d (%d) (Sector %d)", s->getIndex(), s->getId(), s->getSector()->getIndex());
		else
			side_back.info = S_FMT("Back Side #%d (Sector %d)", s->getIndex(), s->getSector()->getIndex());
		side_back.offsets = S_FMT("Offsets: (%d, %d)", xoff, yoff);
		side_back.tex_upper = s->getTexUpper();
		side_back.tex_middle = s->getTexMiddle();
		side_back.tex_lower = s->getTexLower();
	}
	else side_back.exists = false;
}

void LineInfoOverlay::draw(int bottom, int right, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Init GL stuff
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Determine overlay height
	int height = info.size() * 16 + 4;

	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a = col_fg.a*alpha;
	col_bg.a = col_bg.a*alpha;
	rgba_t col_border(0, 0, 0, 140);

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	int bottom2 = bottom;
	bottom += height*alpha_inv*alpha_inv;

	// Determine widths
	int n_side_panels = 0;
	if (side_front.exists)
		n_side_panels++;
	if (side_back.exists)
		n_side_panels++;

	// Draw overlay background
	int main_panel_end = right - (n_side_panels*258);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(1.0f);
	Drawing::drawBorderedRect(0, bottom-height-4, main_panel_end, bottom+2, col_bg, col_border);

	// Draw info text lines
	int y = height;
	for (unsigned a = 0; a < info.size(); a++)
	{
		Drawing::drawText(info[a], 2, bottom - y, col_fg, Drawing::FONT_CONDENSED);
		y -= 16;
	}

	// Side info
	int x = right - 256;
	if (side_front.exists)
	{
		// Background
		glDisable(GL_TEXTURE_2D);
		Drawing::drawBorderedRect(x, bottom-height-4, x + 256, bottom+2, col_bg, col_border);

		drawSide(bottom-4, right, alpha, side_front, x);
		x -= 258;
	}
	if (side_back.exists)
	{
		// Background
		glDisable(GL_TEXTURE_2D);
		Drawing::drawBorderedRect(x, bottom-height-4, x + 256, bottom+2, col_bg, col_border);

		drawSide(bottom-4, right, alpha, side_back, x);
	}

	// Done
	glEnable(GL_LINE_SMOOTH);
}

void LineInfoOverlay::drawSide(int bottom, int right, float alpha, side_t& side, int xstart)
{
	// Get colours
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a = col_fg.a*alpha;

	// Index and sector index
	Drawing::drawText(side.info, xstart + 4, bottom - 32, col_fg, Drawing::FONT_CONDENSED);

	// Texture offsets
	Drawing::drawText(side.offsets, xstart + 4, bottom - 16, col_fg, Drawing::FONT_CONDENSED);

	// Textures
	drawTexture(alpha, xstart + 4, bottom - 32, side.tex_upper);
	drawTexture(alpha, xstart + 88, bottom - 32, side.tex_middle, "M");
	drawTexture(alpha, xstart + 92 + 80, bottom - 32, side.tex_lower, "L");
}

void LineInfoOverlay::drawTexture(float alpha, int x, int y, string texture, string pos)
{
	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a = col_fg.a*alpha;

	// Check texture isn't blank
	if (texture != "-" && !texture.IsEmpty())
	{
		// Draw background
		glEnable(GL_TEXTURE_2D);
		rgba_t(255, 255, 255, 255*alpha, 0).set_gl();
		glPushMatrix();
		glTranslated(x, y-96, 0);
		GLTexture::bgTex().draw2dTiled(80, 80);
		glPopMatrix();

		// Get texture
		GLTexture* tex = theMapEditor->textureManager().getTexture(texture, theGameConfiguration->mixTexFlats());

		// Draw texture
		if (tex)
		{
			rgba_t(255, 255, 255, 255*alpha, 0).set_gl();
			Drawing::drawTextureWithin(tex, x, y - 96, x + 80, y - 16, 0);
		}

		glDisable(GL_TEXTURE_2D);

		// Draw outline
		rgba_t(col_fg.r, col_fg.g, col_fg.b, 255*alpha, 0).set_gl();
		glDisable(GL_LINE_SMOOTH);
		Drawing::drawRect(x, y-96, x+80, y-16);
	}

	// Draw texture name (even if texture is blank)
	texture.Prepend(":");
	texture.Prepend(pos);
	Drawing::drawText(texture, x + 40, y - 16, col_fg, Drawing::FONT_CONDENSED, Drawing::ALIGN_CENTER);
}
