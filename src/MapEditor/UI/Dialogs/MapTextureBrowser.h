#pragma once

#include "MapEditor/MapTextureManager.h"
#include "UI/Browser/BrowserWindow.h"

class SLADEMap;
class Archive;

class MapTexBrowserItem : public BrowserItem
{
public:
	MapTexBrowserItem(string name, int type, unsigned index = 0);
	~MapTexBrowserItem();

	bool   loadImage();
	string itemInfo();
	int    usageCount() { return usage_count_; }
	void   setUsage(int count) { usage_count_ = count; }

private:
	int usage_count_;
};

class MapTextureBrowser : public BrowserWindow
{
public:
	MapTextureBrowser(wxWindow* parent, int type = 0, string texture = "", SLADEMap* map = nullptr);
	~MapTextureBrowser();

	string determineTexturePath(Archive* archive, MapTextureManager::Category category, string type, string path);
	void   doSort(unsigned sort_type);
	void   updateUsage();

private:
	int       type_;
	SLADEMap* map_;
};
