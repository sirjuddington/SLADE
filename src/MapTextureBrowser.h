
#ifndef __MAP_TEXTURE_BROWSER_H__
#define __MAP_TEXTURE_BROWSER_H__

#include "UI/Browser/BrowserWindow.h"

class SLADEMap;
class Archive;

class MapTexBrowserItem : public BrowserItem
{
private:
	int	usage_count;

public:
	MapTexBrowserItem(string name, int type, unsigned index = 0);
	~MapTexBrowserItem();

	bool	loadImage();
	string	itemInfo();
	int		usageCount() { return usage_count; }
	void	setUsage(int count) { usage_count = count; }
};

class MapTextureBrowser : public BrowserWindow
{
private:
	int			type;
	SLADEMap*	map;

public:
	MapTextureBrowser(wxWindow* parent, int type = 0, string texture = "", SLADEMap* map = NULL);
	~MapTextureBrowser();

	string	determineTexturePath(Archive* archive, uint8_t category, string type, string path);
	void	doSort(unsigned sort_type);
	void	updateUsage();
};

#endif//__MAP_TEXTURE_BROWSER_H__
