
#ifndef __S_DIALOG_H__
#define __S_DIALOG_H__

class SDialog : public wxDialog
{
private:
	string	id;

public:
	SDialog(wxWindow* parent, string title, string id, int width = -1, int height = -1, int x = -1, int y = -1);
	virtual ~SDialog();

	void	onSize(wxSizeEvent& e);
	void	onMove(wxMoveEvent& e);
};

#endif//__S_DIALOG_H__
