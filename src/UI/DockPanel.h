
#ifndef __DOCK_PANEL_H__
#define __DOCK_PANEL_H__

#include <wx/panel.h>

class DockPanel : public wxPanel
{
protected:
	uint8_t	current_layout;	// 0=normal, 1=horizontal, 2=vertical

public:
	DockPanel(wxWindow* parent);
	~DockPanel();

	virtual void	layoutNormal() {}
	virtual void	layoutVertical() { layoutNormal(); }
	virtual void	layoutHorizontal() { layoutNormal(); }

	// Events
	void	onSize(wxSizeEvent& e);
};

#endif//__DOCK_PANEL_H__
