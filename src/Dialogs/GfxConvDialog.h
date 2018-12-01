#pragma once

#include "Graphics/SImage/SIFormat.h"
#include "Graphics/SImage/SImage.h"
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

class Archive;
class ArchiveEntry;
class CTexture;
class Palette;
class GfxCanvas;
class PaletteChooser;
class ColourBox;

class GfxConvDialog : public SDialog
{
public:
	GfxConvDialog(wxWindow* parent);
	~GfxConvDialog();

	void setupLayout();

	void openEntry(ArchiveEntry* entry);
	void openEntries(vector<ArchiveEntry*> entries);
	void openTextures(
		vector<CTexture*> textures,
		Palette*          palette    = nullptr,
		Archive*          archive    = nullptr,
		bool              force_rgba = false);
	void updatePreviewGfx();
	void updateControls();
	void convertOptions(SIFormat::ConvertOptions& opt);

	bool      itemModified(int index);
	SImage*   itemImage(int index);
	SIFormat* itemFormat(int index);
	Palette*  itemPalette(int index);

	void applyConversion();

private:
	struct ConvFormat
	{
		SIFormat*    format;
		SImage::Type coltype;

		ConvFormat(SIFormat* format = nullptr, SImage::Type coltype = SImage::Type::RGBA)
		{
			this->format  = format;
			this->coltype = coltype;
		}
	};

	struct ConvItem
	{
		ArchiveEntry* entry;
		CTexture*     texture;
		SImage        image;
		bool          modified;
		SIFormat*     new_format;
		Palette*      palette;
		Archive*      archive;
		bool          force_rgba;

		ConvItem(ArchiveEntry* entry = nullptr)
		{
			this->entry      = entry;
			this->texture    = nullptr;
			this->modified   = false;
			this->new_format = nullptr;
			this->palette    = nullptr;
			this->archive    = nullptr;
			this->force_rgba = false;
		}

		ConvItem(CTexture* texture, Palette* palette = nullptr, Archive* archive = nullptr, bool force_rgba = false)
		{
			this->entry      = nullptr;
			this->texture    = texture;
			this->modified   = false;
			this->new_format = nullptr;
			this->palette    = palette;
			this->archive    = archive;
			this->force_rgba = force_rgba;
		}
	};

	vector<ConvItem>   items_;
	size_t             current_item_;
	vector<ConvFormat> conv_formats_;
	ConvFormat         current_format_;

	wxStaticText*   label_current_format_;
	GfxCanvas*      gfx_current_;
	GfxCanvas*      gfx_target_;
	wxButton*       btn_convert_;
	wxButton*       btn_convert_all_;
	wxButton*       btn_skip_;
	wxButton*       btn_skip_all_;
	wxChoice*       combo_target_format_;
	PaletteChooser* pal_chooser_current_;
	PaletteChooser* pal_chooser_target_;
	wxButton*       btn_colorimetry_settings_;

	wxCheckBox*    cb_enable_transparency_;
	wxRadioButton* rb_transparency_existing_;
	wxRadioButton* rb_transparency_colour_;
	wxRadioButton* rb_transparency_brightness_;
	wxSlider*      slider_alpha_threshold_;
	ColourBox*     colbox_transparent_;

	// Conversion options
	Palette target_pal_;
	uint8_t pal_convert_type_; // 0=nearest colour, 1=keep indices
	uint8_t alpha_threshold_;
	bool    keep_trans_;
	ColRGBA colour_trans_;

	bool nextItem();

	// Static
	static string current_palette_name_;
	static string target_palette_name_;

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
