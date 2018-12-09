#pragma once

class PrefsPanelBase : public wxPanel
{
public:
	PrefsPanelBase(wxWindow* parent) : wxPanel(parent, -1) {}
	~PrefsPanelBase() {}

	virtual void   init() {}
	virtual void   applyPreferences() {}
	virtual void   showSubSection(const string& subsection) {}
	virtual string pageTitle() { return ""; }
	virtual string pageDescription() { return ""; }
};
