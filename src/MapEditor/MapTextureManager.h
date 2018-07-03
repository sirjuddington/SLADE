
#ifndef __MAP_TEXTURE_MANAGER_H__
#define __MAP_TEXTURE_MANAGER_H__

#include "common.h"
#include "OpenGL/GLTexture.h"
#include "General/ListenerAnnouncer.h"

struct map_tex_t
{
	GLTexture*	texture;
	map_tex_t() { texture = nullptr; }
	~map_tex_t() { if (texture && texture != &(GLTexture::missingTex())) delete texture; }
};

class Archive;
struct map_texinfo_t
{
	string			shortName;
	uint8_t			category;
	Archive*		archive;
	string			path;
	unsigned		index;
	string			longName;

	map_texinfo_t(string shortName, uint8_t category, Archive* archive, string path, unsigned index = 0, string longName = "")
	: shortName(shortName), category(category), archive(archive), path(path), index(index), longName(longName)
	{
	}
};

typedef std::map<string, map_tex_t> MapTexHashMap;

class Palette;
class MapTextureManager : public Listener
{
private:
	Archive*				archive;
	MapTexHashMap			textures;
	MapTexHashMap			flats;
	MapTexHashMap			sprites;
	MapTexHashMap			editor_images;
	bool					editor_images_loaded;
	Palette*			palette;
	vector<map_texinfo_t>	tex_info;
	vector<map_texinfo_t>	flat_info;

public:
	enum
	{
		// Texture categories
		TC_NONE = 0,
		TC_TEXTUREX,
		TC_TX,
		TC_TEXTURES,
		TC_HIRES
	};

	MapTextureManager(Archive* archive = nullptr);
	~MapTextureManager();

	void	init();
	void	setArchive(Archive* archive);
	void	refreshResources();
	void	buildTexInfoList();

	Palette*	getResourcePalette();
	GLTexture*		getTexture(string name, bool mixed);
	GLTexture*		getFlat(string name, bool mixed);
	GLTexture*		getSprite(string name, string translation = "", string palette = "");
	GLTexture*		getEditorImage(string name);
	int				getVerticalOffset(string name);

	vector<map_texinfo_t>&	getAllTexturesInfo() { return tex_info; }
	vector<map_texinfo_t>&	getAllFlatsInfo() { return flat_info; }

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);
};

#endif//__MAP_TEXTURE_MANAGER_H__
