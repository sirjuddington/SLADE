
#ifndef __THING_TYPE_BROWSER_H__
#define __THING_TYPE_BROWSER_H__

#include "UI/Browser/BrowserWindow.h"

namespace Game { class ThingType; }

class ThingBrowserItem : public BrowserItem
{
private:
	Game::ThingType const&	type;

public:
	ThingBrowserItem(string name, const Game::ThingType& type, unsigned index);
	~ThingBrowserItem();

	bool	loadImage();
};

class wxCheckBox;
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
