
#include "Main.h"
#include "WxStuff.h"
#include "MapEditorWindow.h"
#include "MapEditor.h"
#include "InfoOverlay3d.h"
#include "SLADEMap.h"
#include "ColourConfiguration.h"
#include "Drawing.h"
#include "OpenGL.h"

EXTERN_CVAR(Bool, use_zeth_icons)

InfoOverlay3D::InfoOverlay3D()
{
}

InfoOverlay3D::~InfoOverlay3D()
{
}

void InfoOverlay3D::update(int item_index, int item_type, SLADEMap* map)
{
	// Clear current info
	info.clear();
	info2.clear();

	// Setup variables
	current_type = item_type;
	texname = "";
	texture = NULL;
	thing_icon = false;
	int map_format = theMapEditor->currentMapDesc().format;

	// Wall
	if (item_type == MapEditor::SEL_SIDE_BOTTOM || item_type == MapEditor::SEL_SIDE_MIDDLE || item_type == MapEditor::SEL_SIDE_TOP)
	{
		// Get line and side
		MapSide* side = map->getSide(item_index);
		if (!side) return;
		MapLine* line = side->getParentLine();
		if (!line) return;
		object = side;

		// --- Line/side info ---
		info.push_back(S_FMT("Line #%d", line->getIndex()));
		if (side == line->s1())
			info.push_back(S_FMT("Front Side #%d", side->getIndex()));
		else
			info.push_back(S_FMT("Back Side #%d", side->getIndex()));

		// Relevant flags
		string flags = "";
		if (theGameConfiguration->lineBasicFlagSet("dontpegtop", line, map_format))
			flags += "Upper Unpegged, ";
		if (theGameConfiguration->lineBasicFlagSet("dontpegbottom", line, map_format))
			flags += "Lower Unpegged, ";
		if (theGameConfiguration->lineBasicFlagSet("blocking", line, map_format))
			flags += "Blocking, ";
		if (!flags.IsEmpty())
		{
			flags.RemoveLast(2);
			info.push_back(flags);
		}

		// Other potential info: special, sector#, length


		// --- Wall part info ---

		// Part
		if (item_type == MapEditor::SEL_SIDE_BOTTOM)
			info2.push_back("Lower Texture");
		else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
			info2.push_back("Middle Texture");
		else
			info2.push_back("Upper Texture");

		// Offsets
		if (theGameConfiguration->udmfNamespace() == "zdoom")
		{
			// Get x offset info
			int xoff = side->intProperty("offsetx");
			double xoff_part = 0;
			if (item_type == MapEditor::SEL_SIDE_BOTTOM)
				xoff_part = side->floatProperty("offsetx_bottom");
			else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
				xoff_part = side->floatProperty("offsetx_mid");
			else
				xoff_part = side->floatProperty("offsetx_top");

			// Add x offset string
			if (xoff_part == 0)
				info2.push_back(S_FMT("X Offset: %d", xoff));
			else if (xoff_part > 0)
				info2.push_back(S_FMT("X Offset: %1.2f (%d+%1.2f)", (double)xoff+xoff_part, xoff, xoff_part));
			else
				info2.push_back(S_FMT("X Offset: %1.2f (%d-%1.2f)", (double)xoff+xoff_part, xoff, -xoff_part));

			// Get y offset info
			int yoff = side->intProperty("offsety");
			double yoff_part = 0;
			if (item_type == MapEditor::SEL_SIDE_BOTTOM)
				yoff_part = side->floatProperty("offsety_bottom");
			else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
				yoff_part = side->floatProperty("offsety_mid");
			else
				yoff_part = side->floatProperty("offsety_top");

			// Add y offset string
			if (yoff_part == 0)
				info2.push_back(S_FMT("Y Offset: %d", yoff));
			else if (yoff_part > 0)
				info2.push_back(S_FMT("Y Offset: %1.2f (%d+%1.2f)", (double)yoff+yoff_part, yoff, yoff_part));
			else
				info2.push_back(S_FMT("Y Offset: %1.2f (%d-%1.2f)", (double)yoff+yoff_part, yoff, -yoff_part));
		}
		else
		{
			// Basic offsets
			info2.push_back(S_FMT("X Offset: %d", side->intProperty("offsetx")));
			info2.push_back(S_FMT("Y Offset: %d", side->intProperty("offsety")));
		}

		// ZDoom UDMF extras
		if (theGameConfiguration->udmfNamespace() == "zdoom")
		{
			// Scale
			double xscale, yscale;
			if (item_type == MapEditor::SEL_SIDE_BOTTOM)
			{
				xscale = side->floatProperty("scalex_bottom");
				yscale = side->floatProperty("scaley_bottom");
			}
			else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
			{
				xscale = side->floatProperty("scalex_mid");
				yscale = side->floatProperty("scaley_mid");
			}
			else
			{
				xscale = side->floatProperty("scalex_top");
				yscale = side->floatProperty("scaley_top");
			}
			info2.push_back(S_FMT("Scale: %1.2fx, %1.2fx", xscale, yscale));
		}

		// Texture
		if (item_type == MapEditor::SEL_SIDE_BOTTOM)
			texname = side->getTexLower();
		else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
			texname = side->getTexMiddle();
		else
			texname = side->getTexUpper();
		texture = theMapEditor->textureManager().getTexture(texname, theGameConfiguration->mixTexFlats());
	}


	// Floor
	else if (item_type == MapEditor::SEL_FLOOR || item_type == MapEditor::SEL_CEILING)
	{
		// Get sector
		MapSector* sector = map->getSector(item_index);
		if (!sector) return;
		object = sector;

		// Get basic info
		int fheight = sector->intProperty("heightfloor");
		int cheight = sector->intProperty("heightceiling");

		// --- Sector info ---

		// Sector index
		info.push_back(S_FMT("Sector #%d", item_index));

		// Sector height
		info.push_back(S_FMT("Total Height: %d", cheight - fheight));

		// ZDoom UDMF extras
		/*
		if (theGameConfiguration->udmfNamespace() == "zdoom") {
			// Sector colour
			rgba_t col = sector->getColour(0, true);
			info.push_back(S_FMT("Colour: R%d, G%d, B%d", col.r, col.g, col.b));
		}
		*/


		// --- Flat info ---

		// Height
		if (item_type == MapEditor::SEL_FLOOR)
			info2.push_back(S_FMT("Floor Height: %d", fheight));
		else
			info2.push_back(S_FMT("Ceiling Height: %d", cheight));

		// Light
		int light = sector->intProperty("lightlevel");
		if (theGameConfiguration->udmfNamespace() == "zdoom")
		{
			// Get extra light info
			int fl = 0;
			bool abs = false;
			if (item_type == MapEditor::SEL_FLOOR)
			{
				fl = sector->intProperty("lightfloor");
				abs = sector->boolProperty("lightfloorabsolute");
			}
			else
			{
				fl = sector->intProperty("lightceiling");
				abs = sector->boolProperty("lightceilingabsolute");
			}

			// Set if absolute
			if (abs)
			{
				light = fl;
				fl = 0;
			}

			// Add info string
			if (fl == 0)
				info2.push_back(S_FMT("Light: %d", light));
			else if (fl > 0)
				info2.push_back(S_FMT("Light: %d (%d+%d)", light+fl, light, fl));
			else
				info2.push_back(S_FMT("Light: %d (%d-%d)", light+fl, light, -fl));
		}
		else
			info2.push_back(S_FMT("Light: %d", light));

		// ZDoom UDMF extras
		if (theGameConfiguration->udmfNamespace() == "zdoom")
		{
			// Offsets
			double xoff, yoff;
			if (item_type == MapEditor::SEL_FLOOR)
			{
				xoff = sector->floatProperty("xpanningfloor");
				yoff = sector->floatProperty("ypanningfloor");
			}
			else
			{
				xoff = sector->floatProperty("xpanningceiling");
				yoff = sector->floatProperty("ypanningceiling");
			}
			info2.push_back(S_FMT("Offsets: %1.2f, %1.2f", xoff, yoff));

			// Scaling
			double xscale, yscale;
			if (item_type == MapEditor::SEL_FLOOR)
			{
				xscale = sector->floatProperty("xscalefloor");
				yscale = sector->floatProperty("yscalefloor");
			}
			else
			{
				xscale = sector->floatProperty("xscaleceiling");
				yscale = sector->floatProperty("yscaleceiling");
			}
			info2.push_back(S_FMT("Scale: %1.2fx, %1.2fx", xscale, yscale));
		}

		// Texture
		if (item_type == MapEditor::SEL_FLOOR)
			texname = sector->getFloorTex();
		else
			texname = sector->getCeilingTex();
		texture = theMapEditor->textureManager().getFlat(texname, theGameConfiguration->mixTexFlats());
	}

	// Thing
	else if (item_type == MapEditor::SEL_THING)
	{
		// index, type, position, sector, zpos, height?, radius?

		// Get thing
		MapThing* thing = map->getThing(item_index);
		if (!thing) return;
		object = thing;

		// Index
		info.push_back(S_FMT("Thing #%d", item_index));

		// Position
		if (theMapEditor->currentMapDesc().format == MAP_HEXEN || theMapEditor->currentMapDesc().format == MAP_UDMF)
			info.push_back(S_FMT("Position: %d, %d, %d", (int)thing->xPos(), (int)thing->yPos(), (int)thing->floatProperty("height")));
		else
			info.push_back(S_FMT("Position: %d, %d", (int)thing->xPos(), (int)thing->yPos()));


		// Type
		ThingType* tt = theGameConfiguration->thingType(thing->getType());
		if (tt->getName() == "Unknown")
			info2.push_back(S_FMT("Type: %d", thing->getType()));
		else
			info2.push_back(S_FMT("Type: %s", tt->getName()));

		// Args
		if (theMapEditor->currentMapDesc().format == MAP_HEXEN ||
		        (theMapEditor->currentMapDesc().format == MAP_UDMF && theGameConfiguration->getUDMFProperty("arg0", MOBJ_THING)))
		{
			// Get thing args
			int args[5];
			args[0] = thing->intProperty("arg0");
			args[1] = thing->intProperty("arg1");
			args[2] = thing->intProperty("arg2");
			args[3] = thing->intProperty("arg3");
			args[4] = thing->intProperty("arg4");
			string argstr = tt->getArgsString(args);

			if (argstr.IsEmpty())
				info2.push_back("No Args");
			else
				info2.push_back(argstr);
		}

		// Sector
		int sector = map->sectorAt(thing->xPos(), thing->yPos());
		if (sector >= 0)
			info2.push_back(S_FMT("In Sector #%d", sector));
		else
			info2.push_back("No Sector");


		// Texture
		texture = theMapEditor->textureManager().getSprite(tt->getSprite(), tt->getTranslation(), tt->getPalette());
		if (!texture)
		{
			if (use_zeth_icons && tt->getZeth() >= 0)
				texture = theMapEditor->textureManager().getEditorImage(S_FMT("zethicons/zeth%02d", tt->getZeth()));
			if (!texture)
				texture = theMapEditor->textureManager().getEditorImage(S_FMT("thing/%s", tt->getIcon()));
			thing_icon = true;
		}
		texname = "";
	}

	last_update = theApp->runTimer();
}

void InfoOverlay3D::draw(int bottom, int right, int middle, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Don't bother if no info
	if (info.size() == 0)
		return;

	// Update if needed
	if (object->modifiedTime() > last_update)
		update(object->getIndex(), current_type, object->getParentMap());	// object updated
	else if (object->getObjType() == MOBJ_SIDE && ((MapSide*)object)->getParentLine()->modifiedTime() > last_update)
		update(object->getIndex(), current_type, object->getParentMap());	// parent line updated

	// Init GL stuff
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Determine overlay height
	int nlines = MAX(info.size(), info2.size());
	if (nlines < 4) nlines = 4;
	int height = nlines * 16 + 4;

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

	// Draw overlay background
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Drawing::drawBorderedRect(0, bottom - height - 4, right, bottom + 2, col_bg, col_border);

	// Draw info text lines (left)
	int y = height;
	for (unsigned a = 0; a < info.size(); a++)
	{
		Drawing::drawText(info[a], middle - 44, bottom - y, col_fg, Drawing::FONT_CONDENSED, Drawing::ALIGN_RIGHT);
		y -= 16;
	}

	// Draw info text lines (right)
	y = height;
	for (unsigned a = 0; a < info2.size(); a++)
	{
		Drawing::drawText(info2[a], middle + 44, bottom - y, col_fg, Drawing::FONT_CONDENSED);
		y -= 16;
	}

	// Draw texture if any
	drawTexture(alpha, middle - 40, bottom);

	// Done
	glEnable(GL_LINE_SMOOTH);
}

void InfoOverlay3D::drawTexture(float alpha, int x, int y)
{
	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a = col_fg.a*alpha;

	// Check texture exists
	if (texture)
	{
		// Draw background
		glEnable(GL_TEXTURE_2D);
		OpenGL::setColour(255, 255, 255, 255*alpha, 0);
		glPushMatrix();
		glTranslated(x, y-96, 0);
		GLTexture::bgTex().draw2dTiled(80, 80);
		glPopMatrix();

		// Draw texture
		if (texture)
		{
			OpenGL::setColour(255, 255, 255, 255*alpha, 0);
			Drawing::drawTextureWithin(texture, x, y - 96, x + 80, y - 16, 0);
		}

		glDisable(GL_TEXTURE_2D);

		// Draw outline
		OpenGL::setColour(col_fg.r, col_fg.g, col_fg.b, 255*alpha, 0);
		glLineWidth(1.0f);
		glDisable(GL_LINE_SMOOTH);
		Drawing::drawRect(x, y-96, x+80, y-16);
	}

	// Draw texture name (even if texture is blank)
	Drawing::drawText(texname, x + 40, y - 16, col_fg, Drawing::FONT_CONDENSED, Drawing::ALIGN_CENTER);
}
