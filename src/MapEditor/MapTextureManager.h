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
		GLTexture* texture;
		Texture() { texture = nullptr; }
		~Texture()
		{
			if (texture && texture != &(GLTexture::missingTex()))
				delete texture;
		}
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
			string   short_name,
			Category category,
			Archive* archive,
			string   path,
			unsigned index     = 0,
			string   long_name = "") :
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

	Palette*   resourcePalette() const;
	GLTexture* texture(const string& name, bool mixed);
	GLTexture* flat(const string& name, bool mixed);
	GLTexture* sprite(string name, const string& translation = "", const string& palette = "");
	GLTexture* editorImage(const string& name);
	int        verticalOffset(const string& name) const;

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
