
#ifndef __PALETTECHOOSER_H__
#define	__PALETTECHOOSER_H__

#include "common.h"
#include "General/ListenerAnnouncer.h"

class Archive;
class ArchiveEntry;
class Palette8bit;
class PaletteChooser : public wxChoice, public Announcer
{
private:
	Palette8bit*	pal_global;

public:
	PaletteChooser(wxWindow* parent, int id);
	~PaletteChooser();

	void			setGlobalFromArchive(Archive* archive, int lump = 0);
	Palette8bit*	getSelectedPalette(ArchiveEntry* entry = NULL);
	bool			globalSelected();
	void			selectPalette(string name);
	void			onPaletteChanged(wxCommandEvent& e);
	void			addPalette(string name);
};

#endif//__PALETTECHOOSER_H__
