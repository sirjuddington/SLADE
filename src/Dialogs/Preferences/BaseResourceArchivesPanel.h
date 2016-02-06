
#ifndef __BASERESOURCEARCHIVESPANEL__
#define __BASERESOURCEARCHIVESPANEL__

#include <wx/panel.h>

class wxListBox;
class wxButton;
class BaseResourceArchivesPanel : public wxPanel
{
private:
	wxListBox*	list_base_archive_paths;
	wxButton*	btn_add;
	wxButton*	btn_remove;
	wxButton*	btn_detect;

public:
	BaseResourceArchivesPanel(wxWindow* parent);
	~BaseResourceArchivesPanel();

	int		getSelectedPath();
	void	autodetect();

	void	onBtnAdd(wxCommandEvent& e);
	void	onBtnRemove(wxCommandEvent& e);
	void	onBtnDetect(wxCommandEvent& e);
};

#endif//__BASERESOURCEARCHIVESPANEL__
