#pragma once

#include "EntryPanel.h"
#include "Graphics/Translation.h"
#include "UI/Canvas/GfxCanvas.h"

class SZoomSlider;
class ColourBox;

class GfxEntryPanel : public EntryPanel
{
public:
	GfxEntryPanel(wxWindow* parent);
	~GfxEntryPanel() = default;

	Translation& prevTranslation() { return prev_translation_; }

	bool            saveEntry() override;
	void            setupToolbar();
	void            fillBrushMenu(wxMenu* bm) const;
	void            updateImagePalette() const;
	GfxCanvas::View detectOffsetType() const;
	void            applyViewType() const;
	void            refresh(ArchiveEntry* entry = nullptr);
	void            refreshPanel() override;
	wxString        statusString() override;
	bool            extractAll() const;

	// SAction handler
	bool handleEntryPanelAction(string_view id) override;
	bool fillCustomMenu(wxMenu* custom) override;

	SImage* image() const
	{
		if (gfx_canvas_)
			return &gfx_canvas_->image();
		else
			return nullptr;
	}

protected:
	bool loadEntry(ArchiveEntry* entry) override;
	bool loadEntry(ArchiveEntry* entry, int index);

private:
	bool        alph_                = false;
	bool        trns_                = false;
	bool        image_data_modified_ = false;
	int         cur_index_           = 0;
	bool        editing_             = false;
	Translation prev_translation_;
	Translation edit_translation_;

	GfxCanvas*      gfx_canvas_         = nullptr;
	SZoomSlider*    slider_zoom_        = nullptr;
	ColourBox*      cb_colour_          = nullptr;
	wxChoice*       choice_offset_type_ = nullptr;
	wxSpinCtrl*     spin_xoffset_       = nullptr;
	wxSpinCtrl*     spin_yoffset_       = nullptr;
	wxCheckBox*     cb_tile_            = nullptr;
	wxCheckBox*     cb_arc_             = nullptr;
	wxButton*       btn_auto_offset_    = nullptr;
	wxButton*       btn_nextimg_        = nullptr;
	wxButton*       btn_previmg_        = nullptr;
	wxStaticText*   text_curimg_        = nullptr;
	SToolBarButton* button_brush_       = nullptr;
	wxMenu*         menu_brushes_       = nullptr;

	// Signal connections
	sigslot::scoped_connection sc_palette_changed_;

	// Events
	void onPaintColourChanged(wxEvent& e);
	void onXOffsetChanged(wxCommandEvent& e);
	void onYOffsetChanged(wxCommandEvent& e);
	void onOffsetTypeChanged(wxCommandEvent& e);
	void onTileChanged(wxCommandEvent& e);
	void onARCChanged(wxCommandEvent& e);
	void onGfxOffsetChanged(wxEvent& e);
	void onGfxPixelsChanged(wxEvent& e);
	void onBtnNextImg(wxCommandEvent& e);
	void onBtnPrevImg(wxCommandEvent& e);
	void onBtnAutoOffset(wxCommandEvent& e);
	void onColourPicked(wxEvent& e);
};
