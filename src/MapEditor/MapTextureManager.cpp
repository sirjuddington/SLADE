
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapTextureManager.cpp
 * Description: Handles and keeps track of all OpenGL textures for
 *              the map editor - textures, thing sprites, etc.
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
#include "MapTextureManager.h"
#include "General/ResourceManager.h"
#include "Graphics/CTexture/CTexture.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "Archive/ArchiveManager.h"
#include "MapEditor.h"
#include "MapEditContext.h"
#include "OpenGL/OpenGL.h"
#include "Graphics/SImage/SImage.h"
#include "General/Misc.h"
#include "UI/PaletteChooser.h"
#include "GameConfiguration/GameConfiguration.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, map_tex_filter, 0, CVAR_SAVE)


/*******************************************************************
 * MAPTEXTUREMANAGER CLASS FUNCTIONS
 *******************************************************************/

/* MapTextureManager::MapTextureManager
 * MapTextureManager class constructor
 *******************************************************************/
MapTextureManager::MapTextureManager(Archive* archive)
{
	// Init variables
	this->archive = archive;
	editor_images_loaded = false;
	palette = new Palette8bit();
}

/* MapTextureManager::~MapTextureManager
 * MapTextureManager class destructor
 *******************************************************************/
MapTextureManager::~MapTextureManager()
{
}

/* MapTextureManager::init
 * Initialises the texture manager
 *******************************************************************/
void MapTextureManager::init()
{
	// Listen to the various managers
	listenTo(theResourceManager);
	listenTo(theArchiveManager);
	listenTo(theMainWindow->getPaletteChooser());
	palette = getResourcePalette();
}

/* MapTextureManager::getResourcePalette
 * Returns the current resource palette (depending on open archives
 * and palette toolbar selection)
 *******************************************************************/
Palette8bit* MapTextureManager::getResourcePalette()
{
	if (theMainWindow->getPaletteChooser()->globalSelected())
	{
		ArchiveEntry* entry = theResourceManager->getPaletteEntry("PLAYPAL", archive);

		if (!entry)
			return theMainWindow->getPaletteChooser()->getSelectedPalette();

		palette->loadMem(entry->getMCData());
		return palette;
	}
	else
		return theMainWindow->getPaletteChooser()->getSelectedPalette();
}

/* MapTextureManager::getTexture
 * Returns the texture matching [name]. Loads it from resources if
 * necessary. If [mixed] is true, flats are also searched if no
 * matching texture is found
 *******************************************************************/
GLTexture* MapTextureManager::getTexture(string name, bool mixed)
{
	// Get texture matching name
	map_tex_t& mtex = textures[name.Upper()];

	// Get desired filter type
	int filter = 1;
	if (map_tex_filter == 0)
		filter = GLTexture::NEAREST_LINEAR_MIN;
	else if (map_tex_filter == 1)
		filter = GLTexture::LINEAR;
	else if (map_tex_filter == 2)
		filter = GLTexture::LINEAR_MIPMAP;
	else if (map_tex_filter == 3)
		filter = GLTexture::NEAREST_MIPMAP;

	// If the texture is loaded
	if (mtex.texture)
	{
		// If the texture filter matches the desired one, return it
		if (mtex.texture->getFilter() == filter)
			return mtex.texture;
		else
		{
			// Otherwise, reload the texture
			if (mtex.texture != &(GLTexture::missingTex())) delete mtex.texture;
			mtex.texture = NULL;
		}
	}

	// Texture not found or unloaded, look for it
	//Palette8bit* pal = getResourcePalette();

	// Look for stand-alone textures first
	ArchiveEntry* etex = theResourceManager->getTextureEntry(name, "hires", archive);
	int textypefound = TEXTYPE_HIRES;
	if (etex == NULL)
	{
		etex = theResourceManager->getTextureEntry(name, "textures", archive);
		textypefound = TEXTYPE_TEXTURE;
	}
	if (etex)
	{
		SImage image;
		// Get image format hint from type, if any
		if (Misc::loadImageFromEntry(&image, etex))
		{
			mtex.texture = new GLTexture(false);
			mtex.texture->setFilter(filter);
			mtex.texture->loadImage(&image, palette);

			// Handle hires texture scale
			if (textypefound == TEXTYPE_HIRES)
			{
				ArchiveEntry* ref = theResourceManager->getTextureEntry(name, "textures", archive);
				if (ref)
				{
					SImage imgref;
					if (Misc::loadImageFromEntry(&imgref, ref))
					{
						int w, h, sw, sh;
						w = image.getWidth();
						h = image.getHeight();
						sw = imgref.getWidth();
						sh = imgref.getHeight();
						mtex.texture->setScale((double)sw/(double)w, (double)sh/(double)h);
					}
				}
			}
		}
	}

	// Try composite textures then
	CTexture* ctex = theResourceManager->getTexture(name, archive);
	if (ctex && (!mtex.texture || textypefound == TEXTYPE_FLAT))
	{
		textypefound = TEXTYPE_WALLTEXTURE;
		SImage image;
		if (ctex->toImage(image, archive, palette))
		{
			mtex.texture = new GLTexture(false);
			mtex.texture->setFilter(filter);
			mtex.texture->loadImage(&image, palette);
			double sx = ctex->getScaleX(); if (sx == 0) sx = 1.0;
			double sy = ctex->getScaleY(); if (sy == 0) sy = 1.0;
			mtex.texture->setScale(1.0/sx, 1.0/sy);
		}
	}

	// Not found
	if (!mtex.texture)
	{
		// Try flats if mixed
		if (mixed)
			return getFlat(name, false);

		// Otherwise use missing texture
		else
			mtex.texture = &(GLTexture::missingTex());
	}

	return mtex.texture;
}

/* MapTextureManager::getFlat
 * Returns the flat matching [name]. Loads it from resources if
 * necessary. If [mixed] is true, textures are also searched if no
 * matching flat is found
 *******************************************************************/
GLTexture* MapTextureManager::getFlat(string name, bool mixed)
{
	// Get flat matching name
	map_tex_t& mtex = flats[name.Upper()];

	// Get desired filter type
	int filter = 1;
	if (map_tex_filter == 0)
		filter = GLTexture::NEAREST_LINEAR_MIN;
	else if (map_tex_filter == 1)
		filter = GLTexture::LINEAR;
	else if (map_tex_filter == 2)
		filter = GLTexture::LINEAR_MIPMAP;
	else if (map_tex_filter == 3)
		filter = GLTexture::NEAREST_MIPMAP;

	// If the texture is loaded
	if (mtex.texture)
	{
		// If the texture filter matches the desired one, return it
		if (mtex.texture->getFilter() == filter)
			return mtex.texture;
		else
		{
			// Otherwise, reload the texture
			if (mtex.texture != &(GLTexture::missingTex())) delete mtex.texture;
			mtex.texture = NULL;
		}
	}

	// Flat not found, look for it
	//Palette8bit* pal = getResourcePalette();
	if (!mtex.texture)
	{
		ArchiveEntry* entry = theResourceManager->getTextureEntry(name, "hires", archive);
		if (entry == NULL)
			entry = theResourceManager->getTextureEntry(name, "flats", archive);
		if (entry == NULL)
			entry = theResourceManager->getFlatEntry(name, archive);
		if (entry)
		{
			SImage image;
			if (Misc::loadImageFromEntry(&image, entry))
			{
				mtex.texture = new GLTexture(false);
				mtex.texture->setFilter(filter);
				mtex.texture->loadImage(&image, palette);
			}
		}
	}

	// Not found
	if (!mtex.texture)
	{
		// Try textures if mixed
		if (mixed)
			return getTexture(name, false);

		// Otherwise use missing texture
		else
			mtex.texture = &(GLTexture::missingTex());
	}

	return mtex.texture;
}

/* MapTextureManager::getSprite
 * Returns the sprite matching [name]. Loads it from resources if
 * necessary. Sprite name also supports wildcards (?)
 *******************************************************************/
GLTexture* MapTextureManager::getSprite(string name, string translation, string palette)
{
	// Don't bother looking for nameless sprites
	if (name.IsEmpty())
		return NULL;

	// Get sprite matching name
	string hashname = name.Upper();
	if (!translation.IsEmpty())
		hashname += translation.Lower();
	if (!palette.IsEmpty())
		hashname += palette.Upper();
	map_tex_t& mtex = sprites[hashname];

	// Get desired filter type
	int filter = 1;
	if (map_tex_filter == 0)
		filter = GLTexture::NEAREST_LINEAR_MIN;
	else if (map_tex_filter == 1)
		filter = GLTexture::LINEAR;
	else if (map_tex_filter == 2)
		filter = GLTexture::LINEAR;
	else if (map_tex_filter == 3)
		filter = GLTexture::NEAREST_MIPMAP;

	// If the texture is loaded
	if (mtex.texture)
	{
		// If the texture filter matches the desired one, return it
		if (mtex.texture->getFilter() == filter)
			return mtex.texture;
		else
		{
			// Otherwise, reload the texture
			delete mtex.texture;
			mtex.texture = NULL;
		}
	}

	// Sprite not found, look for it
	bool found = false;
	bool mirror = false;
	SImage image;
	//Palette8bit* pal = getResourcePalette();
	ArchiveEntry* entry = theResourceManager->getPatchEntry(name, "sprites", archive);
	if (!entry) entry = theResourceManager->getPatchEntry(name, "", archive);
	if (!entry && name.length() == 8)
	{
		string newname = name;
		newname[4] = name[6]; newname[5] = name[7]; newname[6] = name[4]; newname[7] = name[5];
		entry = theResourceManager->getPatchEntry(newname, "sprites", archive);
		if (entry) mirror = true;
	}
	if (entry)
	{
		found = true;
		Misc::loadImageFromEntry(&image, entry);
	}
	else  	// Try composite textures then
	{
		CTexture* ctex = theResourceManager->getTexture(name, archive);
		if (ctex && ctex->toImage(image, archive, this->palette))
			found = true;
	}

	// We have a valid image either from an entry or a composite texture.
	if (found)
	{
		Palette8bit* pal = this->palette;
		// Apply translation
		if (!translation.IsEmpty()) image.applyTranslation(translation, pal);
		// Apply palette override
		if (!palette.IsEmpty())
		{
			ArchiveEntry* newpal = theResourceManager->getPaletteEntry(palette, archive);
			if (newpal && newpal->getSize() == 768)
			{
				// Why is this needed?
				// Copying data in pal->loadMem shouldn't
				// change it in the original entry...
				// We shouldn't need to copy the data in a temporary place first.
				pal = image.getPalette();
				MemChunk mc;
				mc.importMem(newpal->getData(), newpal->getSize());
				pal->loadMem(mc);
			}
		}
		// Apply mirroring
		if (mirror) image.mirror(false);
		// Turn into GL texture
		mtex.texture = new GLTexture(false);
		mtex.texture->setFilter(filter);
		mtex.texture->setTiling(false);
		mtex.texture->loadImage(&image, pal);
		return mtex.texture;
	}
	else if (name.EndsWith("?"))
	{
		name.RemoveLast(1);
		GLTexture* sprite = getSprite(name + '0', translation, palette);
		if (!sprite)
			sprite = getSprite(name + '1', translation, palette);
		if (sprite)
			return sprite;
		if (!sprite && name.length() == 5)
		{
			for (char chr = 'A'; chr <= ']'; ++chr)
			{
				sprite = getSprite(name + '0' + chr + '0', translation, palette);
				if (sprite) return sprite;
				sprite = getSprite(name + '1' + chr + '1', translation, palette);
				if (sprite) return sprite;
			}
		}
	}

	return NULL;
}

/* MapTextureManager::getVerticalOffset
 * Detects offset hacks such as that used by the wall torch thing in
 * Heretic (type 50). If the Y offset is noticeably larger than the
 * sprite height, that means the thing is supposed to be rendered
 * above its real position.
 *******************************************************************/
int MapTextureManager::getVerticalOffset(string name)
{
	// Don't bother looking for nameless sprites
	if (name.IsEmpty())
		return 0;

	// Get sprite matching name
	ArchiveEntry* entry = theResourceManager->getPatchEntry(name, "sprites", archive);
	if (!entry) entry = theResourceManager->getPatchEntry(name, "", archive);
	if (entry)
	{
		SImage image;
		Misc::loadImageFromEntry(&image, entry);
		int h = image.getHeight();
		int o = image.offset().y;
		if (o > h)
			return o - h;
		else
			return 0;
	}

	return 0;
}

/* MapTextureManager::importEditorImages
 * Loads all editor images (thing icons, etc) from the program
 * resource archive
 *******************************************************************/
void importEditorImages(MapTexHashMap& map, ArchiveTreeNode* dir, string path)
{
	SImage image;

	// Go through entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		ArchiveEntry* entry = dir->getEntry(a);

		// Load entry to image
		if (image.open(entry->getMCData()))
		{
			// Create texture in hashmap
			string name = path + entry->getName(true);
			LOG_MESSAGE(4, "Loading editor texture %s", name);
			map_tex_t& mtex = map[name];
			mtex.texture = new GLTexture(false);
			mtex.texture->setFilter(GLTexture::MIPMAP);
			mtex.texture->loadImage(&image);
		}
	}

	// Go through subdirs
	for (unsigned a = 0; a < dir->nChildren(); a++)
	{
		ArchiveTreeNode* subdir = (ArchiveTreeNode*)dir->getChild(a);
		importEditorImages(map, subdir, path + subdir->getName() + "/");
	}
}

/* MapTextureManager::getEditorImage
 * Returns the editor image matching [name]
 *******************************************************************/
GLTexture* MapTextureManager::getEditorImage(string name)
{
	if (!OpenGL::isInitialised())
		return NULL;

	// Load thing image textures if they haven't already
	if (!editor_images_loaded)
	{
		// Load all thing images to textures
		Archive* slade_pk3 = theArchiveManager->programResourceArchive();
		ArchiveTreeNode* dir = slade_pk3->getDir("images");
		if (dir)
			importEditorImages(editor_images, dir, "");

		editor_images_loaded = true;
	}

	return editor_images[name].texture;
}

/* MapTextureManager::refreshResources
 * Unloads all cached textures, flats and sprites
 *******************************************************************/
void MapTextureManager::refreshResources()
{
	// Just clear all cached textures
	textures.clear();
	flats.clear();
	sprites.clear();
	theMainWindow->getPaletteChooser()->setGlobalFromArchive(archive);
	MapEditor::forceRefresh(true);
	palette = getResourcePalette();
	buildTexInfoList();
	//LOG_MESSAGE(1, "texture manager cleared");
}

/* MapTextureManager::buildTexInfoList
 * (Re)builds lists with information about all currently available
 * resource textures and flats
 *******************************************************************/
void MapTextureManager::buildTexInfoList()
{
	// Clear
	tex_info.clear();
	flat_info.clear();

	// --- Textures ---

	// Composite textures
	vector<TextureResource::Texture*> textures;
	theResourceManager->getAllTextures(textures, theArchiveManager->baseResourceArchive());
	for (unsigned a = 0; a < textures.size(); a++)
	{
		CTexture * tex = &textures[a]->tex;
		Archive* parent = textures[a]->parent;
		if (tex->isExtended())
		{
			if (S_CMPNOCASE(tex->getType(), "texture") || S_CMPNOCASE(tex->getType(), "walltexture"))
				tex_info.push_back(map_texinfo_t(tex->getName(), TC_TEXTURES, parent));
			else if (S_CMPNOCASE(tex->getType(), "define"))
				tex_info.push_back(map_texinfo_t(tex->getName(), TC_HIRES, parent));
			else if (S_CMPNOCASE(tex->getType(), "flat"))
				flat_info.push_back(map_texinfo_t(tex->getName(), TC_TEXTURES, parent));
			// Ignore graphics, patches and sprites
		}
		else
			tex_info.push_back(map_texinfo_t(tex->getName(), TC_TEXTUREX, parent, "", tex->getIndex() + 1));
	}

	// Texture namespace patches (TX_)
	if (theGameConfiguration->txTextures())
	{
		vector<ArchiveEntry*> patches;
		theResourceManager->getAllPatchEntries(patches, NULL);
		for (unsigned a = 0; a < patches.size(); a++)
		{
			if (patches[a]->isInNamespace("textures") || patches[a]->isInNamespace("hires"))
			{
				// Determine texture path if it's in a pk3
				string path = patches[a]->getPath();
				if (path.StartsWith("/textures/"))
					path.Remove(0, 9);
				else if (path.StartsWith("/hires/"))
					path.Remove(0, 6);
				else
					path = "";

				tex_info.push_back(map_texinfo_t(patches[a]->getName(true), TC_TX, patches[a]->getParent(), path));
			}
		}
	}

	// Flats
	vector<ArchiveEntry*> flats;
	theResourceManager->getAllFlatEntries(flats, NULL);
	for (unsigned a = 0; a < flats.size(); a++)
	{
		ArchiveEntry* entry = flats[a];

		// Determine flat path if it's in a pk3
		string path = entry->getPath();
		if (path.StartsWith("/flats/") || path.StartsWith("/hires/"))
			path.Remove(0, 6);
		else
			path = "";

		flat_info.push_back(map_texinfo_t(entry->getName(true), TC_NONE, flats[a]->getParent(), path));
	}
}

/* MapTextureManager::setArchive
 * Sets the current archive to [archive], and refreshes all resources
 *******************************************************************/
void MapTextureManager::setArchive(Archive* archive)
{
	this->archive = archive;
	refreshResources();
}

/* MapTextureManager::onAnnouncement
 * Handles announcements from any announcers listened to
 *******************************************************************/
void MapTextureManager::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	// Only interested in the resource manager,
	// archive manager and palette chooser.
	if (announcer != theResourceManager
	        && announcer != theMainWindow->getPaletteChooser()
	        && announcer != theArchiveManager)
		return;

	// If the map's archive is being closed,
	// we need to close the map editor
	if (event_name == "archive_closing")
	{
		event_data.seek(0, SEEK_SET);
		int32_t ac_index;
		event_data.read(&ac_index, 4);
		if (theArchiveManager->getArchive(ac_index) == archive)
		{
			MapEditor::windowWx()->Hide();
			MapEditor::editContext().clearMap();
			archive = NULL;
		}
	}

	// If the resources have been updated
	if (event_name == "resources_updated")
		refreshResources();

	if (event_name == "main_palette_changed")
		refreshResources();
}
