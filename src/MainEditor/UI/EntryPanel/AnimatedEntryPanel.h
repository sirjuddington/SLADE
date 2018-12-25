
#ifndef ANIMATEDENTRYPANEL_H
#define ANIMATEDENTRYPANEL_H

#include "EntryPanel.h"
#include "General/SAction.h"
#include "MainEditor/AnimatedList.h"
#include "UI/Lists/ListView.h"

class GfxCanvas;
class AnimatedEntryPanel : public EntryPanel
{
private:
	AnimatedList	animated;
	AnimatedEntry*	ae_current;
	bool			ae_modified;

	ListView*		list_entries;
	wxTextCtrl*		text_firstname;
	wxTextCtrl*		text_lastname;
	wxTextCtrl*		text_speed;
	wxRadioButton*	rbtn_flat;
	wxRadioButton*	rbtn_texture;
	wxCheckBox*		cbox_decals;
	wxCheckBox*		cbox_swirl;
	wxButton*		butn_delete;
	GfxCanvas*		preview_canvas;

public:
	AnimatedEntryPanel(wxWindow* parent);
	~AnimatedEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();
	bool	revertEntry();

	void	populateEntryList();
	void	updateControls();

	void	applyChanges();

	void	insertListItem(AnimatedEntry* ent, uint32_t pos);
	void	updateListItem(AnimatedEntry* ent, uint32_t pos);
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
	void	onSwirlChanged(wxCommandEvent& e);
	void	onDecalsChanged(wxCommandEvent& e);
	void	onFirstNameChanged(wxCommandEvent& e);
	void	onLastNameChanged(wxCommandEvent& e);
	void	onSpeedChanged(wxCommandEvent& e);
};

#endif//ANIMATEDENTRYPANEL_H
