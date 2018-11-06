
#ifndef __PALETTECHOOSER_H__
#define	__PALETTECHOOSER_H__

#include "common.h"
#include "General/ListenerAnnouncer.h"

class Archive;
class ArchiveEntry;
class Palette;
class PaletteChooser : public wxChoice, public Announcer
{
private:
	Palette*	pal_global;

public:
	PaletteChooser(wxWindow* parent, int id);
	~PaletteChooser();

	void			setGlobalFromArchive(Archive* archive, int lump = 0);
	Palette*	getSelectedPalette(ArchiveEntry* entry = nullptr);
	bool			globalSelected();
	void			selectPalette(string name);
	void			onPaletteChanged(wxCommandEvent& e);
	void			addPalette(string name);
};

#endif//__PALETTECHOOSER_H__
