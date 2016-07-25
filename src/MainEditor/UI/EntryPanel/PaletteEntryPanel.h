
#ifndef __PALETTEENTRYPANEL_H__
#define	__PALETTEENTRYPANEL_H__

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
class Palette8bit;
class ArchiveEntry;
class PaletteEntryPanel : public EntryPanel, public SActionHandler
{
private:
	PaletteCanvas*			pal_canvas;
	vector<Palette8bit*>	palettes;
	uint32_t				cur_palette;

	wxStaticText*			text_curpal;

	// A helper for generatePalettes() which has no reason to be called outside
	void					generatePalette(int r, int g, int b, int shift, int steps);

public:
	PaletteEntryPanel(wxWindow* parent);
	~PaletteEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();
	string	statusString();
	void	refreshPanel();
	void	toolbarButtonClick(string action_id);

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
	bool	handleAction(string id);
	bool	fillCustomMenu(wxMenu* custom);

	// Events
	void	onPalCanvasMouseEvent(wxMouseEvent& e);
};

#endif //__PALETTEENTRYPANEL_H__
