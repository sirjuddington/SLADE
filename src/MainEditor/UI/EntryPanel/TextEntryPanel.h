
#ifndef __TEXTENTRYPANEL_H__
#define	__TEXTENTRYPANEL_H__

#include "EntryPanel.h"
#include "General/SAction.h"
#include "UI/TextEditor/TextEditor.h"

class TextEntryPanel : public EntryPanel, SActionHandler
{
private:
	TextEditor*	text_area;
	wxButton*	btn_find_replace;
	wxChoice*	choice_text_language;
	wxCheckBox*	cb_wordwrap;
	wxButton*	btn_jump_to;

public:
	TextEntryPanel(wxWindow* parent);
	~TextEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();
	void	refreshPanel();
	void	closeEntry();
	string	statusString();
	bool	undo();
	bool	redo();

	// SAction Handler
	bool	handleAction(string id);

	// Events
	void	onTextModified(wxStyledTextEvent& e);
	void	onChoiceLanguageChanged(wxCommandEvent& e);
	void	onUpdateUI(wxStyledTextEvent& e);
};


#endif //__TEXTENTRYPANEL_H__
