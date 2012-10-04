
#ifndef __MAP_TEXTURE_MANAGER_H__
#define __MAP_TEXTURE_MANAGER_H__

#include "GLTexture.h"
#include "ListenerAnnouncer.h"
#include <wx/hashmap.h>

struct map_tex_t {
	GLTexture*	texture;
	map_tex_t() { texture = NULL; }
	~map_tex_t() { if (texture && texture != &(GLTexture::missingTex())) delete texture; }
};

// Declare hash map class to hold textures
WX_DECLARE_STRING_HASH_MAP(map_tex_t, MapTexHashMap);

class Archive;
class MapTextureManager : public Listener {
private:
	Archive*		archive;
	MapTexHashMap	textures;
	MapTexHashMap	flats;
	MapTexHashMap	sprites;
	MapTexHashMap	editor_images;
	bool			editor_images_loaded;

public:
	MapTextureManager(Archive* archive = NULL);
	~MapTextureManager();

	void	setArchive(Archive* archive);
	void	refreshResources();

	GLTexture*	getTexture(string name, bool mixed);
	GLTexture*	getFlat(string name, bool mixed);
	GLTexture*	getSprite(string name, string translation = "", string palette = "");
	GLTexture*	getEditorImage(string name);
	int			getVerticalOffset(string name);

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);
};

#endif//__MAP_TEXTURE_MANAGER_H__
