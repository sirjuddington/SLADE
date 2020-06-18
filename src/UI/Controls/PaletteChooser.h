#pragma once

#include "Graphics/Palette/Palette.h"

namespace slade
{
class Archive;
class ArchiveEntry;

class PaletteChooser : public wxChoice
{
public:
	PaletteChooser(wxWindow* parent, int id);
	~PaletteChooser() = default;

	void     setGlobalFromArchive(Archive* archive, int lump = 0);
	Palette* selectedPalette(ArchiveEntry* entry = nullptr);
	bool     globalSelected() const;
	void     selectPalette(wxString name);
	void     onPaletteChanged(wxCommandEvent& e);
	void     addPalette(wxString name);

	// Signals
	struct Signals
	{
		sigslot::signal<> palette_changed;
	};
	Signals& signals() { return signals_; }

private:
	Palette pal_global_;
	Signals signals_;
};
} // namespace slade
