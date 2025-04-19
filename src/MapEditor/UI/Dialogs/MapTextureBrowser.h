#pragma once

#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "UI/Browser/BrowserWindow.h"

namespace slade
{
class SLADEMap;
class Archive;

class MapTexBrowserItem : public BrowserItem
{
public:
	static const string TEXTURE;
	static const string FLAT;

	MapTexBrowserItem(const string& name, const string& type, unsigned index = 0);
	~MapTexBrowserItem() = default;

	bool   loadImage() override;
	string itemInfo() override;
	int    usageCount() const { return usage_count_; }
	void   setUsage(int count) { usage_count_ = count; }

private:
	int   usage_count_ = 0;
	Vec2d scale_       = { 1., 1. };
};

class MapTextureBrowser : public BrowserWindow
{
public:
	MapTextureBrowser(
		wxWindow*              parent,
		mapeditor::TextureType type    = mapeditor::TextureType::Texture,
		string_view            texture = "",
		SLADEMap*              map     = nullptr);
	~MapTextureBrowser() = default;

	string determineTexturePath(
		Archive*                    archive,
		MapTextureManager::Category category,
		const string&               type,
		const string&               path) const;
	void doSort(unsigned sort_type) override;
	void updateUsage() const;

private:
	mapeditor::TextureType type_ = mapeditor::TextureType::Texture;
	SLADEMap*              map_  = nullptr;
};
} // namespace slade
