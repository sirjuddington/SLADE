
#ifndef __TEXTENTRYPANEL_H__
#define	__TEXTENTRYPANEL_H__

#include "EntryPanel.h"
#include "General/SAction.h"
#include "MainEditor/EntryOperations.h"

class TextEditorCtrl;
class FindReplacePanel;

class TextEntryPanel : public EntryPanel, SActionHandler
{
public:
	TextEntryPanel(wxWindow* parent);
	~TextEntryPanel();

	bool	loadEntry(ArchiveEntry* entry) override;
	bool	saveEntry() override;
	void	refreshPanel() override;
	void	closeEntry() override;
	string	statusString() override;
	bool	undo() override;
	bool	redo() override;

	// SAction Handler
	bool	handleAction(string id) override;

private:
	TextEditorCtrl*		text_area_;
	FindReplacePanel*	panel_fr_;
	wxButton*			btn_find_replace_;
	wxChoice*			choice_text_language_;
	wxCheckBox*			cb_wordwrap_;
	wxButton*			btn_jump_to_;
	wxChoice*			choice_jump_to_;

	// Events
	void	onTextModified(wxCommandEvent& e);
	void	onChoiceLanguageChanged(wxCommandEvent& e);
	void	onUpdateUI(wxStyledTextEvent& e);
};


#endif //__TEXTENTRYPANEL_H__
