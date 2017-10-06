#pragma once

#include "EntryPanel.h"
#include "General/SAction.h"

/* TODO:
 * - Improve and enrich palette edition functions
 * - Add some COLORMAP generator or editor maybe. It should handle a fade
 *   to a given color (generally black for COLORMAP, but it might be grey
 *   like Hexen's FOGMAP for example) as well as special colormaps (e.g.,
 *   invulnerability) and invariant ranges (cf. Strife or Harmony). It
 *   should also be able to generate TINTTAB/XLATAB/TRANMAP used by Boom,
 *   Heretic, Hexen and Strife for translucency effects. Note: Hacx only
 *   features 33 ranges in its COLORMAP lump, while the other games have
 *   the full complement of 34 including one unused (a legacy of the Doom
 *   beta version which used a green colormap for the light amp visors).
 */
class PaletteCanvas;
class Palette;
class ArchiveEntry;

class PaletteEntryPanel : public EntryPanel, public SActionHandler
{
public:
	PaletteEntryPanel(wxWindow* parent);
	~PaletteEntryPanel();

	bool	loadEntry(ArchiveEntry* entry) override;
	bool	saveEntry() override;
	string	statusString() override;
	void	refreshPanel() override;
	void	toolbarButtonClick(string action_id) override;

	bool	showPalette(uint32_t index);
	bool	addCustomPalette();
	bool	exportAs();
	bool	importFrom();

	// Palette manipulation functions
	bool	generateColormaps();
	bool	generatePalettes();
	bool	clearOne();
	bool	clearOthers();
	bool	duplicate();
	bool	move(bool infront = false);
	bool	tint();
	bool	colourise();
	bool	tweak();
	bool	invert();
	bool    gradient();
	bool	testPalette();

	// Temporary analysis tool to help reverse engineer stuff
	void	analysePalettes();

	// SAction handler
	bool	handleAction(string id) override;
	bool	fillCustomMenu(wxMenu* custom) override;

private:
	PaletteCanvas*		pal_canvas_		= nullptr;
	vector<Palette*>	palettes_;
	uint32_t			cur_palette_	= 1;
	wxStaticText*		text_curpal_	= nullptr;

	// A helper for generatePalettes() which has no reason to be called outside
	void	generatePalette(int r, int g, int b, int shift, int steps);

	// Events
	void	onPalCanvasMouseEvent(wxMouseEvent& e);
};
