#pragma once

#include "Graphics/Palette/Palette.h"

namespace slade
{
class ColourBox;
class GfxCanvas;
class ArchiveEntry;

/*******************************************************************
 * GFXTINTDIALOG CLASS
 *******************************************************************
 A simple dialog for the 'Tint' function, allows the user to select
 tint colour+amount and shows a preview of the tinted image
 */
class GfxTintDialog : public wxDialog
{
public:
	GfxTintDialog(wxWindow* parent, ArchiveEntry* entry, const Palette& pal);

	ColRGBA colour() const;
	float   amount() const;
	void    setValues(const wxString& col, int val);

private:
	GfxCanvas*    gfx_preview_ = nullptr;
	ArchiveEntry* entry_       = nullptr;
	Palette       palette_;
	ColourBox*    cb_colour_     = nullptr;
	wxSlider*     slider_amount_ = nullptr;
	wxStaticText* label_amount_  = nullptr;

	// Events
	void onColourChanged(wxEvent& e);
	void onAmountChanged(wxCommandEvent& e);
	void onResize(wxSizeEvent& e);
};
} // namespace slade
