
#ifndef __INFO_OVERLAY_3D_H__
#define __INFO_OVERLAY_3D_H__

class SLADEMap;
class GLTexture;
class MapObject;
class InfoOverlay3D {
private:
	vector<string>	info;
	vector<string>	info2;
	int				current_type;
	string			texname;
	GLTexture*		texture;
	bool			thing_icon;
	MapObject*		object;
	long			last_update;

public:
	InfoOverlay3D();
	~InfoOverlay3D();

	void	update(int item_index, int item_type, SLADEMap* map);
	void	draw(int bottom, int right, int middle, float alpha = 1.0f);
	void	drawTexture(float alpha, int x, int y);
};

#endif//__INFO_OVERLAY_3D_H__
