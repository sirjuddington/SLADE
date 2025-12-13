#pragma once

#include "EntryPanel.h"

namespace slade
{
namespace ui
{
	class ANSICanvas;
}
namespace gfx
{
	class ANSIScreen;
}

class ANSIEntryPanel : public EntryPanel
{
public:
	ANSIEntryPanel(wxWindow* parent);
	~ANSIEntryPanel() override = default;

	static constexpr int DATASIZE = 4000;

protected:
	bool loadEntry(ArchiveEntry* entry) override;
	bool writeEntry(ArchiveEntry& entry) override;

private:
	unique_ptr<gfx::ANSIScreen> ansi_screen_;
	ui::ANSICanvas*             ansi_canvas_   = nullptr;
	wxChoice*                   fg_choice_     = nullptr;
	wxChoice*                   bg_choice_     = nullptr;
	bool                        updating_      = false;
	bool                        dragging_      = false;
	unsigned                    last_selected_ = UINT_MAX;
	unsigned                    drag_anchor_   = UINT_MAX;
	bool                        drag_ctrl_     = false;

	void updateControls();

	void onCanvasMouseDown(wxMouseEvent& e);
	void onCanvasMouseMove(wxMouseEvent& e);
	void onCanvasMouseUp(wxMouseEvent& e) { dragging_ = false; }
	void onCanvasKeyDown(wxKeyEvent& e);
	void onCanvasChar(wxKeyEvent& e);
	void onCharMapPicked(wxCommandEvent& e);
};
} // namespace slade
