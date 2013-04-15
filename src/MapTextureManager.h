
#ifndef __MAP_TEXTURE_MANAGER_H__
#define __MAP_TEXTURE_MANAGER_H__

#include "GLTexture.h"
#include "ListenerAnnouncer.h"
#include <map>

struct map_tex_t {
	GLTexture*	texture;
	map_tex_t() { texture = NULL; }
	~map_tex_t() { if (texture && texture != &(GLTexture::missingTex())) delete texture; }
};

typedef std::map<string, map_tex_t> MapTexHashMap;

class Archive;
class Palette8bit;
class MapTextureManager : public Listener {
private:
	Archive*		archive;
	MapTexHashMap	textures;
	MapTexHashMap	flats;
	MapTexHashMap	sprites;
	MapTexHashMap	editor_images;
	bool			editor_images_loaded;
	Palette8bit*	palette;

public:
	MapTextureManager(Archive* archive = NULL);
	~MapTextureManager();

	void	setArchive(Archive* archive);
	void	refreshResources();

	Palette8bit*	getResourcePalette();
	GLTexture*		getTexture(string name, bool mixed);
	GLTexture*		getFlat(string name, bool mixed);
	GLTexture*		getSprite(string name, string translation = "", string palette = "");
	GLTexture*		getEditorImage(string name);
	int				getVerticalOffset(string name);

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);
};

#endif//__MAP_TEXTURE_MANAGER_H__
