
#ifndef __EXT_MESSAGE_DIALOG_H__
#define __EXT_MESSAGE_DIALOG_H__

#include <wx/dialog.h>

class ExtMessageDialog : public wxDialog {
private:
	wxStaticText*	label_message;
	wxTextCtrl*		text_ext;

public:
	ExtMessageDialog(wxWindow* parent, string caption);
	~ExtMessageDialog();

	void	setMessage(string message);
	void	setExt(string text);

	// Events
	void	onSize(wxSizeEvent& e);
};

#endif//__EXT_MESSAGE_DIALOG_H__
