
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GfxConvDialog.cpp
// Description: A dialog UI for converting between different gfx formats,
//              including options for conversion
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "GfxConvDialog.h"
#include "General/Misc.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/Icons.h"
#include "Graphics/SImage/SIFormat.h"
#include "MainEditor/MainEditor.h"
#include "SettingsDialog.h"
#include "UI/Canvas/Canvas.h"
#include "UI/Canvas/GL/GfxGLCanvas.h"
#include "UI/Controls/ColourBox.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/Layout.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
string GfxConvDialog::current_palette_name_;
string GfxConvDialog::target_palette_name_;
CVAR(Bool, gfx_extraconv, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// GfxConvDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GfxConvDialog class constructor
// -----------------------------------------------------------------------------
GfxConvDialog::GfxConvDialog(wxWindow* parent) : SDialog(parent, "Graphic Format Conversion", "gfxconv")
{
	// Set dialog icon
	wxutil::setWindowIcon(this, "convert");

	setupLayout();
	CenterOnParent();
}

// -----------------------------------------------------------------------------
// GfxConvDialog class destructor
// -----------------------------------------------------------------------------
GfxConvDialog::~GfxConvDialog()
{
	current_palette_name_ = pal_chooser_current_->GetStringSelection().utf8_string();
	target_palette_name_  = pal_chooser_target_->GetStringSelection().utf8_string();
}

// -----------------------------------------------------------------------------
// Opens the next item to be converted.
// Returns true if the selected format was valid for the next image
// -----------------------------------------------------------------------------
bool GfxConvDialog::nextItem()
{
	// Go to next image
	current_item_++;
	if (current_item_ >= items_.size())
	{
		Close(true);
		return false;
	}

	// Load image if needed
	if (!items_[current_item_].image.isValid())
	{
		// If loading images from entries
		if (items_[current_item_].entry != nullptr)
		{
			if (!misc::loadImageFromEntry(&(items_[current_item_].image), items_[current_item_].entry))
				return nextItem(); // Skip if not a valid image entry
		}
		// If loading images from textures
		else if (items_[current_item_].texture != nullptr)
		{
			if (items_[current_item_].force_rgba)
				items_[current_item_].image.convertRGBA(items_[current_item_].palette);
			if (!items_[current_item_].texture->toImage(
					items_[current_item_].image,
					items_[current_item_].archive,
					items_[current_item_].palette,
					items_[current_item_].force_rgba))
				return nextItem(); // Skip if not a valid image entry
		}
		else
			return nextItem(); // Skip if not a valid image entry
	}

	// Update valid formats
	combo_target_format_->Clear();
	conv_formats_.clear();
	vector<SIFormat*> all_formats;
	SIFormat::putAllFormats(all_formats);
	int current_index = -1;
	int default_index = -1;
	for (auto& format : all_formats)
	{
		// Check if the image can be written to this format
		if (format->canWrite(items_[current_item_].image) != SIFormat::Writable::No)
		{
			// Add conversion formats depending on what colour types this image format can handle
			if (format->canWriteType(SImage::Type::PalMask))
			{
				// Add format
				conv_formats_.emplace_back(format, SImage::Type::PalMask);
				combo_target_format_->Append(wxString::FromUTF8(format->name() + " (Paletted)"));

				// Check for match with current format
				if (current_format_.format == format && current_format_.coltype == SImage::Type::PalMask)
					current_index = conv_formats_.size() - 1;

				// Default format is 'doom gfx'
				if (format->id() == "doom")
					default_index = conv_formats_.size() - 1;
			}

			if (format->canWriteType(SImage::Type::RGBA))
			{
				// Add format
				conv_formats_.emplace_back(format, SImage::Type::RGBA);
				combo_target_format_->Append(wxString::FromUTF8(format->name() + " (Truecolour)"));

				// Check for match with current format
				if (current_format_.format == format && current_format_.coltype == SImage::Type::RGBA)
					current_index = conv_formats_.size() - 1;
			}

			if (format->canWriteType(SImage::Type::AlphaMap))
			{
				// Add format
				conv_formats_.emplace_back(format, SImage::Type::AlphaMap);
				combo_target_format_->Append(wxString::FromUTF8(format->name() + " (Alpha Map)"));

				// Check for match with current format
				if (current_format_.format == format && current_format_.coltype == SImage::Type::AlphaMap)
					current_index = conv_formats_.size() - 1;
			}
		}
	}

	// If the image cannot be converted to the selected format
	bool ok = true;
	if (current_index < 0)
	{
		// Default to Doom Gfx
		current_index = default_index;
		ok            = false;
	}

	// Set current format
	combo_target_format_->SetSelection(current_index);
	current_format_ = conv_formats_[current_index];

	// Setup current format string
	string fmt_string = "Current Format: ";
	if (items_[current_item_].texture == nullptr)
	{
		if (items_[current_item_].image.format())
			fmt_string += items_[current_item_].image.format()->name();
		else
			fmt_string += "Font";
	}
	else
		fmt_string += "Texture";
	if (items_[current_item_].image.type() == SImage::Type::RGBA)
		fmt_string += " (Truecolour)";
	else if (items_[current_item_].image.type() == SImage::Type::PalMask)
	{
		fmt_string += " (Paletted - ";
		if (items_[current_item_].image.hasPalette())
			fmt_string += "Internally)";
		else
			fmt_string += "Externally)";
	}
	else if (items_[current_item_].image.type() == SImage::Type::AlphaMap)
		fmt_string += " (Alpha Map)";
	label_current_format_->SetLabel(wxString::FromUTF8(fmt_string));

	// Update UI
	updatePreviewGfx();
	ui::setSplashProgressMessage(fmt::format("{} of {}", current_item_, items_.size()));
	ui::setSplashProgress(current_item_, items_.size());

	return ok;
}

// -----------------------------------------------------------------------------
// Updates the convert/skip buttons depending on the number of items
// -----------------------------------------------------------------------------
void GfxConvDialog::updateButtons() const
{
	if (items_.size() > 1)
	{
		btn_convert_all_->Show();
		btn_skip_all_->Show();
		btn_skip_->SetLabelText(wxS("Skip"));
	}
	else
	{
		btn_convert_all_->Hide();
		btn_skip_all_->Hide();
		btn_skip_->SetLabelText(wxS("Cancel"));
	}
}

// -----------------------------------------------------------------------------
// Sets up the dialog UI layout
// -----------------------------------------------------------------------------
void GfxConvDialog::setupLayout()
{
	auto lh              = ui::LayoutHelper(this);
	auto px_preview_size = lh.size(192, 192);

	auto msizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(msizer);

	auto m_vbox = new wxBoxSizer(wxVERTICAL);
	msizer->Add(m_vbox, lh.sfWithLargeBorder(1).Expand());

	// Add current format label
	label_current_format_ = new wxStaticText(this, -1, wxS("Current Format:"));
	m_vbox->Add(label_current_format_, lh.sfWithBorder(0, wxBOTTOM));

	// Add 'Convert To' combo box
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	m_vbox->Add(hbox, lh.sfWithLargeBorder(0, wxBOTTOM).Expand());
	hbox->Add(new wxStaticText(this, -1, wxS("Convert to:")), lh.sfWithSmallBorder(0, wxRIGHT).CenterVertical());
	combo_target_format_ = new wxChoice(this, -1);
	hbox->Add(combo_target_format_, wxSizerFlags(1).Expand());


	// Add Gfx previews
	auto frame      = new wxStaticBox(this, -1, wxS("Colour Options"));
	auto framesizer = new wxStaticBoxSizer(frame, wxHORIZONTAL);
	m_vbox->Add(framesizer, lh.sfWithLargeBorder(1, wxBOTTOM).Expand());

	auto gbsizer = new wxGridBagSizer(lh.pad(), lh.pad());
	framesizer->Add(gbsizer, lh.sfWithBorder(1).Expand());

	// Current
	gbsizer->Add(new wxStaticText(this, -1, wxS("Current Graphic")), { 0, 0 }, { 1, 1 });
	gfx_current_ = ui::createGfxCanvas(this);
	gfx_current_->window()->SetInitialSize(px_preview_size);
	gfx_current_->setViewType(GfxView::Centered);
	gbsizer->Add(gfx_current_->window(), { 1, 0 }, { 1, 1 }, wxEXPAND);
	pal_chooser_current_ = new PaletteChooser(this, -1);
	pal_chooser_current_->selectPalette(current_palette_name_);
	gbsizer->Add(pal_chooser_current_, { 2, 0 }, { 1, 1 }, wxEXPAND);

	// Converted
	gbsizer->Add(new wxStaticText(this, -1, wxS("Converted Graphic")), { 0, 1 }, { 1, 2 });
	gfx_target_ = ui::createGfxCanvas(this);
	gfx_target_->window()->SetInitialSize(px_preview_size);
	gfx_target_->setViewType(GfxView::Centered);
	gbsizer->Add(gfx_target_->window(), { 1, 1 }, { 1, 2 }, wxEXPAND);
	pal_chooser_target_ = new PaletteChooser(this, -1);
	pal_chooser_target_->selectPalette(target_palette_name_);
	gbsizer->Add(pal_chooser_target_, { 2, 1 }, { 1, 1 }, wxEXPAND);
	btn_colorimetry_settings_ = new wxBitmapButton(
		this, -1, icons::getIcon(icons::General, "settings"), wxDefaultPosition, wxDefaultSize);
	btn_colorimetry_settings_->SetToolTip(wxS("Adjust Colorimetry Settings..."));
	gbsizer->Add(btn_colorimetry_settings_, { 2, 2 }, { 1, 1 }, wxALIGN_CENTER);
	gbsizer->AddGrowableCol(0, 1);
	gbsizer->AddGrowableCol(1, 1);
	gbsizer->AddGrowableRow(1, 1);


	// Add transparency options
	frame      = new wxStaticBox(this, -1, wxS("Transparency Options"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	m_vbox->Add(framesizer, lh.sfWithLargeBorder(0, wxBOTTOM).Expand());

	gbsizer = new wxGridBagSizer(lh.pad(), lh.pad());
	framesizer->Add(gbsizer, lh.sfWithBorder(1).Expand());

	// 'Enable transparency' checkbox
	cb_enable_transparency_ = new wxCheckBox(this, -1, wxS("Enable Transparency"));
	cb_enable_transparency_->SetValue(true);
	cb_enable_transparency_->SetToolTip(wxS("Uncheck this to remove any existing transparency from the graphic"));
	gbsizer->Add(cb_enable_transparency_, { 0, 0 }, { 1, 2 });

	// Keep existing transparency
	rb_transparency_existing_ = new wxRadioButton(
		this, 100, wxS("Existing w/Threshold:"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	rb_transparency_existing_->SetValue(true);
	gbsizer->Add(rb_transparency_existing_, { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);

	// Alpha threshold
	slider_alpha_threshold_ = new wxSlider(
		this, -1, 0, 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS | wxSL_BOTTOM);
	slider_alpha_threshold_->SetToolTip(
		wxS("Specifies the 'cutoff' transparency level, anything above this will be fully opaque, "
			"anything equal or below will be completely transparent"));
	gbsizer->Add(slider_alpha_threshold_, { 1, 1 }, { 1, 1 }, wxEXPAND);

	// Transparent colour
	rb_transparency_colour_ = new wxRadioButton(
		this, 101, wxS("Transparent Colour:"), wxDefaultPosition, wxDefaultSize, 0);
	rb_transparency_colour_->SetValue(false);
	gbsizer->Add(rb_transparency_colour_, { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);

	colbox_transparent_ = new ColourBox(this, -1, false);
	colbox_transparent_->setColour(ColRGBA(0, 255, 255, 255));
	gbsizer->Add(colbox_transparent_, { 2, 1 }, { 1, 1 });

	// From brightness
	rb_transparency_brightness_ = new wxRadioButton(this, 102, wxS("Transparency from Brightness"));
	rb_transparency_brightness_->SetValue(false);
	gbsizer->Add(rb_transparency_brightness_, { 3, 0 }, { 1, 2 });
	gbsizer->AddGrowableCol(1, 1);

	// Buttons
	hbox = new wxBoxSizer(wxHORIZONTAL);
	m_vbox->Add(hbox, wxSizerFlags().Expand());

	btn_convert_     = new wxButton(this, -1, wxS("Convert"));
	btn_convert_all_ = new wxButton(this, -1, wxS("Convert All"));
	btn_skip_        = new wxButton(this, -1, wxS("Skip"));
	btn_skip_all_    = new wxButton(this, -1, wxS("Skip All"));

	hbox->AddStretchSpacer(1);
	hbox->Add(btn_convert_, lh.sfWithBorder(0, wxRIGHT).Expand());
	hbox->Add(btn_convert_all_, lh.sfWithBorder(0, wxRIGHT).Expand());
	hbox->Add(btn_skip_, lh.sfWithBorder(0, wxRIGHT).Expand());
	hbox->Add(btn_skip_all_, wxSizerFlags().Expand());


	// Bind events
	Bind(wxEVT_SIZE, &GfxConvDialog::onResize, this);
	btn_convert_->Bind(wxEVT_BUTTON, &GfxConvDialog::onBtnConvert, this);
	btn_convert_all_->Bind(wxEVT_BUTTON, &GfxConvDialog::onBtnConvertAll, this);
	btn_skip_->Bind(wxEVT_BUTTON, &GfxConvDialog::onBtnSkip, this);
	btn_skip_all_->Bind(wxEVT_BUTTON, &GfxConvDialog::onBtnSkipAll, this);
	combo_target_format_->Bind(wxEVT_CHOICE, &GfxConvDialog::onTargetFormatChanged, this);
	pal_chooser_current_->Bind(wxEVT_CHOICE, &GfxConvDialog::onCurrentPaletteChanged, this);
	pal_chooser_target_->Bind(wxEVT_CHOICE, &GfxConvDialog::onTargetPaletteChanged, this);
	slider_alpha_threshold_->Bind(wxEVT_SLIDER, &GfxConvDialog::onAlphaThresholdChanged, this);
	cb_enable_transparency_->Bind(wxEVT_CHECKBOX, &GfxConvDialog::onEnableTransparencyChanged, this);
	rb_transparency_colour_->Bind(wxEVT_RADIOBUTTON, &GfxConvDialog::onTransTypeChanged, this);
	rb_transparency_existing_->Bind(wxEVT_RADIOBUTTON, &GfxConvDialog::onTransTypeChanged, this);
	rb_transparency_brightness_->Bind(wxEVT_RADIOBUTTON, &GfxConvDialog::onTransTypeChanged, this);
	Bind(wxEVT_COLOURBOX_CHANGED, &GfxConvDialog::onTransColourChanged, this, colbox_transparent_->GetId());
	gfx_current_->window()->Bind(wxEVT_LEFT_DOWN, &GfxConvDialog::onPreviewCurrentMouseDown, this);
	btn_colorimetry_settings_->Bind(wxEVT_BUTTON, &GfxConvDialog::onBtnColorimetrySettings, this);


	// Autosize to fit contents (and set this as the minimum size)
	SetMinClientSize(msizer->GetMinSize());
}

// -----------------------------------------------------------------------------
// Opens an image entry to be converted
// -----------------------------------------------------------------------------
void GfxConvDialog::openEntry(ArchiveEntry* entry)
{
	// Add item
	items_.emplace_back(entry);

	// Open it
	current_item_ = -1;
	nextItem();
	updateButtons();
}

// -----------------------------------------------------------------------------
// Opens a list of image entries to be converted
// -----------------------------------------------------------------------------
void GfxConvDialog::openEntries(const vector<ArchiveEntry*>& entries)
{
	// Add entries to item list
	for (auto& entry : entries)
		items_.emplace_back(entry);

	// Open the first item
	current_item_ = -1;
	nextItem();
	updateButtons();
}

// -----------------------------------------------------------------------------
// Opens a list of composite textures to be converted
// -----------------------------------------------------------------------------
void GfxConvDialog::openTextures(
	const vector<CTexture*>& textures,
	const Palette*           palette,
	Archive*                 archive,
	bool                     force_rgba)
{
	// Add entries to item list
	for (auto& texture : textures)
		items_.emplace_back(texture, palette, archive, force_rgba);

	// Open the first item
	current_item_ = -1;
	nextItem();
	updateButtons();
}

// -----------------------------------------------------------------------------
// Updates the current and target preview windows
// -----------------------------------------------------------------------------
void GfxConvDialog::updatePreviewGfx() const
{
	// Check current item is valid
	if (items_.size() <= current_item_)
		return;

	// Get the current image entry
	auto& item = items_[current_item_];

	// Set palettes
	if (item.image.hasPalette() && pal_chooser_current_->globalSelected())
		gfx_current_->setPalette(item.image.palette());
	else
		gfx_current_->setPalette(pal_chooser_current_->selectedPalette(item.entry));
	if (pal_chooser_target_->globalSelected())
		gfx_target_->setPalette(gfx_current_->palette());
	else
		gfx_target_->setPalette(pal_chooser_target_->selectedPalette(item.entry));

	// Load the image to both gfx canvases
	gfx_current_->image().copyImage(&item.image);
	gfx_target_->image().copyImage(&item.image);

	// Update controls
	updateControls();


	// --- Apply image conversion to target preview ---

	// Get conversion options
	SIFormat::ConvertOptions opt;
	convertOptions(opt);

	// Do conversion
	current_format_.format->convertWritable(gfx_target_->image(), opt);


	// Refresh
	gfx_current_->resetViewOffsets();
	gfx_current_->zoomToFit(true, 0.05);
	gfx_current_->window()->Refresh();
	gfx_target_->resetViewOffsets();
	gfx_target_->zoomToFit(true, 0.05);
	gfx_target_->window()->Refresh();
}

// -----------------------------------------------------------------------------
// Disables/enables controls based on what is currently selected
// -----------------------------------------------------------------------------
void GfxConvDialog::updateControls() const
{
	// Check current item is valid
	if (items_.size() <= current_item_)
		return;

	// Set colourbox palette if source image has one
	auto coltype = gfx_current_->image().type();
	if (coltype == SImage::Type::PalMask)
		colbox_transparent_->setPalette(gfx_current_->palette());
	else
		colbox_transparent_->setPalette(nullptr);

	// Disable/enable transparency options depending on transparency checkbox
	if (cb_enable_transparency_->GetValue())
	{
		// Disable/enable alpha threshold slider as needed
		if (coltype == SImage::Type::RGBA || coltype == SImage::Type::AlphaMap)
			slider_alpha_threshold_->Enable(true);
		else
			slider_alpha_threshold_->Enable(false);

		rb_transparency_colour_->Enable(true);
		rb_transparency_existing_->Enable(true);
		rb_transparency_brightness_->Enable(true);
	}
	else
	{
		rb_transparency_colour_->Enable(false);
		rb_transparency_existing_->Enable(false);
		rb_transparency_brightness_->Enable(false);
		slider_alpha_threshold_->Enable(false);
	}
}

// -----------------------------------------------------------------------------
// Writes the state of the conversion option controls to [opt]
// -----------------------------------------------------------------------------
void GfxConvDialog::convertOptions(SIFormat::ConvertOptions& opt) const
{
	// Set transparency options
	opt.transparency = cb_enable_transparency_->GetValue();
	if (rb_transparency_existing_->GetValue())
	{
		opt.mask_source     = SIFormat::Mask::Alpha;
		opt.alpha_threshold = slider_alpha_threshold_->GetValue();
	}
	else if (rb_transparency_colour_->GetValue())
	{
		opt.mask_source = SIFormat::Mask::Colour;
		opt.mask_colour = colbox_transparent_->colour();
	}
	else
		opt.mask_source = SIFormat::Mask::Brightness;

	// Set conversion palettes
	opt.pal_current = pal_chooser_current_->selectedPalette(items_[current_item_].entry);
	opt.pal_target  = pal_chooser_target_->selectedPalette(items_[current_item_].entry);

	// Set conversion colour format
	opt.col_format = current_format_.coltype;
}

// -----------------------------------------------------------------------------
// Returns true if the item at [index] has been modified, false otherwise
// -----------------------------------------------------------------------------
bool GfxConvDialog::itemModified(int index) const
{
	// Check index
	if (index < 0 || index >= static_cast<int>(items_.size()))
		return false;

	return items_[index].modified;
}

// -----------------------------------------------------------------------------
// Returns the image for the item at [index]
// -----------------------------------------------------------------------------
SImage* GfxConvDialog::itemImage(int index)
{
	// Check index
	if (index < 0 || index >= static_cast<int>(items_.size()))
		return nullptr;

	return &(items_[index].image);
}

// -----------------------------------------------------------------------------
// Returns the format for the item at [index]
// -----------------------------------------------------------------------------
SIFormat* GfxConvDialog::itemFormat(int index) const
{
	// Check index
	if (index < 0 || index >= static_cast<int>(items_.size()))
		return nullptr;

	return items_[index].new_format;
}

// -----------------------------------------------------------------------------
// Returns the palette for the item at [index]
// -----------------------------------------------------------------------------
const Palette* GfxConvDialog::itemPalette(int index) const
{
	// Check index
	if (index < 0 || index >= static_cast<int>(items_.size()))
		return nullptr;

	return items_[index].palette;
}

// -----------------------------------------------------------------------------
// Applies the conversion to the current image
// -----------------------------------------------------------------------------
void GfxConvDialog::applyConversion()
{
	// Get current item
	auto& item = items_[current_item_];

	// Write converted image data to it
	item.image.copyImage(&gfx_target_->image());

	// Update item info
	item.modified   = true;
	item.new_format = current_format_.format;
	item.palette    = pal_chooser_target_->selectedPalette(item.entry);
}


// -----------------------------------------------------------------------------
//
// GfxConvDialog Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the dialog is resized
// -----------------------------------------------------------------------------
void GfxConvDialog::onResize(wxSizeEvent& e)
{
	OnSize(e);

	gfx_current_->zoomToFit(true, 0.05);
	gfx_target_->zoomToFit(true, 0.05);

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the 'Convert' button is clicked
// -----------------------------------------------------------------------------
void GfxConvDialog::onBtnConvert(wxCommandEvent& e)
{
	applyConversion();
	nextItem();
}

// -----------------------------------------------------------------------------
// Called when the 'Convert All' button is clicked
// -----------------------------------------------------------------------------
void GfxConvDialog::onBtnConvertAll(wxCommandEvent& e)
{
	// Show splash window
	ui::showSplash("Converting Gfx...", true, maineditor::windowWx());

	// Convert all images
	for (size_t a = current_item_; a < items_.size(); a++)
	{
		applyConversion();
		if (!nextItem())
			break;
	}

	// Hide splash window
	ui::hideSplash();
}

// -----------------------------------------------------------------------------
// Called when the 'Skip' button is clicked
// -----------------------------------------------------------------------------
void GfxConvDialog::onBtnSkip(wxCommandEvent& e)
{
	nextItem();
}

// -----------------------------------------------------------------------------
// Called when the 'Skip All' button is clicked
// -----------------------------------------------------------------------------
void GfxConvDialog::onBtnSkipAll(wxCommandEvent& e)
{
	Close(true);
}

// -----------------------------------------------------------------------------
// Called when the 'Convert To' combo box is changed
// -----------------------------------------------------------------------------
void GfxConvDialog::onTargetFormatChanged(wxCommandEvent& e)
{
	current_format_ = conv_formats_[combo_target_format_->GetSelection()];
	updatePreviewGfx();
}

// -----------------------------------------------------------------------------
// Called when the current image palette chooser is changed
// -----------------------------------------------------------------------------
void GfxConvDialog::onCurrentPaletteChanged(wxCommandEvent& e)
{
	updatePreviewGfx();
}

// -----------------------------------------------------------------------------
// Called when the target image palette chooser is changed
// -----------------------------------------------------------------------------
void GfxConvDialog::onTargetPaletteChanged(wxCommandEvent& e)
{
	updatePreviewGfx();
}

// -----------------------------------------------------------------------------
// Called when the alpha threshold slider is changed
// -----------------------------------------------------------------------------
void GfxConvDialog::onAlphaThresholdChanged(wxCommandEvent& e)
{
	// Ignore while slider is being dragged
	if (e.GetEventType() == wxEVT_SCROLL_THUMBTRACK)
	{
		e.Skip();
		return;
	}

	updatePreviewGfx();
}

// -----------------------------------------------------------------------------
// Called when the 'enable transparency' checkbox is changed
// -----------------------------------------------------------------------------
void GfxConvDialog::onEnableTransparencyChanged(wxCommandEvent& e)
{
	updatePreviewGfx();
}

// -----------------------------------------------------------------------------
// Called when the 'existing' and 'colour' radio buttons are toggled
// -----------------------------------------------------------------------------
void GfxConvDialog::onTransTypeChanged(wxCommandEvent& e)
{
	updatePreviewGfx();
}

// -----------------------------------------------------------------------------
// Called when the transparent colour box is changed
// -----------------------------------------------------------------------------
void GfxConvDialog::onTransColourChanged(wxEvent& e)
{
	updatePreviewGfx();
}

// -----------------------------------------------------------------------------
// Called when the 'current' gfx preview is clicked
// -----------------------------------------------------------------------------
void GfxConvDialog::onPreviewCurrentMouseDown(wxMouseEvent& e)
{
	// Get image coordinates of the point clicked
	auto imgcoord = gfx_current_->imageCoords(e.GetX() * GetContentScaleFactor(), e.GetY() * GetContentScaleFactor());
	if (imgcoord.x < 0)
		return;

	// Get the colour at that point
	auto col = gfx_current_->image().pixelAt(imgcoord.x, imgcoord.y, gfx_current_->palette());

	// Set the background colour
	colbox_transparent_->setColour(col);
	updatePreviewGfx();
}

// -----------------------------------------------------------------------------
// Called when the 'Adjust Colorimetry Settings' button is clicked
// -----------------------------------------------------------------------------
void GfxConvDialog::onBtnColorimetrySettings(wxCommandEvent& e)
{
	ui::SettingsDialog::popupSettingsPage(this, ui::SettingsPage::Colorimetry);
	updatePreviewGfx();
}
