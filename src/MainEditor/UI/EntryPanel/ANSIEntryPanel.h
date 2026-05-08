#pragma once

#include "EntryPanel.h"

// Forward declarations
namespace slade
{
class ColourPickerPanel;
class CharMapPanel;
namespace ui
{
	class ANSICanvas;
}
namespace gfx
{
	class ANSIScreen;
}
} // namespace slade

namespace slade
{
class ANSIEntryPanel : public EntryPanel
{
public:
	ANSIEntryPanel(wxWindow* parent);
	~ANSIEntryPanel() override = default;

	static constexpr int DATASIZE = 4000;

protected:
	bool loadEntry(ArchiveEntry* entry) override;
	bool writeEntry(ArchiveEntry& entry) override;

	void toolbarButtonClick(const string& action_id) override;

private:
	enum class Mode : u8
	{
		View,
		Edit,
		Paint,
		Text
	};

	unique_ptr<gfx::ANSIScreen> ansi_screen_;
	ui::ANSICanvas*             ansi_canvas_     = nullptr;
	wxPanel*                    sidebar_         = nullptr;
	ColourPickerPanel*          fg_picker_       = nullptr;
	ColourPickerPanel*          bg_picker_       = nullptr;
	CharMapPanel*               char_map_        = nullptr;
	wxStaticText*               label_character_ = nullptr;
	bool                        updating_        = false;
	bool                        dragging_        = false;
	unsigned                    last_selected_   = UINT_MAX;
	unsigned                    drag_anchor_     = UINT_MAX;
	bool                        drag_ctrl_       = false;
	Mode                        mode_            = Mode::View;
	int                         paint_fg_        = -1;
	int                         paint_bg_        = -1;
	int                         paint_char_      = -1;
	int                         text_fg_         = -1;
	int                         text_bg_         = -1;

	void updateControlsForSelection();

	void onCanvasMouseDown(wxMouseEvent& e);
	void onCanvasMouseMove(wxMouseEvent& e);
	void onCanvasMouseUp(wxMouseEvent& e) { dragging_ = false; }
	void onCanvasKeyDown(wxKeyEvent& e);
	void onCanvasChar(wxKeyEvent& e);
	void onCharMapPicked(wxCommandEvent& e);
	void onForegroundColourPicked(wxCommandEvent& e);
	void onBackgroundColourPicked(wxCommandEvent& e);
};
} // namespace slade
