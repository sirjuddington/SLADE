#pragma once

#include "Graphics/Palette/Palette.h"

namespace slade
{
class ArchiveEntry;
class GfxCanvas;
class ColourBox;

class GfxColouriseDialog : public wxDialog
{
public:
	GfxColouriseDialog(wxWindow* parent, ArchiveEntry* entry, const Palette& pal);

	ColRGBA colour() const;
	void    setColour(const wxString& col);

private:
	GfxCanvas*    gfx_preview_ = nullptr;
	ArchiveEntry* entry_       = nullptr;
	Palette       palette_;
	ColourBox*    cb_colour_ = nullptr;

	// Events
	void onColourChanged(wxEvent& e);
	void onResize(wxSizeEvent& e);
};
} // namespace slade
