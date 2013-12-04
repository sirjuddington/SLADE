
#include "Main.h"
#include "MapTextureManager.h"
#include "ResourceManager.h"
#include "CTexture.h"
#include "MainWindow.h"
#include "ArchiveManager.h"
#include "MapEditorWindow.h"
#include "OpenGL.h"

CVAR(Int, map_tex_filter, 0, CVAR_SAVE)

MapTextureManager::MapTextureManager(Archive* archive)
{
	// Init variables
	this->archive = archive;
	editor_images_loaded = false;
	palette = new Palette8bit();

	// Listen to the various managers
	listenTo(theResourceManager);
	listenTo(theArchiveManager);
	listenTo(thePaletteChooser);
}

MapTextureManager::~MapTextureManager()
{
	delete palette;
}

Palette8bit* MapTextureManager::getResourcePalette()
{
	if (thePaletteChooser->globalSelected())
	{
		ArchiveEntry* entry = theResourceManager->getPaletteEntry("PLAYPAL", archive);

		if (!entry)
			return thePaletteChooser->getSelectedPalette();

		palette->loadMem(entry->getMCData());
		return palette;
	}
	else
		return thePaletteChooser->getSelectedPalette();
}

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
	Palette8bit* pal = getResourcePalette();

	// Look for stand-alone textures first
	ArchiveEntry* etex = theResourceManager->getTextureEntry(name, "hires", archive);
	int textypefound = TEXTYPE_HIRES;
	if (etex == NULL)
	{
		etex = theResourceManager->getTextureEntry(name, "textures", archive);
		textypefound = TEXTYPE_TEXTURE;
	}
	/*
	if (etex == NULL) {
		etex = theResourceManager->getTextureEntry(name, "flats", archive);
		textypefound = TEXTYPE_FLAT;
	}
	*/
	if (etex)
	{
		SImage image;
		// Get image format hint from type, if any
		if (Misc::loadImageFromEntry(&image, etex))
		{
			mtex.texture = new GLTexture(false);
			mtex.texture->setFilter(filter);
			mtex.texture->loadImage(&image, pal);
		}
	}

	// Try composite textures then
	CTexture* ctex = theResourceManager->getTexture(name, archive);
	if (ctex && (!mtex.texture || textypefound == TEXTYPE_FLAT))
	{
		textypefound = TEXTYPE_WALLTEXTURE;
		SImage image;
		if (ctex->toImage(image, archive, pal))
		{
			mtex.texture = new GLTexture(false);
			mtex.texture->setFilter(filter);
			mtex.texture->loadImage(&image, pal);
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
	Palette8bit* pal = getResourcePalette();
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
				mtex.texture->loadImage(&image, pal);
			}
		}
	}

	/*
	// Try composite textures then
	if (!mtex.texture) {
		CTexture* ctex = theResourceManager->getTexture(name, archive);
		if (ctex) {
			SImage image;
			if (ctex->toImage(image, archive, pal)) {
				mtex.texture = new GLTexture(false);
				mtex.texture->setFilter(filter);
				mtex.texture->loadImage(&image, pal);
			}
		}
	}
	*/

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
	Palette8bit* pal = getResourcePalette();
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
		if (ctex && ctex->toImage(image, archive, pal))
			found = true;
	}

	// We have a valid image either from an entry or a composite texture.
	if (found)
	{
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
			//wxLogMessage("Loading editor texture %s", CHR(name));
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

void MapTextureManager::refreshResources()
{
	// Just clear all cached textures
	textures.clear();
	flats.clear();
	sprites.clear();
	thePaletteChooser->setGlobalFromArchive(archive);
	theMapEditor->forceRefresh(true);
	//wxLogMessage("texture manager cleared");
}

void MapTextureManager::setArchive(Archive* archive)
{
	this->archive = archive;
	refreshResources();
}

void MapTextureManager::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	// Only interested in the resource manager,
	// archive manager and palette chooser.
	if (announcer != theResourceManager
	        && announcer != thePaletteChooser
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
			theMapEditor->Hide();
			theMapEditor->mapEditor().clearMap();
			archive = NULL;
		}
	}

	// If the resources have been updated
	if (event_name == "resources_updated")
		refreshResources();

	if (event_name == "main_palette_changed")
		refreshResources();
}
