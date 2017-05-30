
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    QuickTextureOverlay.cpp
 * Description: QuickTextureOverlay class - A full screen map editor
 *              overlay that shows a scrollable list of textures
 *              in 3d mode. The currently selected texture is applied
 *              in real-time to the currently selected walls/flats,
 *              giving a quick 'preview' of how it would look
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "UI/WxStuff.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "QuickTextureOverlay3d.h"
#include "OpenGL/Drawing.h"
#include "General/ResourceManager.h"
#include "OpenGL/GLTexture.h"
#include "MapEditor/MapEditor.h"
#include "Game/Configuration.h"
#include "OpenGL/OpenGL.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapTextureManager.h"


/*******************************************************************
 * QUICKTEXTUREOVERLAY3D CLASS FUNCTIONS
 *******************************************************************/

/* QuickTextureOverlay3d::QuickTextureOverlay3d
 * QuickTextureOverlay3d class constructor
 *******************************************************************/
QuickTextureOverlay3d::QuickTextureOverlay3d(MapEditContext* editor)
{
	// Init variables
	allow_3d_mlook = true;
	current_index = 0;
	anim_offset = 0;
	this->editor = editor;

	if (editor)
	{
		auto& sel = editor->selection();

		if (!ok(sel))
		{
			active = false;
			return;
		}

		// Determine texture type
		sel_type = 2;
		int initial = 0;
		if (!Game::configuration().featureSupported(Game::Feature::MixTexFlats))
		{
			sel_type = 0;
			for (unsigned a = 0; a < sel.size(); a++)
			{
				if (sel[a].type != MapEditor::ItemType::Thing &&
					sel[a].type != MapEditor::ItemType::Ceiling &&
					sel[a].type != MapEditor::ItemType::Floor)
				{
					sel_type = 1;
					initial = a;
					break;
				}
			}
		}

		// Get initial texture
		string tex_init;
		if (sel[initial].type == MapEditor::ItemType::Ceiling)
			tex_init = editor->map().getSector(sel[initial].index)->stringProperty("textureceiling");
		else if (sel[initial].type == MapEditor::ItemType::Floor)
			tex_init = editor->map().getSector(sel[initial].index)->stringProperty("texturefloor");
		else if (sel[initial].type == MapEditor::ItemType::WallTop)
			tex_init = editor->map().getSide(sel[initial].index)->stringProperty("texturetop");
		else if (sel[initial].type == MapEditor::ItemType::WallMiddle)
			tex_init = editor->map().getSide(sel[initial].index)->stringProperty("texturemiddle");
		else if (sel[initial].type == MapEditor::ItemType::WallBottom)
			tex_init = editor->map().getSide(sel[initial].index)->stringProperty("texturebottom");

		// Get all available texture names (sorted alphabetically)
		vector<string> tex_names;
		if (sel_type > 0)
		{
			vector<map_texinfo_t>& ti = MapEditor::textureManager().getAllTexturesInfo();
			for (unsigned a = 0; a < ti.size(); a++)
				tex_names.push_back(ti[a].name);
		}
		if (sel_type == 0 || sel_type == 2)
		{
			vector<map_texinfo_t>& ti = MapEditor::textureManager().getAllFlatsInfo();
			for (unsigned a = 0; a < ti.size(); a++)
				tex_names.push_back(ti[a].name);
		}
		std::sort(tex_names.begin(), tex_names.end());

		// Init textures
		for (unsigned a = 0; a < tex_names.size(); a++)
			textures.push_back(qt_tex_t(tex_names[a]));

		// Set initial texture
		setTexture(tex_init);

		// Begin recording undo step
		editor->beginUndoRecord("Quick Texture", true, false, false);
	}
}

/* QuickTextureOverlay3d::~QuickTextureOverlay3d
 * QuickTextureOverlay3d class destructor
 *******************************************************************/
QuickTextureOverlay3d::~QuickTextureOverlay3d()
{
}

/* QuickTextureOverlay3d::setTexture
 * Sets the currentl texture to [name], if it exists
 *******************************************************************/
void QuickTextureOverlay3d::setTexture(string name)
{
	for (unsigned a = 0; a < textures.size(); a++)
	{
		if (S_CMPNOCASE(textures[a].name, name))
		{
			current_index = a;
			anim_offset = a;
			return;
		}
	}
}

/* QuickTextureOverlay3d::applyTexture
 * Applies the current texture to all selected walls/flats
 *******************************************************************/
void QuickTextureOverlay3d::applyTexture()
{
	// Check editor is associated
	if (!editor)
		return;

	// Get selection/hilight
	auto& selection = editor->selection();

	// Go through items
	if (!selection.empty())
	{
		for (unsigned a = 0; a < selection.size(); a++)
		{
			// Thing (skip)
			if (selection[a].type == MapEditor::ItemType::Thing)
				continue;

			// Floor
			else if (selection[a].type == MapEditor::ItemType::Floor && (sel_type == 0 || sel_type == 2))
			{
				MapSector* sector = editor->map().getSector(selection[a].index);
				if (sector) sector->setStringProperty("texturefloor", textures[current_index].name);
			}

			// Ceiling
			else if (selection[a].type == MapEditor::ItemType::Ceiling && (sel_type == 0 || sel_type == 2))
			{
				MapSector* sector = editor->map().getSector(selection[a].index);
				if (sector) sector->setStringProperty("textureceiling", textures[current_index].name);
			}

			// Wall
			else if (sel_type > 0)
			{
				MapSide* side = editor->map().getSide(selection[a].index);
				if (side)
				{
					// Upper
					if (selection[a].type == MapEditor::ItemType::WallTop)
						side->setStringProperty("texturetop", textures[current_index].name);
					// Middle
					else if (selection[a].type == MapEditor::ItemType::WallMiddle)
						side->setStringProperty("texturemiddle", textures[current_index].name);
					// Lower
					else if (selection[a].type == MapEditor::ItemType::WallBottom)
						side->setStringProperty("texturebottom", textures[current_index].name);
				}
			}
		}
	}
}

/* QuickTextureOverlay3d::update
 * Updates the overlay (animations, etc.)
 *******************************************************************/
void QuickTextureOverlay3d::update(long frametime)
{
	double target = current_index;
	float mult = (float)frametime / 10.0f;
	if (anim_offset < target - 0.01)
		anim_offset += (target - anim_offset) * (0.2 * mult);
	else if (anim_offset > target + 0.01)
		anim_offset -= (anim_offset - target) * (0.2 * mult);
	else
		anim_offset = current_index;
}

/* QuickTextureOverlay3d::draw
 * Draws the overlay to [width],[height]
 *******************************************************************/
void QuickTextureOverlay3d::draw(int width, int height, float fade)
{
	// Don't draw if invisible
	if (fade < 0.001f)
		return;

	// Draw background
	glColor4f(0.0f, 0.0f, 0.0f, fade * 0.6f);
	glDisable(GL_TEXTURE_2D);
	Drawing::drawFilledRect(0, height - 120, width, height);

	// Draw current tex name
	glEnable(GL_TEXTURE_2D);

	// Draw textures
	double x = ((double)width * 0.5) - (anim_offset * 136.0);
	glColor4f(1.0f, 1.0f, 1.0f, fade);
	for (unsigned a = 0; a < textures.size(); a++)
	{
		// Skip until first texture to show on left
		if (x < -96)
		{
			x += 136;
			continue;
		}

		// Draw texture
		drawTexture(a, x, height - 18, determineSize(x, width), fade);
		x += 136;

		// Stop when we get to the right edge of the screen
		if (x > width + 96)
			break;
	}
}

/* QuickTextureOverlay3d::drawTexture
 * Draws the texture in the list at [index]
 *******************************************************************/
void QuickTextureOverlay3d::drawTexture(unsigned index, double x, double bottom, double size, float fade)
{
	// Get texture if needed
	if (!textures[index].texture)
	{
		if (sel_type == 1)
			textures[index].texture = MapEditor::textureManager().getTexture(textures[index].name, false);
		else if (sel_type == 0)
			textures[index].texture = MapEditor::textureManager().getFlat(textures[index].name, false);
		else
			textures[index].texture = MapEditor::textureManager().getTexture(textures[index].name, true);
	}

	// Draw name
	double brightness = 0.5 + (size - 1.0);
	Drawing::drawText(textures[index].name, x, bottom + 2, rgba_t(brightness*255, brightness*255, brightness*255, brightness*255*fade),
	                  index == current_index ? Drawing::FONT_BOLD : Drawing::FONT_NORMAL, Drawing::ALIGN_CENTER);

	// Draw texture
	frect_t rect = Drawing::fitTextureWithin(textures[index].texture, x - 48*size, bottom - (96*size), x + 48*size, bottom, 0, 2);
	glColor4f(brightness, brightness, brightness, brightness*fade);
	textures[index].texture->bind();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);	glVertex2d(rect.x1(), rect.y1());
	glTexCoord2f(0.0f, 1.0f);	glVertex2d(rect.x1(), rect.y2());
	glTexCoord2f(1.0f, 1.0f);	glVertex2d(rect.x2(), rect.y2());
	glTexCoord2f(1.0f, 0.0f);	glVertex2d(rect.x2(), rect.y1());
	glEnd();
}

/* QuickTextureOverlay3d::determineSize
 * Calculates the size to draw a texture of [width] at position [x]
 * (towards the middle of the screen is larger)
 *******************************************************************/
double QuickTextureOverlay3d::determineSize(double x, int width)
{
	double mid = (double)width * 0.5;
	if (x < mid - 384 || x > mid + 384)
		return 1.0;

	double diff = (x < mid) ? mid - x : x - mid;
	double mult = ((384 - diff) / 384);
	return 1 + (0.5 * mult * mult);
}

/* QuickTextureOverlay3d::close
 * Called when the user closes the overlay. Applies changes if
 * [cancel] is false
 *******************************************************************/
void QuickTextureOverlay3d::close(bool cancel)
{
	if (editor)
	{
		editor->endUndoRecord(true);
		editor->selection().lockHilight(false);
		if (cancel)
			editor->doUndo();
	}

	active = false;
}

/* QuickTextureOverlay3d::mouseMotion
 * Called when the mouse cursor is moved
 *******************************************************************/
void QuickTextureOverlay3d::mouseMotion(int x, int y)
{
}

/* QuickTextureOverlay3d::mouseLeftClick
 * Called when the left mouse button is clicked
 *******************************************************************/
void QuickTextureOverlay3d::mouseLeftClick()
{
}

/* QuickTextureOverlay3d::mouseRightClick
 * Called when the right mouse button is clicked
 *******************************************************************/
void QuickTextureOverlay3d::mouseRightClick()
{
}

/* QuickTextureOverlay3d::doSearch
 * Finds and selects the first texture matching the current search
 *******************************************************************/
void QuickTextureOverlay3d::doSearch()
{
	if (!search.empty())
	{
		for (unsigned a = 0; a < textures.size(); a++)
		{
			if (textures[a].name.Lower().StartsWith(search.Lower()))
			{
				current_index = a;
				applyTexture();
				return;
			}
		}
	}
}

/* QuickTextureOverlay3d::keyDown
 * Called when a key is pressed
 *******************************************************************/
void QuickTextureOverlay3d::keyDown(string key)
{
	// Up texture
	if ((key == "right" || key == "mwheeldown") && current_index < textures.size() - 1)
	{
		current_index++;
		search = "";
		applyTexture();
	}

	// Down texture
	else if ((key == "left" || key == "mwheelup") && current_index > 0)
	{
		current_index--;
		search = "";
		applyTexture();
	}

	// Character (search)
	else if (key.length() == 1)
	{
		search += key;
		doSearch();
	}
}

/* QuickTextureOverlay3d::ok
 * Returns true if [sel] is valid for quick texture selection
 *******************************************************************/
bool QuickTextureOverlay3d::ok(const ItemSelection& sel)
{
	// Cancel if no selection
	if (sel.empty())
		return false;

	// Cancel if only things selected
	bool ok = false;
	for (unsigned a = 0; a < sel.size(); a++)
	{
		if (sel[a].type != MapEditor::ItemType::Thing)
		{
			ok = true;
			break;
		}
	}
	return ok;
}
