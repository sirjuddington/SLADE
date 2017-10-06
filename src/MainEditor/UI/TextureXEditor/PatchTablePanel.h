#pragma once

#include "common.h"
#include "Graphics/CTexture/PatchTable.h"
#include "UI/Lists/VirtualListView.h"

class GfxCanvas;
class SZoomSlider;
class TextureXEditor;

class PatchTableListView : public VirtualListView, Listener
{
protected:
	string	getItemText(long item, long column, long index) const override;
	void	updateItemAttr(long item, long column, long index) const override;

public:
	PatchTableListView(wxWindow* parent, PatchTable* patch_table);
	~PatchTableListView() {}

	PatchTable*	patchTable() const { return patch_table_; }

	void		updateList(bool clear = false) override;
	void		onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) override;
	static bool usageSort(long left, long right);
	void		sortItems() override;

private:
	PatchTable*	patch_table_ = nullptr;
};


class PatchTablePanel : public wxPanel, public Listener
{
public:
	PatchTablePanel(wxWindow* parent, PatchTable* patch_table, TextureXEditor* tx_editor = nullptr);
	~PatchTablePanel() {}

private:
	PatchTable*			patch_table_			= nullptr;
	PatchTableListView*	list_patches_			= nullptr;
	wxButton*			btn_add_patch_			= nullptr;
	wxButton*			btn_patch_from_file_	= nullptr;
	wxButton*			btn_remove_patch_		= nullptr;
	wxButton*			btn_change_patch_		= nullptr;
	wxButton*			btn_import_patch_file_	= nullptr;
	TextureXEditor*		parent_					= nullptr;
	GfxCanvas*			patch_canvas_			= nullptr;
	wxStaticText*		label_dimensions_		= nullptr;
	wxStaticText*		label_textures_			= nullptr;
	SZoomSlider*		slider_zoom_			= nullptr;

	void	setupLayout();

	// Events
	void	onBtnAddPatch(wxCommandEvent& e);
	void	onBtnPatchFromFile(wxCommandEvent& e);
	void	onBtnRemovePatch(wxCommandEvent& e);
	void	onBtnChangePatch(wxCommandEvent& e);
	void	onDisplayChanged(wxCommandEvent& e);
	void	updateDisplay();
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) override;
};
