
#ifndef __S_DIALOG_H__
#define __S_DIALOG_H__

#include <wx/dialog.h>

class SDialog : public wxDialog
{
private:
	string	id;

public:
	SDialog(wxWindow* parent, string title, string id, int width = -1, int height = -1, int x = -1, int y = -1);
	virtual ~SDialog();

	void	setSavedSize(int def_width = -1, int def_height = -1);

	void	onSize(wxSizeEvent& e);
	void	onMove(wxMoveEvent& e);
	void	onShow(wxShowEvent& e);
};

#endif//__S_DIALOG_H__
