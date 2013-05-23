
#ifndef __MAP_TEXTURE_BROWSER_H__
#define __MAP_TEXTURE_BROWSER_H__

#include "BrowserWindow.h"

class MapTexBrowserItem : public BrowserItem
{
private:

public:
	MapTexBrowserItem(string name, int type, unsigned index = 0);
	~MapTexBrowserItem();

	bool	loadImage();
	string	itemInfo();
};

class MapTextureBrowser : public BrowserWindow
{
private:
	int	type;

public:
	MapTextureBrowser(wxWindow* parent, int type = 0, string texture = "");
	~MapTextureBrowser();
};

#endif//__MAP_TEXTURE_BROWSER_H__
