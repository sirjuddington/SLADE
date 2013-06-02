
#ifndef __PALETTECHOOSER_H__
#define	__PALETTECHOOSER_H__

#include "Palette.h"
#include "Archive.h"
#include "Misc.h"
#include "ListenerAnnouncer.h"

class PaletteChooser : public wxChoice, public Announcer
{
private:
	Palette8bit	pal_global;

public:
	PaletteChooser(wxWindow* parent, int id);
	~PaletteChooser();

	void			setGlobalFromArchive(Archive* archive, int lump = PAL_NOHACK);
	Palette8bit*	getSelectedPalette(ArchiveEntry* entry = NULL);
	bool			globalSelected();
	void			selectPalette(string name);
	void			onPaletteChanged(wxCommandEvent& e);
	void			addPalette(string name);
};

#endif//__PALETTECHOOSER_H__
