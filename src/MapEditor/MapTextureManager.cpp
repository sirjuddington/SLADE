
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapTextureManager.cpp
// Description: Handles and keeps track of all OpenGL textures for the map
//              editor - textures, thing sprites, etc.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "MapTextureManager.h"
#include "Archive/ArchiveManager.h"
#include "Game/Configuration.h"
#include "General/Misc.h"
#include "General/ResourceManager.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MapEditContext.h"
#include "MapEditor.h"
#include "OpenGL/OpenGL.h"
#include "UI/Controls/PaletteChooser.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, map_tex_filter, 0, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// MapTextureManager Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapTextureManager class constructor
// -----------------------------------------------------------------------------
MapTextureManager::MapTextureManager(Archive* archive)
{
	// Init variables
	this->archive_        = archive;
	editor_images_loaded_ = false;
	palette_              = new Palette();
}

// -----------------------------------------------------------------------------
// MapTextureManager class destructor
// -----------------------------------------------------------------------------
MapTextureManager::~MapTextureManager() {}

// -----------------------------------------------------------------------------
// Initialises the texture manager
// -----------------------------------------------------------------------------
void MapTextureManager::init()
{
	// Listen to the various managers
	listenTo(theResourceManager);
	listenTo(&App::archiveManager());
	listenTo(theMainWindow->paletteChooser());
	palette_ = resourcePalette();
}

// -----------------------------------------------------------------------------
// Returns the current resource palette
// (depending on open archives and palette toolbar selection)
// -----------------------------------------------------------------------------
Palette* MapTextureManager::resourcePalette()
{
	if (theMainWindow->paletteChooser()->globalSelected())
	{
		ArchiveEntry* entry = theResourceManager->getPaletteEntry("PLAYPAL", archive_);

		if (!entry)
			return theMainWindow->paletteChooser()->selectedPalette();

		palette_->loadMem(entry->data());
		return palette_;
	}
	else
		return theMainWindow->paletteChooser()->selectedPalette();
}

// -----------------------------------------------------------------------------
// Returns the texture matching [name], loading it from resources if necessary.
// If [mixed] is true, flats are also searched if no matching texture is found
// -----------------------------------------------------------------------------
GLTexture* MapTextureManager::texture(string name, bool mixed)
{
	// Get texture matching name
	Texture& mtex = textures_[name.Upper()];

	// Get desired filter type
	auto filter = GLTexture::Filter::Linear;
	if (map_tex_filter == 0)
		filter = GLTexture::Filter::NearestLinearMin;
	else if (map_tex_filter == 1)
		filter = GLTexture::Filter::Linear;
	else if (map_tex_filter == 2)
		filter = GLTexture::Filter::LinearMipmap;
	else if (map_tex_filter == 3)
		filter = GLTexture::Filter::NearestMipmap;

	// If the texture is loaded
	if (mtex.texture)
	{
		// If the texture filter matches the desired one, return it
		if (mtex.texture->filter() == filter)
			return mtex.texture;
		else
		{
			// Otherwise, reload the texture
			if (mtex.texture != &(GLTexture::missingTex()))
				delete mtex.texture;
			mtex.texture = nullptr;
		}
	}

	// Texture not found or unloaded, look for it
	// Palette8bit* pal = getResourcePalette();

	// Look for stand-alone textures first
	ArchiveEntry* etex         = theResourceManager->getTextureEntry(name, "hires", archive_);
	auto          textypefound = CTexture::Type::HiRes;
	if (etex == nullptr)
	{
		etex         = theResourceManager->getTextureEntry(name, "textures", archive_);
		textypefound = CTexture::Type::Texture;
	}
	if (etex)
	{
		SImage image;
		// Get image format hint from type, if any
		if (Misc::loadImageFromEntry(&image, etex))
		{
			mtex.texture = new GLTexture(false);
			mtex.texture->setFilter(filter);
			mtex.texture->loadImage(&image, palette_);

			// Handle hires texture scale
			if (textypefound == CTexture::Type::HiRes)
			{
				ArchiveEntry* ref = theResourceManager->getTextureEntry(name, "textures", archive_);
				if (ref)
				{
					SImage imgref;
					if (Misc::loadImageFromEntry(&imgref, ref))
					{
						int w, h, sw, sh;
						w  = image.width();
						h  = image.height();
						sw = imgref.width();
						sh = imgref.height();
						mtex.texture->setWorldPanning(true);
						mtex.texture->setScale((double)sw / (double)w, (double)sh / (double)h);
					}
				}
			}
		}
	}

	// Try composite textures then
	CTexture* ctex = theResourceManager->getTexture(name, archive_);
	if (ctex) // Composite textures take precedence over the textures directory
	{
		textypefound = CTexture::Type::WallTexture;
		SImage image;
		if (ctex->toImage(image, archive_, palette_, true))
		{
			mtex.texture = new GLTexture(false);
			mtex.texture->setFilter(filter);
			mtex.texture->loadImage(&image, palette_);
			double sx = ctex->scaleX();
			if (sx == 0)
				sx = 1.0;
			double sy = ctex->scaleY();
			if (sy == 0)
				sy = 1.0;
			mtex.texture->setWorldPanning(ctex->worldPanning());
			mtex.texture->setScale(1.0 / sx, 1.0 / sy);
		}
	}

	// Not found
	if (!mtex.texture)
	{
		// Try flats if mixed
		if (mixed)
			return flat(name, false);

		// Otherwise use missing texture
		else
			mtex.texture = &(GLTexture::missingTex());
	}

	return mtex.texture;
}

// -----------------------------------------------------------------------------
// Returns the flat matching [name], loading it from resources if necessary.
// If [mixed] is true, textures are also searched if no matching flat is found
// -----------------------------------------------------------------------------
GLTexture* MapTextureManager::flat(string name, bool mixed)
{
	// Get flat matching name
	Texture& mtex = flats_[name.Upper()];

	// Get desired filter type
	auto filter = GLTexture::Filter::Linear;
	if (map_tex_filter == 0)
		filter = GLTexture::Filter::NearestLinearMin;
	else if (map_tex_filter == 1)
		filter = GLTexture::Filter::Linear;
	else if (map_tex_filter == 2)
		filter = GLTexture::Filter::LinearMipmap;
	else if (map_tex_filter == 3)
		filter = GLTexture::Filter::NearestMipmap;

	// If the texture is loaded
	if (mtex.texture)
	{
		// If the texture filter matches the desired one, return it
		if (mtex.texture->filter() == filter)
			return mtex.texture;
		else
		{
			// Otherwise, reload the texture
			if (mtex.texture != &(GLTexture::missingTex()))
				delete mtex.texture;
			mtex.texture = nullptr;
		}
	}

	if (mixed)
	{
		CTexture* ctex = theResourceManager->getTexture(name, archive_);
		if (ctex && ctex->isExtended() && ctex->type() != "WallTexture")
		{
			SImage image;
			if (ctex->toImage(image, archive_, palette_, true))
			{
				mtex.texture = new GLTexture(false);
				mtex.texture->setFilter(filter);
				mtex.texture->loadImage(&image, palette_);
				double sx = ctex->scaleX();
				if (sx == 0)
					sx = 1.0;
				double sy = ctex->scaleY();
				if (sy == 0)
					sy = 1.0;
				mtex.texture->setScale(1.0 / sx, 1.0 / sy);
				mtex.texture->setWorldPanning(ctex->worldPanning());
				return mtex.texture;
			}
		}
	}

	// Flat not found, look for it
	// Palette8bit* pal = getResourcePalette();
	if (!mtex.texture)
	{
		ArchiveEntry* entry = theResourceManager->getTextureEntry(name, "hires", archive_);
		if (entry == nullptr)
			entry = theResourceManager->getTextureEntry(name, "flats", archive_);
		if (entry == nullptr)
			entry = theResourceManager->getFlatEntry(name, archive_);
		if (entry)
		{
			SImage image;
			if (Misc::loadImageFromEntry(&image, entry))
			{
				mtex.texture = new GLTexture(false);
				mtex.texture->setFilter(filter);
				mtex.texture->loadImage(&image, palette_);
			}
		}
	}

	// Not found
	if (!mtex.texture)
	{
		// Try textures if mixed
		if (mixed)
			return texture(name, false);

		// Otherwise use missing texture
		else
			mtex.texture = &(GLTexture::missingTex());
	}

	return mtex.texture;
}

// -----------------------------------------------------------------------------
// Returns the sprite matching [name], loading it from resources if necessary.
// Sprite name also supports wildcards (?)
// -----------------------------------------------------------------------------
GLTexture* MapTextureManager::sprite(string name, string translation, string palette)
{
	// Don't bother looking for nameless sprites
	if (name.IsEmpty())
		return nullptr;

	// Get sprite matching name
	string hashname = name.Upper();
	if (!translation.IsEmpty())
		hashname += translation.Lower();
	if (!palette.IsEmpty())
		hashname += palette.Upper();
	Texture& mtex = sprites_[hashname];

	// Get desired filter type
	auto filter = GLTexture::Filter::Linear;
	if (map_tex_filter == 0)
		filter = GLTexture::Filter::NearestLinearMin;
	else if (map_tex_filter == 1)
		filter = GLTexture::Filter::Linear;
	else if (map_tex_filter == 2)
		filter = GLTexture::Filter::Linear;
	else if (map_tex_filter == 3)
		filter = GLTexture::Filter::NearestMipmap;

	// If the texture is loaded
	if (mtex.texture)
	{
		// If the texture filter matches the desired one, return it
		if (mtex.texture->filter() == filter)
			return mtex.texture;
		else
		{
			// Otherwise, reload the texture
			delete mtex.texture;
			mtex.texture = nullptr;
		}
	}

	// Sprite not found, look for it
	bool   found  = false;
	bool   mirror = false;
	SImage image;
	// Palette8bit* pal = getResourcePalette();
	ArchiveEntry* entry = theResourceManager->getPatchEntry(name, "sprites", archive_);
	if (!entry)
		entry = theResourceManager->getPatchEntry(name, "", archive_);
	if (!entry && name.length() == 8)
	{
		string newname = name;
		newname[4]     = name[6];
		newname[5]     = name[7];
		newname[6]     = name[4];
		newname[7]     = name[5];
		entry          = theResourceManager->getPatchEntry(newname, "sprites", archive_);
		if (entry)
			mirror = true;
	}
	if (entry)
	{
		found = true;
		Misc::loadImageFromEntry(&image, entry);
	}
	else // Try composite textures then
	{
		CTexture* ctex = theResourceManager->getTexture(name, archive_);
		if (ctex && ctex->toImage(image, archive_, this->palette_, true))
			found = true;
	}

	// We have a valid image either from an entry or a composite texture.
	if (found)
	{
		Palette* pal = this->palette_;
		// Apply translation
		if (!translation.IsEmpty())
			image.applyTranslation(translation, pal, true);
		// Apply palette override
		if (!palette.IsEmpty())
		{
			ArchiveEntry* newpal = theResourceManager->getPaletteEntry(palette, archive_);
			if (newpal && newpal->size() == 768)
			{
				// Why is this needed?
				// Copying data in pal->loadMem shouldn't
				// change it in the original entry...
				// We shouldn't need to copy the data in a temporary place first.
				pal = image.palette();
				MemChunk mc;
				mc.importMem(newpal->rawData(), newpal->size());
				pal->loadMem(mc);
			}
		}
		// Apply mirroring
		if (mirror)
			image.mirror(false);
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
		GLTexture* stex = sprite(name + '0', translation, palette);
		if (!stex)
			stex = sprite(name + '1', translation, palette);
		if (stex)
			return stex;
		if (!stex && name.length() == 5)
		{
			for (char chr = 'A'; chr <= ']'; ++chr)
			{
				stex = sprite(name + '0' + chr + '0', translation, palette);
				if (stex)
					return stex;
				stex = sprite(name + '1' + chr + '1', translation, palette);
				if (stex)
					return stex;
			}
		}
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Detects offset hacks such as that used by the wall torch thing in Heretic.
// If the Y offset is noticeably larger than the sprite height, that means the
// thing is supposed to be rendered above its real position.
// -----------------------------------------------------------------------------
int MapTextureManager::verticalOffset(string name)
{
	// Don't bother looking for nameless sprites
	if (name.IsEmpty())
		return 0;

	// Get sprite matching name
	ArchiveEntry* entry = theResourceManager->getPatchEntry(name, "sprites", archive_);
	if (!entry)
		entry = theResourceManager->getPatchEntry(name, "", archive_);
	if (entry)
	{
		SImage image;
		Misc::loadImageFromEntry(&image, entry);
		int h = image.height();
		int o = image.offset().y;
		if (o > h)
			return o - h;
		else
			return 0;
	}

	return 0;
}

// -----------------------------------------------------------------------------
// Loads all editor images (thing icons, etc) from the program resource archive
// -----------------------------------------------------------------------------
void MapTextureManager::importEditorImages(MapTexHashMap& map, ArchiveTreeNode* dir, string path)
{
	SImage image;

	// Go through entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		ArchiveEntry* entry = dir->entryAt(a);

		// Load entry to image
		if (image.open(entry->data()))
		{
			// Create texture in hashmap
			string name = path + entry->name(true);
			LOG_MESSAGE(4, "Loading editor texture %s", name);
			Texture& mtex = map[name];
			mtex.texture  = new GLTexture(false);
			mtex.texture->setFilter(GLTexture::Filter::Mipmap);
			mtex.texture->loadImage(&image);
		}
	}

	// Go through subdirs
	for (unsigned a = 0; a < dir->nChildren(); a++)
	{
		ArchiveTreeNode* subdir = (ArchiveTreeNode*)dir->child(a);
		importEditorImages(map, subdir, path + subdir->name() + "/");
	}
}

// -----------------------------------------------------------------------------
// Returns the editor image matching [name]
// -----------------------------------------------------------------------------
GLTexture* MapTextureManager::editorImage(string name)
{
	if (!OpenGL::isInitialised())
		return nullptr;

	// Load thing image textures if they haven't already
	if (!editor_images_loaded_)
	{
		// Load all thing images to textures
		Archive*         slade_pk3 = App::archiveManager().programResourceArchive();
		ArchiveTreeNode* dir       = slade_pk3->dir("images");
		if (dir)
			importEditorImages(editor_images_, dir, "");

		editor_images_loaded_ = true;
	}

	return editor_images_[name].texture;
}

// -----------------------------------------------------------------------------
// Unloads all cached textures, flats and sprites
// -----------------------------------------------------------------------------
void MapTextureManager::refreshResources()
{
	// Just clear all cached textures
	textures_.clear();
	flats_.clear();
	sprites_.clear();
	theMainWindow->paletteChooser()->setGlobalFromArchive(archive_);
	MapEditor::forceRefresh(true);
	palette_ = resourcePalette();
	buildTexInfoList();
	// LOG_MESSAGE(1, "texture manager cleared");
}

// -----------------------------------------------------------------------------
// (Re)builds lists with information about all currently available resource
// textures and flats
// -----------------------------------------------------------------------------
void MapTextureManager::buildTexInfoList()
{
	// Clear
	tex_info_.clear();
	flat_info_.clear();

	// --- Textures ---

	// Composite textures
	vector<TextureResource::Texture*> textures;
	theResourceManager->putAllTextures(textures, App::archiveManager().baseResourceArchive());
	for (unsigned a = 0; a < textures.size(); a++)
	{
		CTexture* tex    = &textures[a]->tex;
		Archive*  parent = textures[a]->parent;

		// string shortName = tex->getName().Truncate(8);
		string longName = tex->name();
		string path     = longName.BeforeLast('/');

		if (tex->isExtended())
		{
			if (S_CMPNOCASE(tex->type(), "texture") || S_CMPNOCASE(tex->type(), "walltexture"))
				tex_info_.push_back(TexInfo(longName, Category::ZDTextures, parent, path, tex->index(), longName));
			else if (S_CMPNOCASE(tex->type(), "define"))
				tex_info_.push_back(TexInfo(longName, Category::HiRes, parent, path, tex->index(), longName));
			else if (S_CMPNOCASE(tex->type(), "flat"))
				flat_info_.push_back(TexInfo(longName, Category::ZDTextures, parent, path, tex->index(), longName));
			// Ignore graphics, patches and sprites
		}
		else
			tex_info_.push_back(TexInfo(longName, Category::TextureX, parent, path, tex->index() + 1, longName));
	}

	// Texture namespace patches (TX_)
	if (Game::configuration().featureSupported(Game::Feature::TxTextures))
	{
		vector<ArchiveEntry*> patches;
		theResourceManager->putAllPatchEntries(
			patches, nullptr, Game::configuration().featureSupported(Game::Feature::LongNames));
		for (unsigned a = 0; a < patches.size(); a++)
		{
			if (patches[a]->isInNamespace("textures") || patches[a]->isInNamespace("hires"))
			{
				// Determine texture path if it's in a pk3
				string longName  = patches[a]->path(true).Remove(0, 1);
				string shortName = patches[a]->name(true).Upper().Truncate(8);
				string path      = patches[a]->path(false);

				tex_info_.push_back(TexInfo(shortName, Category::Tx, patches[a]->parent(), path, 0, longName));
			}
		}
	}

	// Flats
	vector<ArchiveEntry*> flats;
	theResourceManager->putAllFlatEntries(
		flats, nullptr, Game::configuration().featureSupported(Game::Feature::LongNames));
	for (unsigned a = 0; a < flats.size(); a++)
	{
		ArchiveEntry* entry = flats[a];

		// Determine flat path if it's in a pk3
		string longName  = entry->path(true).Remove(0, 1);
		string shortName = entry->name(true).Upper().Truncate(8);
		string path      = entry->path(false);

		flat_info_.push_back(TexInfo(shortName, Category::None, flats[a]->parent(), path, 0, longName));
	}
}

// -----------------------------------------------------------------------------
// Sets the current archive to [archive], and refreshes all resources
// -----------------------------------------------------------------------------
void MapTextureManager::setArchive(Archive* archive)
{
	this->archive_ = archive;
	refreshResources();
}

// -----------------------------------------------------------------------------
// Handles announcements from any announcers listened to
// -----------------------------------------------------------------------------
void MapTextureManager::onAnnouncement(Announcer* announcer, const string& event_name, MemChunk& event_data)
{
	// Only interested in the resource manager,
	// archive manager and palette chooser.
	if (announcer != theResourceManager && announcer != theMainWindow->paletteChooser()
		&& announcer != &App::archiveManager())
		return;

	// If the map's archive is being closed,
	// we need to close the map editor
	if (event_name == "archive_closing")
	{
		event_data.seek(0, SEEK_SET);
		int32_t ac_index;
		event_data.read(&ac_index, 4);
		if (App::archiveManager().getArchive(ac_index) == archive_)
		{
			MapEditor::windowWx()->Hide();
			MapEditor::editContext().clearMap();
			archive_ = nullptr;
		}
	}

	// If the resources have been updated
	if (event_name == "resources_updated")
		refreshResources();

	if (event_name == "main_palette_changed")
		refreshResources();
}
