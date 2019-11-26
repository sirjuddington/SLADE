#pragma once

#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "UI/Browser/BrowserWindow.h"

class SLADEMap;
class Archive;

class MapTexBrowserItem : public BrowserItem
{
public:
	static const wxString TEXTURE;
	static const wxString FLAT;

	MapTexBrowserItem(const wxString& name, const wxString& type, unsigned index = 0);
	~MapTexBrowserItem() = default;

	bool     loadImage() override;
	wxString itemInfo() override;
	int      usageCount() const { return usage_count_; }
	void     setUsage(int count) { usage_count_ = count; }

private:
	int   usage_count_ = 0;
	Vec2d scale_       = { 1., 1. };
};

class MapTextureBrowser : public BrowserWindow
{
public:
	MapTextureBrowser(
		wxWindow*              parent,
		MapEditor::TextureType type    = MapEditor::TextureType::Texture,
		const wxString&        texture = "",
		SLADEMap*              map     = nullptr);
	~MapTextureBrowser() = default;

	wxString determineTexturePath(
		Archive*                    archive,
		MapTextureManager::Category category,
		const wxString&             type,
		const wxString&             path) const;
	void doSort(unsigned sort_type) override;
	void updateUsage() const;

private:
	MapEditor::TextureType type_ = MapEditor::TextureType::Texture;
	SLADEMap*              map_  = nullptr;
};
