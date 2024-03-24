#pragma once

#include "MapEditor/MapEditor.h"
#include "UI/Browser/BrowserWindow.h"

namespace slade
{
enum class MapTextureCategory;

class MapTextureBrowser : public BrowserWindow
{
public:
	MapTextureBrowser(
		wxWindow*              parent,
		mapeditor::TextureType type    = mapeditor::TextureType::Texture,
		const wxString&        texture = "",
		SLADEMap*              map     = nullptr);
	~MapTextureBrowser() override = default;

	wxString determineTexturePath(
		const Archive*     archive,
		MapTextureCategory category,
		const wxString&    type,
		const wxString&    path) const;
	void doSort(unsigned sort_type) override;
	void updateUsage() const;

private:
	mapeditor::TextureType type_ = mapeditor::TextureType::Texture;
	SLADEMap*              map_  = nullptr;
};
} // namespace slade
