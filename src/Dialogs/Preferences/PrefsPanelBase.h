
#ifndef __PREFS_PANEL_BASE_H__
#define __PREFS_PANEL_BASE_H__

#include "common.h"
#include "UI/WxBasicControls.h"

class PrefsPanelBase : public wxPanel
{
public:
	PrefsPanelBase(wxWindow* parent) : wxPanel(parent, -1) {}
	~PrefsPanelBase() {}

	virtual void init() {}
	virtual void applyPreferences() {}
	virtual void showSubSection(string subsection) {}
};

#endif//__PREFS_PANEL_BASE_H__
