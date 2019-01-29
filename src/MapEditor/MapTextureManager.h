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
			const string& short_name,
			Category      category,
			Archive*      archive,
			const string& path,
			unsigned      index     = 0,
			const string& long_name = "") :
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
	const Texture& texture(const string& name, bool mixed);
	const Texture& flat(const string& name, bool mixed);
	const Texture& sprite(string name, const string& translation = "", const string& palette = "");
	const Texture& editorImage(const string& name);
	int            verticalOffset(const string& name) const;

	vector<TexInfo>& allTexturesInfo() { return tex_info_; }
	vector<TexInfo>& allFlatsInfo() { return flat_info_; }

	void onAnnouncement(Announcer* announcer, const string& event_name, MemChunk& event_data) override;

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

	void importEditorImages(MapTexHashMap& map, ArchiveTreeNode* dir, const string& path) const;
};
