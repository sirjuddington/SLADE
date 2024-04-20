#pragma once

#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SIFormat.h"
#include "UI/SDialog.h"

/* Convert from anything to:
 * Doom Gfx
 * Doom Flat
 * PNG 32bit
 * PNG Paletted
 *
 * Conversion options:
 *	Colours:
 *		- Specify target palette (only if converting to paletted)
 *		- Specify palette conversion type:
 *			- Keep palette indices (only if converting from 8bit)
 *			- Nearest colour matching
 *
 *	Transparency:
 *		- Specify threshold alpha, anything above is opaque (optional if converting from 32bit)
 *		- Specify transparency info:
 *			- Keep existing transparency (threshold comes into play from 32bit-paletted)
 *			- Select transparency colour (to 32bit - select colour, to paletted - select from target palette)
 */

namespace slade
{
class CTexture;
class GfxGLCanvas;
class PaletteChooser;
class ColourBox;

class GfxConvDialog : public SDialog
{
public:
	GfxConvDialog(wxWindow* parent);
	~GfxConvDialog() override;

	void setupLayout();

	void openEntry(ArchiveEntry* entry);
	void openEntries(const vector<ArchiveEntry*>& entries);
	void openTextures(
		const vector<CTexture*>& textures,
		const Palette*           palette    = nullptr,
		Archive*                 archive    = nullptr,
		bool                     force_rgba = false);
	void updatePreviewGfx() const;
	void updateControls() const;
	void convertOptions(SIFormat::ConvertOptions& opt) const;

	bool           itemModified(int index) const;
	SImage*        itemImage(int index);
	SIFormat*      itemFormat(int index) const;
	const Palette* itemPalette(int index) const;

	void applyConversion();

private:
	struct ConvFormat
	{
		SIFormat*    format;
		SImage::Type coltype;

		ConvFormat(SIFormat* format = nullptr, SImage::Type coltype = SImage::Type::RGBA) :
			format{ format },
			coltype{ coltype }
		{
		}
	};

	struct ConvItem
	{
		ArchiveEntry*  entry   = nullptr;
		CTexture*      texture = nullptr;
		SImage         image;
		bool           modified   = false;
		SIFormat*      new_format = nullptr;
		const Palette* palette    = nullptr;
		Archive*       archive    = nullptr;
		bool           force_rgba = false;

		ConvItem(ArchiveEntry* entry = nullptr) : entry{ entry } {}

		ConvItem(
			CTexture*      texture,
			const Palette* palette    = nullptr,
			Archive*       archive    = nullptr,
			bool           force_rgba = false) :
			texture{ texture },
			palette{ palette },
			archive{ archive },
			force_rgba{ force_rgba }
		{
		}
	};

	vector<ConvItem>   items_;
	size_t             current_item_ = 0;
	vector<ConvFormat> conv_formats_;
	ConvFormat         current_format_;

	wxStaticText*   label_current_format_     = nullptr;
	GfxGLCanvas*    gfx_current_              = nullptr;
	GfxGLCanvas*    gfx_target_               = nullptr;
	wxButton*       btn_convert_              = nullptr;
	wxButton*       btn_convert_all_          = nullptr;
	wxButton*       btn_skip_                 = nullptr;
	wxButton*       btn_skip_all_             = nullptr;
	wxChoice*       combo_target_format_      = nullptr;
	PaletteChooser* pal_chooser_current_      = nullptr;
	PaletteChooser* pal_chooser_target_       = nullptr;
	wxButton*       btn_colorimetry_settings_ = nullptr;

	wxCheckBox*    cb_enable_transparency_     = nullptr;
	wxRadioButton* rb_transparency_existing_   = nullptr;
	wxRadioButton* rb_transparency_colour_     = nullptr;
	wxRadioButton* rb_transparency_brightness_ = nullptr;
	wxSlider*      slider_alpha_threshold_     = nullptr;
	ColourBox*     colbox_transparent_         = nullptr;

	// Conversion options
	Palette target_pal_;
	ColRGBA colour_trans_;

	bool nextItem();
	void updateButtons() const;

	// Static
	static wxString current_palette_name_;
	static wxString target_palette_name_;

	// Events
	void onResize(wxSizeEvent& e);
	void onBtnConvert(wxCommandEvent& e);
	void onBtnConvertAll(wxCommandEvent& e);
	void onBtnSkip(wxCommandEvent& e);
	void onBtnSkipAll(wxCommandEvent& e);
	void onTargetFormatChanged(wxCommandEvent& e);
	void onCurrentPaletteChanged(wxCommandEvent& e);
	void onTargetPaletteChanged(wxCommandEvent& e);
	void onAlphaThresholdChanged(wxCommandEvent& e);
	void onEnableTransparencyChanged(wxCommandEvent& e);
	void onTransTypeChanged(wxCommandEvent& e);
	void onTransColourChanged(wxEvent& e);
	void onPreviewCurrentMouseDown(wxMouseEvent& e);
	void onBtnColorimetrySettings(wxCommandEvent& e);
};
} // namespace slade
