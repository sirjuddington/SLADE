
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
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
#include "App.h"
#include "Archive/ArchiveEntry.h"
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
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
MapTextureManager::Texture tex_invalid;
}
CVAR(Int, map_tex_filter, 0, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// MapTextureManager Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapTextureManager class constructor
// -----------------------------------------------------------------------------
MapTextureManager::MapTextureManager(shared_ptr<Archive> archive) : archive_{ archive }, palette_{ new Palette() } {}

// -----------------------------------------------------------------------------
// Initialises the texture manager
// -----------------------------------------------------------------------------
void MapTextureManager::init()
{
	// Refresh when resources are updated or the main palette is changed
	sc_resources_updated_ = app::resources().signals().resources_updated.connect([this]() { refreshResources(); });
	sc_palette_changed_   = theMainWindow->paletteChooser()->signals().palette_changed.connect(
        [this]() { refreshResources(); });

	// Load palette
	if (auto pal = resourcePalette(); pal != palette_.get())
		palette_->copyPalette(pal);
}

// -----------------------------------------------------------------------------
// Returns the current resource palette
// (depending on open archives and palette toolbar selection)
// -----------------------------------------------------------------------------
Palette* MapTextureManager::resourcePalette() const
{
	if (theMainWindow->paletteChooser()->globalSelected())
	{
		auto entry = app::resources().getPaletteEntry("PLAYPAL", archive_.lock().get());

		if (!entry)
			return theMainWindow->paletteChooser()->selectedPalette();

		palette_->loadMem(entry->data());
		return palette_.get();
	}
	else
		return theMainWindow->paletteChooser()->selectedPalette();
}

// -----------------------------------------------------------------------------
// Returns the texture matching [name], loading it from resources if necessary.
// If [mixed] is true, flats are also searched if no matching texture is found
// -----------------------------------------------------------------------------
const MapTextureManager::Texture& MapTextureManager::texture(string_view name, bool mixed)
{
	// Get texture matching name
	auto& mtex = textures_[strutil::upper(name)];

	// Get desired filter type
	auto filter = gl::TexFilter::Linear;
	if (map_tex_filter == 0)
		filter = gl::TexFilter::NearestLinearMin;
	else if (map_tex_filter == 1)
		filter = gl::TexFilter::Linear;
	else if (map_tex_filter == 2)
		filter = gl::TexFilter::LinearMipmap;
	else if (map_tex_filter == 3)
		filter = gl::TexFilter::NearestMipmap;

	// If the texture is loaded
	if (mtex.gl_id)
	{
		// If the texture filter matches the desired one, return it
		if (gl::Texture::info(mtex.gl_id).filter == filter)
			return mtex;

		// Otherwise, reload the texture
		gl::Texture::clear(mtex.gl_id);
		mtex.gl_id = 0;
	}

	// Texture not found or unloaded, look for it

	// Look for composite textures first
	auto  archive = archive_.lock().get();
	auto* ctex    = app::resources().getTexture(name, "WallTexture", archive);
	if (!ctex)
		ctex = app::resources().getTexture(name, "", archive);
	if (ctex)
	{
		SImage image;
		if (ctex->toImage(image, archive, palette_.get(), true))
		{
			mtex.gl_id = gl::Texture::createFromImage(image, palette_.get(), filter);

			double sx = ctex->scaleX();
			if (sx == 0.0)
				sx = 1.0;
			double sy = ctex->scaleY();
			if (sy == 0.0)
				sy = 1.0;

			mtex.world_panning = ctex->worldPanning();
			mtex.scale         = { 1.0 / sx, 1.0 / sy };
		}
	}

	// No composite match, look for stand-alone textures
	else
	{
		// HIRES
		if (auto* etex = app::resources().getHiresEntry(name, archive))
		{
			SImage image;
			if (misc::loadImageFromEntry(&image, etex))
			{
				mtex.gl_id = gl::Texture::createFromImage(image, palette_.get(), filter);

				if (auto* ref = app::resources().getTextureEntry(name, "textures", archive))
				{
					SImage imgref;
					if (misc::loadImageFromEntry(&imgref, ref))
					{
						int w, h, sw, sh;
						w                  = image.width();
						h                  = image.height();
						sw                 = imgref.width();
						sh                 = imgref.height();
						mtex.world_panning = true;
						mtex.scale         = { (double)sw / (double)w, (double)sh / (double)h };
					}
				}
			}
		}

		// TEXTURES
		else
		{
			SImage image;
			etex = app::resources().getTextureEntry(name, "textures", archive);
			if (misc::loadImageFromEntry(&image, etex))
				mtex.gl_id = gl::Texture::createFromImage(image, palette_.get(), filter);
		}
	}

	// Not found
	if (!mtex.gl_id)
	{
		// Try flats if mixed
		if (mixed)
			return flat(name, false);

		// Otherwise use missing texture
		mtex.gl_id = gl::Texture::missingTexture();
	}

	return mtex;
}

// -----------------------------------------------------------------------------
// Returns the flat matching [name], loading it from resources if necessary.
// If [mixed] is true, textures are also searched if no matching flat is found
// -----------------------------------------------------------------------------
const MapTextureManager::Texture& MapTextureManager::flat(string_view name, bool mixed)
{
	// Get flat matching name
	auto& mtex = flats_[strutil::upper(name)];

	// Get desired filter type
	auto filter = gl::TexFilter::Linear;
	if (map_tex_filter == 0)
		filter = gl::TexFilter::NearestLinearMin;
	else if (map_tex_filter == 1)
		filter = gl::TexFilter::Linear;
	else if (map_tex_filter == 2)
		filter = gl::TexFilter::LinearMipmap;
	else if (map_tex_filter == 3)
		filter = gl::TexFilter::NearestMipmap;

	// If the texture is loaded
	if (mtex.gl_id)
	{
		// If the texture filter matches the desired one, return it
		if (gl::Texture::info(mtex.gl_id).filter == filter)
			return mtex;
		
		// Otherwise, reload the texture
		gl::Texture::clear(mtex.gl_id);
		mtex.gl_id = 0;
	}

	// Prioritize standalone textures
	auto archive = archive_.lock().get();
	if (mixed && app::resources().getTextureEntry(name, "textures", archive))
	{
		return texture(name, false);
	}

	// Try composite flat texture
	if (mixed)
	{
		if (auto * ctex = app::resources().getTexture(name, "Flat", archive))
		{
			SImage image;
			if (ctex->toImage(image, archive, palette_.get(), true))
			{
				mtex.gl_id = gl::Texture::createFromImage(image, palette_.get(), filter);

				double sx = ctex->scaleX();
				if (sx == 0.0)
					sx = 1.0;
				double sy = ctex->scaleY();
				if (sy == 0.0)
					sy = 1.0;

				mtex.world_panning = ctex->worldPanning();
				mtex.scale         = { 1.0 / sx, 1.0 / sy };

				return mtex;
			}
		}
	}

	// Try to search for an actual flat
	if (!mtex.gl_id)
	{
		auto* entry = app::resources().getFlatEntry(name, archive);
		auto* hires_entry = app::resources().getHiresEntry(name, archive);
		auto* image_entry = hires_entry;
		auto* scale_entry = entry;
		
		// No high-res texture found
		if (!image_entry)
		{
			image_entry = entry;
			scale_entry = nullptr;
		}
		
		// Load the image
		SImage image;
		if (misc::loadImageFromEntry(&image, image_entry))
			mtex.gl_id = gl::Texture::createFromImage(image, palette_.get(), filter);
		
		// Get high-res texture scale
		if (scale_entry)
		{
			SImage lores_image;
			if (misc::loadImageFromEntry(&lores_image, scale_entry))
			{
				double scale_x = static_cast<double>(lores_image.width()) / static_cast<double>(image.width());
				double scale_y = static_cast<double>(lores_image.height()) / static_cast<double>(image.height());
				mtex.world_panning = true;
				mtex.scale = { scale_x, scale_y };
			}
		}
	}

	// Not found
	if (!mtex.gl_id)
	{
		// Try to search for a composite texture instead
		if (mixed)
			return texture(name, false);

		// Otherwise use missing texture
		mtex.gl_id = gl::Texture::missingTexture();
	}

	return mtex;
}

// -----------------------------------------------------------------------------
// Returns the sprite matching [name], loading it from resources if necessary.
// Sprite name also supports wildcards (?)
// -----------------------------------------------------------------------------
const MapTextureManager::Texture& MapTextureManager::sprite(
	string_view name,
	string_view translation,
	string_view palette)
{
	// Don't bother looking for nameless sprites
	if (name.empty())
		return tex_invalid;

	// Get sprite matching name
	auto hashname = fmt::format("{}{}{}", name, translation, palette);
	strutil::upperIP(hashname);
	auto& mtex = sprites_[hashname];

	// Get desired filter type
	auto filter = gl::TexFilter::Linear;
	if (map_tex_filter == 0)
		filter = gl::TexFilter::NearestLinearMin;
	else if (map_tex_filter == 1)
		filter = gl::TexFilter::Linear;
	else if (map_tex_filter == 2)
		filter = gl::TexFilter::Linear;
	else if (map_tex_filter == 3)
		filter = gl::TexFilter::NearestMipmap;

	// If the texture is loaded
	if (mtex.gl_id)
	{
		// If the texture filter matches the desired one, return it
		auto& tex_info = gl::Texture::info(mtex.gl_id);
		if (tex_info.filter == filter)
			return mtex;
		else
		{
			// Otherwise, reload the texture
			gl::Texture::clear(mtex.gl_id);
			mtex.gl_id = 0;
		}
	}

	// Sprite not found, look for it
	bool   found  = false;
	bool   mirror = false;
	SImage image;
	auto   archive = archive_.lock().get();
	auto   entry   = app::resources().getPatchEntry(name, "sprites", archive);
	if (!entry)
		entry = app::resources().getPatchEntry(name, "", archive);
	if (!entry && name.length() == 8)
	{
		string newname{ name };
		newname[4] = name[6];
		newname[5] = name[7];
		newname[6] = name[4];
		newname[7] = name[5];
		entry      = app::resources().getPatchEntry(newname, "sprites", archive);
		if (entry)
			mirror = true;
	}
	if (entry)
	{
		found = true;
		misc::loadImageFromEntry(&image, entry);
	}
	else // Try composite textures then
	{
		auto ctex = app::resources().getTexture(name, "", archive);
		if (ctex && ctex->toImage(image, archive, palette_.get(), true))
			found = true;
	}

	// We have a valid image either from an entry or a composite texture.
	if (found)
	{
		auto pal = palette_.get();

		// Apply translation
		if (!translation.empty())
			image.applyTranslation(translation, pal, true);

		// Apply palette override
		if (!palette.empty())
		{
			auto newpal = app::resources().getPaletteEntry(palette, archive);
			if (newpal && newpal->size() == 768)
			{
				pal = image.palette();
				pal->loadMem(newpal->data());
			}
		}

		// Apply mirroring
		if (mirror)
			image.mirror(false);

		// Turn into GL texture
		mtex.gl_id = gl::Texture::createFromImage(image, pal, filter, false);
		return mtex;
	}
	else if (name.back() == '?')
	{
		name.remove_suffix(1);
		auto stex = &sprite(fmt::format("{}0", name), translation, palette);
		if (!stex->gl_id)
			stex = &sprite(fmt::format("{}1", name), translation, palette);
		if (stex->gl_id)
			return *stex;
		if (!stex->gl_id && name.length() == 5)
		{
			for (char chr = 'A'; chr <= ']'; ++chr)
			{
				stex = &sprite(fmt::format("{}0{}0", name, chr), translation, palette);
				if (stex->gl_id)
					return *stex;
				stex = &sprite(fmt::format("{}1{}1", name, chr), translation, palette);
				if (stex->gl_id)
					return *stex;
			}
		}
	}

	return tex_invalid;
}

// -----------------------------------------------------------------------------
// Detects offset hacks such as that used by the wall torch thing in Heretic.
// If the Y offset is noticeably larger than the sprite height, that means the
// thing is supposed to be rendered above its real position.
// -----------------------------------------------------------------------------
int MapTextureManager::verticalOffset(string_view name) const
{
	// Don't bother looking for nameless sprites
	if (name.empty())
		return 0;

	// Get sprite matching name
	auto archive = archive_.lock().get();
	auto entry   = app::resources().getPatchEntry(name, "sprites", archive);
	if (!entry)
		entry = app::resources().getPatchEntry(name, "", archive);
	if (entry)
	{
		SImage image;
		misc::loadImageFromEntry(&image, entry);
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
void MapTextureManager::importEditorImages(MapTexHashMap& map, ArchiveDir* dir, string_view path) const
{
	SImage image;

	// Go through entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		auto entry = dir->entryAt(a);

		// Load entry to image
		if (image.open(entry->data()))
		{
			// Create texture in hashmap
			auto name = fmt::format("{}{}", path, entry->nameNoExt());
			log::info(4, "Loading editor texture {}", name);
			auto& mtex = map[name];
			mtex.gl_id = gl::Texture::createFromImage(image, nullptr, gl::TexFilter::Mipmap);
		}
	}

	// Go through subdirs
	for (const auto& subdir : dir->subdirs())
	{
		importEditorImages(map, subdir.get(), fmt::format("{}{}/", path, subdir->name()));
	}
}

// -----------------------------------------------------------------------------
// Returns the editor image matching [name]
// -----------------------------------------------------------------------------
const MapTextureManager::Texture& MapTextureManager::editorImage(string_view name)
{
	if (!gl::isInitialised())
		return tex_invalid;

	// Load thing image textures if they haven't already
	if (!editor_images_loaded_)
	{
		// Load all thing images to textures
		auto slade_pk3 = app::archiveManager().programResourceArchive();
		auto dir       = slade_pk3->dirAtPath("images");
		if (dir)
			importEditorImages(editor_images_, dir, "");

		editor_images_loaded_ = true;
	}

	return editor_images_[strutil::toString(name)];
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
	theMainWindow->paletteChooser()->setGlobalFromArchive(archive_.lock().get());
	mapeditor::forceRefresh(true);
	palette_->copyPalette(resourcePalette());
	buildTexInfoList();
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
	app::resources().putAllTextures(textures, app::archiveManager().baseResourceArchive());
	for (auto& texture : textures)
	{
		auto tex    = &texture->tex;
		auto parent = texture->parent.lock().get();
		if (!parent)
			continue;

		auto long_name = tex->name();
		auto path      = strutil::contains(long_name, '/') ? strutil::beforeLast(long_name, '/') : strutil::EMPTY;

		if (tex->isExtended())
		{
			if (strutil::equalCI(tex->type(), "texture") || strutil::equalCI(tex->type(), "walltexture"))
				tex_info_.emplace_back(long_name, Category::ZDTextures, parent, path, tex->index(), long_name);
			else if (strutil::equalCI(tex->type(), "define"))
				tex_info_.emplace_back(long_name, Category::HiRes, parent, path, tex->index(), long_name);
			else if (strutil::equalCI(tex->type(), "flat"))
				flat_info_.emplace_back(long_name, Category::ZDTextures, parent, path, tex->index(), long_name);
			// Ignore graphics, patches and sprites
		}
		else
			tex_info_.emplace_back(long_name, Category::TextureX, parent, path, tex->index() + 1, long_name);
	}

	// Texture namespace patches (TX_)
	if (game::configuration().featureSupported(game::Feature::TxTextures))
	{
		vector<ArchiveEntry*> patches;
		app::resources().putAllPatchEntries(
			patches, nullptr, game::configuration().featureSupported(game::Feature::LongNames));
		for (auto& patch : patches)
		{
			if (patch->isInNamespace("textures") || patch->isInNamespace("hires"))
			{
				// Determine texture path if it's in a pk3
				auto long_name  = patch->path(true).erase(0, 1);
				auto short_name = strutil::truncate(patch->upperNameNoExt(), 8);
				auto path       = patch->path(false).erase(0, 1);

				tex_info_.emplace_back(short_name, Category::Tx, patch->parent(), path, 0, long_name);
			}
		}
	}

	// Flats
	vector<ArchiveEntry*> flats;
	app::resources().putAllFlatEntries(
		flats, nullptr, game::configuration().featureSupported(game::Feature::LongNames));
	for (auto& flat : flats)
	{
		// Determine flat path if it's in a pk3
		auto long_name  = flat->path(true).erase(0, 1);
		auto short_name = strutil::truncate(flat->upperNameNoExt(), 8);
		auto path       = flat->path(false).erase(0, 1);

		flat_info_.emplace_back(short_name, Category::None, flat->parent(), path, 0, long_name);
	}
}

// -----------------------------------------------------------------------------
// Sets the current archive to [archive], and refreshes all resources
// -----------------------------------------------------------------------------
void MapTextureManager::setArchive(shared_ptr<Archive> archive)
{
	archive_ = archive;
	refreshResources();
}
