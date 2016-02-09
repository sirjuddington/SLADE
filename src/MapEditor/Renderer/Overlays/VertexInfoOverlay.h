
#ifndef __VERTEX_INFO_OVERLAY_H__
#define __VERTEX_INFO_OVERLAY_H__

#include "OpenGL/GLUI/Panel.h"

namespace GLUI { class TextBox; }
class MapVertex;
class VertexInfoOverlay : public GLUI::Panel
{
private:
	GLUI::TextBox*	text_coords;

public:
	VertexInfoOverlay();
	virtual ~VertexInfoOverlay() {}

	void	update(MapVertex* v);
	void	updateLayout(dim2_t fit);
};


#if 0
class MapVertex;
class VertexInfoOverlay
{
private:
	string	info;
	bool	pos_frac;

public:
	VertexInfoOverlay();
	~VertexInfoOverlay();

	void	update(MapVertex* vertex);
	void	draw(int bottom, int right, float alpha = 1.0f);
};
#endif

#endif//__VERTEX_INFO_OVERLAY_H__
