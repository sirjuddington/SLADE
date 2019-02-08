#pragma once

#include "UI/WxUtils.h" // Will be needed for most prefs panels

class PrefsPanelBase : public wxPanel
{
public:
	PrefsPanelBase(wxWindow* parent) : wxPanel(parent, -1) {}
	~PrefsPanelBase() {}

	virtual void     init() {}
	virtual void     applyPreferences() {}
	virtual void     showSubSection(const wxString& subsection) {}
	virtual wxString pageTitle() { return ""; }
	virtual wxString pageDescription() { return ""; }
};
