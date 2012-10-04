
#ifndef __VERTEX_INFO_OVERLAY_H__
#define __VERTEX_INFO_OVERLAY_H__

class MapVertex;
class VertexInfoOverlay {
private:
	string	info;
	bool	pos_frac;

public:
	VertexInfoOverlay();
	~VertexInfoOverlay();

	void	update(MapVertex* vertex);
	void	draw(int bottom, int right, float alpha = 1.0f);
};

#endif//__VERTEX_INFO_OVERLAY_H__
