#pragma once

#include "OpenGL/GLTexture.h"

namespace slade
{
class ArchiveDir;
class Archive;
class Palette;

class MapTextureManager
{
public:
	enum class Category
	{
		// Texture categories
		None = 0,
		TextureX,
		Tx,
		ZDTextures,
		HiRes
	};

	struct Texture
	{
		unsigned gl_id         = 0;
		bool     world_panning = false;
		Vec2d    scale         = { 1., 1. };
		~Texture() { gl::Texture::clear(gl_id); }
	};
	typedef std::map<string, Texture> MapTexHashMap;

	struct TexInfo
	{
		string   short_name;
		Category category;
		Archive* archive;
		string   path;
		unsigned index;
		string   long_name;

		TexInfo(
			string_view short_name,
			Category    category,
			Archive*    archive,
			string_view path,
			unsigned    index     = 0,
			string_view long_name = "") :
			short_name(short_name),
			category(category),
			archive(archive),
			path(path),
			index(index),
			long_name(long_name)
		{
		}
	};

	MapTextureManager(shared_ptr<Archive> archive = nullptr);
	~MapTextureManager() = default;

	void init();
	void setArchive(shared_ptr<Archive> archive);
	void refreshResources();

	Palette*       resourcePalette() const;
	const Texture& texture(string_view name, bool mixed);
	const Texture& flat(string_view name, bool mixed);
	const Texture& sprite(string_view name, string_view translation = "", string_view palette = "");
	const Texture& editorImage(string_view name);
	int            verticalOffset(string_view name) const;

	vector<TexInfo>& allTexturesInfo()
	{
		if (tex_info_.empty() && flat_info_.empty())
			buildTexInfoList();

		return tex_info_;
	}

	vector<TexInfo>& allFlatsInfo()
	{
		if (tex_info_.empty() && flat_info_.empty())
			buildTexInfoList();

		return flat_info_;
	}

private:
	weak_ptr<Archive>   archive_;
	MapTexHashMap       textures_;
	MapTexHashMap       flats_;
	MapTexHashMap       sprites_;
	MapTexHashMap       editor_images_;
	bool                editor_images_loaded_ = false;
	unique_ptr<Palette> palette_;
	vector<TexInfo>     tex_info_;
	vector<TexInfo>     flat_info_;

	// Signal connections
	sigslot::scoped_connection sc_resources_updated_;
	sigslot::scoped_connection sc_palette_changed_;

	void buildTexInfoList();
	void importEditorImages(MapTexHashMap& map, const ArchiveDir* dir, string_view path) const;
};
} // namespace slade
