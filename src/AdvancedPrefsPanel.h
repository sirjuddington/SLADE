
#ifndef __ADVANCED_PREFS_PANEL_H__
#define __ADVANCED_PREFS_PANEL_H__

#include "PrefsPanelBase.h"
#include <wx/propgrid/propgrid.h>

class AdvancedPrefsPanel : public PrefsPanelBase
{
private:
	wxPropertyGrid*	pg_cvars;

public:
	AdvancedPrefsPanel(wxWindow* parent);
	~AdvancedPrefsPanel();

	void	refreshPropGrid();
	void	init();
	void	applyPreferences();
};

#endif//__ADVANDED_PREFS_PANEL_H__
