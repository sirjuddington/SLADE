
#ifndef __VERTEX_INFO_OVERLAY_H__
#define __VERTEX_INFO_OVERLAY_H__

#include "InfoOverlay.h"

namespace GLUI { class TextBox; }
class MapVertex;
class VertexInfoOverlay : public InfoOverlay
{
private:
	GLUI::TextBox*	text_coords;

public:
	VertexInfoOverlay();
	virtual ~VertexInfoOverlay() {}

	void	update(MapVertex* v);
	void	updateLayout(dim2_t fit);
};

#endif//__VERTEX_INFO_OVERLAY_H__
