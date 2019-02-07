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
	typedef std::map<wxString, Texture> MapTexHashMap;

	struct TexInfo
	{
		wxString short_name;
		Category category;
		Archive* archive;
		wxString path;
		unsigned index;
		wxString long_name;

		TexInfo(
			const wxString& short_name,
			Category        category,
			Archive*        archive,
			const wxString& path,
			unsigned        index     = 0,
			const wxString& long_name = "") :
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
	const Texture& texture(const wxString& name, bool mixed);
	const Texture& flat(const wxString& name, bool mixed);
	const Texture& sprite(wxString name, const wxString& translation = "", const wxString& palette = "");
	const Texture& editorImage(const wxString& name);
	int            verticalOffset(const wxString& name) const;

	vector<TexInfo>& allTexturesInfo() { return tex_info_; }
	vector<TexInfo>& allFlatsInfo() { return flat_info_; }

	void onAnnouncement(Announcer* announcer, const wxString& event_name, MemChunk& event_data) override;

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

	void importEditorImages(MapTexHashMap& map, ArchiveTreeNode* dir, const wxString& path) const;
};
