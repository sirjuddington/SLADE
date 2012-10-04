
#ifndef SWITCHESENTRYPANEL_H
#define SWITCHESENTRYPANEL_H

#include "MainApp.h"
#include "EntryPanel.h"
#include "BinaryControlLump.h"
#include "SwitchesList.h"
#include "GfxCanvas.h"
#include "ListView.h"

class SwitchesEntryPanel : public EntryPanel, public SActionHandler {
private:
	SwitchesList	switches;
	SwitchesEntry*	se_current;
	bool			se_modified;

	ListView*		list_entries;
	wxTextCtrl*		text_offname;
	wxTextCtrl*		text_onname;
	wxButton*		butn_new;
	wxButton*		butn_delete;
	GfxCanvas*		off_canvas;
	GfxCanvas*		on_canvas;
	wxRadioButton*	rbtn_shareware;
	wxRadioButton*	rbtn_registered;
	wxRadioButton*	rbtn_commercial;

public:
	SwitchesEntryPanel(wxWindow* parent);
	~SwitchesEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();
	bool	revertEntry();

	void	populateEntryList();
	void	updateControls();

	void	applyChanges();

	void	insertListItem(SwitchesEntry* ent, uint32_t pos);
	void	updateListItem(SwitchesEntry* ent, uint32_t pos);
	void	moveUp();
	void	moveDown();
	void	remove();
	void	add();

	// SAction handler
	bool	handleAction(string id);

	// Events
	void	onListSelect(wxListEvent& e);
	void	onListRightClick(wxListEvent& e);
	void	onTypeChanged(wxCommandEvent& e);
	void	onOffNameChanged(wxCommandEvent& e);
	void	onOnNameChanged(wxCommandEvent& e);
	void	onBtnNew(wxCommandEvent& e);};

#endif//SWITCHESENTRYPANEL_H
