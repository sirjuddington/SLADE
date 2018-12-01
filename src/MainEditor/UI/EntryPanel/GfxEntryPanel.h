#pragma once

#include "EntryPanel.h"
#include "General/SAction.h"
#include "Graphics/Translation.h"
#include "UI/Canvas/GfxCanvas.h"

class SZoomSlider;
class ColourBox;

class GfxEntryPanel : public EntryPanel, public SActionHandler
{
public:
	GfxEntryPanel(wxWindow* parent);
	~GfxEntryPanel() {}

	Translation& prevTranslation() { return prev_translation_; }

	bool            loadEntry(ArchiveEntry* entry) override;
	bool            loadEntry(ArchiveEntry* entry, int index);
	bool            saveEntry() override;
	void            setupToolbar();
	void            fillBrushMenu(wxMenu* bm);
	void            updateImagePalette();
	GfxCanvas::View detectOffsetType();
	void            applyViewType();
	void            refresh();
	void            refreshPanel() override;
	string          statusString() override;
	bool            extractAll();

	// SAction handler
	bool handleAction(string id) override;
	bool fillCustomMenu(wxMenu* custom) override;

	void onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) override;

	SImage* image() const
	{
		if (gfx_canvas_)
			return gfx_canvas_->getImage();
		else
			return nullptr;
	}

private:
	bool        alph_                = false;
	bool        trns_                = false;
	bool        image_data_modified_ = false;
	int         cur_index_           = 0;
	bool        offset_changing_     = false;
	bool        editing_             = false;
	Translation prev_translation_;
	Translation edit_translation_;

	GfxCanvas*      gfx_canvas_         = nullptr;
	SZoomSlider*    slider_zoom_        = nullptr;
	ColourBox*      cb_colour_          = nullptr;
	wxChoice*       choice_offset_type_ = nullptr;
	wxChoice*       choice_brush_       = nullptr;
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
