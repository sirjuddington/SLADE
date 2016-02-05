
#ifndef __THING_TYPE_BROWSER_H__
#define __THING_TYPE_BROWSER_H__

#include "UI/Browser/BrowserWindow.h"

class ThingType;
class ThingBrowserItem : public BrowserItem
{
private:
	ThingType*	type;

public:
	ThingBrowserItem(string name, ThingType* type, unsigned index);
	~ThingBrowserItem();

	bool	loadImage();
};

class ThingTypeBrowser : public BrowserWindow
{
private:
	wxCheckBox*	cb_view_tiles;

public:
	ThingTypeBrowser(wxWindow* parent, int type = -1);
	~ThingTypeBrowser();

	void	setupViewOptions();
	int		getSelectedType();

	// Events
	void	onViewTilesClicked(wxCommandEvent& e);
};

#endif//__THING_TYPE_BROWSER_H__
