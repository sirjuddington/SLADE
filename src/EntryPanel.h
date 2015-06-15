
#ifndef __ENTRYPANEL_H__
#define __ENTRYPANEL_H__

#include "ArchiveEntry.h"
#include "ListenerAnnouncer.h"
#include "UndoRedo.h"
#include "SToolBar.h"

class SToolBarButton;
class EntryPanel : public wxPanel, public Listener
{
private:
	bool			modified;
	wxStaticBox*	frame;
	string			id;

protected:
	MemChunk		entry_data;
	ArchiveEntry*	entry;
	UndoManager*	undo_manager;

	wxSizer*		sizer_main;
	wxSizer*		sizer_top;
	wxSizer*		sizer_bottom;
	SToolBarButton*	stb_save;
	SToolBarButton*	stb_revert;

	wxMenu*			menu_custom;
	string			custom_menu_name;
	string			custom_toolbar_actions;	// A simple list of action ids separated by ;
	SToolBar*		toolbar;

	void	setModified(bool c = true);

public:
	EntryPanel(wxWindow* parent, string id);
	~EntryPanel();

	string			getName() { return id; }
	ArchiveEntry*	getEntry() { return entry; }
	bool			isModified() { return modified; }
	bool			isActivePanel();
	void			setUndoManager(UndoManager* manager) { undo_manager = manager; }
	MemChunk*		getEntryData() { return &entry_data; }

	bool			openEntry(ArchiveEntry* entry);
	virtual bool	loadEntry(ArchiveEntry* entry);
	virtual bool	saveEntry();
	virtual bool	revertEntry();
	virtual void	refreshPanel();
	virtual void	closeEntry();
	void			updateStatus();
	virtual string	statusString() { return ""; }
	virtual void	addCustomMenu();
	void			removeCustomMenu();
	virtual void	addCustomToolBar();
	void			removeCustomToolBar();
	virtual bool	fillCustomMenu(wxMenu* custom) { return false; }
	string			getCustomMenuName() { return custom_menu_name; }
	void			callRefresh() { refreshPanel(); }
	void			nullEntry() { entry = NULL; }
	virtual bool	undo() { return false; }
	virtual bool	redo() { return false; }
	void			updateToolbar();
	virtual void	toolbarButtonClick(string action_id) {}

	virtual void	onBtnSave(wxCommandEvent& e);
	virtual void	onBtnRevert(wxCommandEvent& e);
	virtual void	onBtnEditExt(wxCommandEvent& e);
	virtual void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) {}
	void			onToolbarButton(wxCommandEvent& e);
};

#endif //__ENTRYPANEL_H__
