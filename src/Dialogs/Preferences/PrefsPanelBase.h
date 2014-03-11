
#ifndef __PREFS_PANEL_BASE_H__
#define __PREFS_PANEL_BASE_H__

class PrefsPanelBase : public wxPanel
{
public:
	PrefsPanelBase(wxWindow* parent) : wxPanel(parent, -1) {}
	~PrefsPanelBase() {}

	virtual void init() {}
	virtual void applyPreferences() {}
};

#endif//__PREFS_PANEL_BASE_H__
