
#ifndef SWITCHESENTRYPANEL_H
#define SWITCHESENTRYPANEL_H

#include "General/SAction.h"
#include "EntryPanel.h"
#include "MainEditor/BinaryControlLump.h"
#include "MainEditor/SwitchesList.h"
#include "UI/Lists/ListView.h"

class SwitchesEntryPanel : public EntryPanel, public SActionHandler
{
private:
	SwitchesList	switches;
	SwitchesEntry*	se_current;
	bool			se_modified;

	ListView*		list_entries;
	wxTextCtrl*		text_offname;
	wxTextCtrl*		text_onname;
	wxButton*		butn_delete;
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
	void	toolbarButtonClick(string action_id);

	// SAction handler
	bool	handleAction(string id);

	// Events
	void	onListSelect(wxListEvent& e);
	void	onListRightClick(wxListEvent& e);
	void	onTypeChanged(wxCommandEvent& e);
	void	onOffNameChanged(wxCommandEvent& e);
	void	onOnNameChanged(wxCommandEvent& e);
};

#endif//SWITCHESENTRYPANEL_H
