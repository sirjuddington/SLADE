
#ifndef __DEFAULT_ENTRY_PANEL_H__
#define __DEFAULT_ENTRY_PANEL_H__

#include "EntryPanel.h"

class DefaultEntryPanel : public EntryPanel
{
private:
	vector<ArchiveEntry*>	entries;

	wxStaticText*	label_type;
	wxStaticText*	label_size;

	// Actions
	wxStaticBox*	frame_actions;
	wxButton*		btn_gfx_convert;
	wxButton*		btn_gfx_modify_offsets;
	wxButton*		btn_texture_edit;

public:
	DefaultEntryPanel(wxWindow* parent);
	~DefaultEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	loadEntries(vector<ArchiveEntry*>& entries);
	bool	saveEntry();

	void	onBtnGfxConvert(wxCommandEvent& e);
	void	onBtnGfxModifyOffsets(wxCommandEvent& e);
	void	onBtnTextureEdit(wxCommandEvent& e);
};

#endif//__DEFAULT_ENTRY_PANEL_H__
