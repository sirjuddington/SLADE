#pragma once

#include "General/ListenerAnnouncer.h"

class Archive;
class ArchiveEntry;
class Palette;
class PaletteChooser : public wxChoice, public Announcer
{
public:
	PaletteChooser(wxWindow* parent, int id);
	~PaletteChooser();

	void     setGlobalFromArchive(Archive* archive, int lump = 0);
	Palette* selectedPalette(ArchiveEntry* entry = nullptr);
	bool     globalSelected();
	void     selectPalette(string name);
	void     onPaletteChanged(wxCommandEvent& e);
	void     addPalette(string name);

private:
	Palette* pal_global_;
};
