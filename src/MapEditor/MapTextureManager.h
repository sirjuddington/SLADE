
#ifndef __MAP_TEXTURE_MANAGER_H__
#define __MAP_TEXTURE_MANAGER_H__

#include "common.h"
#include "OpenGL/GLTexture.h"
#include "General/ListenerAnnouncer.h"

struct map_tex_t
{
	GLTexture*	texture;
	map_tex_t() { texture = NULL; }
	~map_tex_t() { if (texture && texture != &(GLTexture::missingTex())) delete texture; }
};

class Archive;
struct map_texinfo_t
{
	string			name;
	uint8_t			category;
	string			path;
	unsigned		index;
	Archive*		archive;

	map_texinfo_t(string name, uint8_t category, Archive* archive, string path = "", unsigned index = 0)
	{
		this->name = name;
		this->category = category;
		this->archive = archive;
		this->path = path;
		this->index = index;
	}
};

typedef std::map<string, map_tex_t> MapTexHashMap;

class Palette8bit;
class MapTextureManager : public Listener
{
private:
	Archive*				archive;
	MapTexHashMap			textures;
	MapTexHashMap			flats;
	MapTexHashMap			sprites;
	MapTexHashMap			editor_images;
	bool					editor_images_loaded;
	Palette8bit*			palette;
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

	MapTextureManager(Archive* archive = NULL);
	~MapTextureManager();

	void	init();
	void	setArchive(Archive* archive);
	void	refreshResources();
	void	buildTexInfoList();

	Palette8bit*	getResourcePalette();
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
