
#include "Main.h"
#include "VertexInfoOverlay.h"
#include "MapVertex.h"
#include "Drawing.h"
#include "MathStuff.h"
#include "ColourConfiguration.h"

VertexInfoOverlay::VertexInfoOverlay() {
	// Init variables
	pos_frac = false;
}

VertexInfoOverlay::~VertexInfoOverlay() {
}

void VertexInfoOverlay::update(MapVertex* vertex) {
	if (!vertex)
		return;

	// Update info string
	if (pos_frac)
		info = S_FMT("Vertex %d: (%1.4f, %1.4f)", vertex->getIndex(), vertex->xPos(), vertex->yPos());
	else
		info = S_FMT("Vertex %d: (%d, %d)", vertex->getIndex(), (int)vertex->xPos(), (int)vertex->yPos());

	if (Global::debug)
		info += S_FMT(" (%d)", vertex->getId());
}

void VertexInfoOverlay::draw(int bottom, int right, float alpha) {
	// Don't bother if completely faded
	if (alpha <= 0.0f)
		return;

	// Init GL stuff
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a = col_fg.a*alpha;
	col_bg.a = col_bg.a*alpha;
	rgba_t col_border(0, 0, 0, 140);

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	bottom += 16*alpha_inv*alpha_inv;

	// Draw overlay background
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Drawing::drawBorderedRect(0, bottom - 24, right, bottom + 2, col_bg, col_border);

	// Draw text
	Drawing::drawText(info, 2, bottom - 20, col_fg, Drawing::FONT_CONDENSED);

	// Done
	glEnable(GL_LINE_SMOOTH);
}
