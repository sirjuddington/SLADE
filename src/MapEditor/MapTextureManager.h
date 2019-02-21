#pragma once

#include "General/ListenerAnnouncer.h"
#include "OpenGL/GLTexture.h"

class ArchiveTreeNode;
class Archive;
class Palette;

class MapTextureManager : public Listener
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
		~Texture() { OpenGL::Texture::clear(gl_id); }
	};
	typedef std::map<std::string, Texture> MapTexHashMap;

	struct TexInfo
	{
		std::string short_name;
		Category    category;
		Archive*    archive;
		std::string path;
		unsigned    index;
		std::string long_name;

		TexInfo(
			std::string_view short_name,
			Category         category,
			Archive*         archive,
			std::string_view path,
			unsigned         index     = 0,
			std::string_view long_name = "") :
			short_name(short_name),
			category(category),
			archive(archive),
			path(path),
			index(index),
			long_name(long_name)
		{
		}
	};

	MapTextureManager(Archive* archive = nullptr);
	~MapTextureManager() = default;

	void init();
	void setArchive(Archive* archive);
	void refreshResources();
	void buildTexInfoList();

	Palette*       resourcePalette() const;
	const Texture& texture(std::string_view name, bool mixed);
	const Texture& flat(std::string_view name, bool mixed);
	const Texture& sprite(std::string_view name, std::string_view translation = "", std::string_view palette = "");
	const Texture& editorImage(std::string_view name);
	int            verticalOffset(std::string_view name) const;

	vector<TexInfo>& allTexturesInfo() { return tex_info_; }
	vector<TexInfo>& allFlatsInfo() { return flat_info_; }

	void onAnnouncement(Announcer* announcer, std::string_view event_name, MemChunk& event_data) override;

private:
	Archive*                 archive_ = nullptr;
	MapTexHashMap            textures_;
	MapTexHashMap            flats_;
	MapTexHashMap            sprites_;
	MapTexHashMap            editor_images_;
	bool                     editor_images_loaded_ = false;
	std::unique_ptr<Palette> palette_;
	vector<TexInfo>          tex_info_;
	vector<TexInfo>          flat_info_;

	void importEditorImages(MapTexHashMap& map, ArchiveTreeNode* dir, std::string_view path) const;
};
