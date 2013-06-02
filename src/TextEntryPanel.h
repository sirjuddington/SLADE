
#ifndef __TEXTENTRYPANEL_H__
#define	__TEXTENTRYPANEL_H__

#include "EntryPanel.h"
#include "TextEditor.h"
#include <wx/choice.h>

class TextEntryPanel : public EntryPanel
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

	// Events
	void	onTextModified(wxStyledTextEvent& e);
	void	onBtnFindReplace(wxCommandEvent& e);
	void	onChoiceLanguageChanged(wxCommandEvent& e);
	void	onUpdateUI(wxStyledTextEvent& e);
	void	onWordWrapChanged(wxCommandEvent& e);
	void	onBtnJumpTo(wxCommandEvent& e);
};


#endif //__TEXTENTRYPANEL_H__
