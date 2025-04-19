#pragma once

#include "EntryPanel.h"
#include "Graphics/Translation.h"
#include "UI/Canvas/GfxCanvas.h"

namespace slade
{
class ColourBox;
namespace ui
{
	class ZoomControl;
}

class GfxEntryPanel : public EntryPanel
{
public:
	GfxEntryPanel(wxWindow* parent);
	~GfxEntryPanel() override = default;

	Translation& prevTranslation() { return prev_translation_; }

	void            setupToolbars();
	void            fillBrushMenu(wxMenu* bm) const;
	void            updateImagePalette() const;
	GfxCanvas::View detectOffsetType(ArchiveEntry* entry) const;
	void            applyViewType(ArchiveEntry* entry) const;
	void            refresh(ArchiveEntry* entry = nullptr);
	void            refreshPanel() override;
	string          statusString() override;
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
	bool writeEntry(ArchiveEntry& entry) override;

private:
	bool        alph_                = false;
	bool        trns_                = false;
	bool        image_data_modified_ = false;
	int         cur_index_           = 0;
	bool        editing_             = false;
	Translation prev_translation_;
	Translation edit_translation_;

	GfxCanvas*       gfx_canvas_         = nullptr;
	ColourBox*       cb_colour_          = nullptr;
	wxChoice*        choice_offset_type_ = nullptr;
	wxSpinCtrl*      spin_xoffset_       = nullptr;
	wxSpinCtrl*      spin_yoffset_       = nullptr;
	wxButton*        btn_auto_offset_    = nullptr;
	wxSpinCtrl*      spin_curimg_        = nullptr;
	wxStaticText*    text_imgnum_        = nullptr;
	wxStaticText*    text_imgoutof_      = nullptr;
	SToolBarButton*  button_brush_       = nullptr;
	wxMenu*          menu_brushes_       = nullptr;
	SToolBarButton*  btn_arc_            = nullptr;
	SToolBarButton*  btn_tile_           = nullptr;
	ui::ZoomControl* zc_zoom_            = nullptr;

	// Toolbar
	void toolbarButtonClick(const string& action_id) override;

	// Signal connections
	sigslot::scoped_connection sc_palette_changed_;

	// Events
	void onPaintColourChanged(wxEvent& e);
	void onXOffsetChanged(wxCommandEvent& e);
	void onYOffsetChanged(wxCommandEvent& e);
	void onOffsetTypeChanged(wxCommandEvent& e);
	void onGfxOffsetChanged(wxEvent& e);
	void onGfxPixelsChanged(wxEvent& e);
	void onCurImgChanged(wxCommandEvent& e);
	void onBtnAutoOffset(wxCommandEvent& e);
	void onColourPicked(wxEvent& e);
	void onToolSelected(wxCommandEvent& e);
};
} // namespace slade
