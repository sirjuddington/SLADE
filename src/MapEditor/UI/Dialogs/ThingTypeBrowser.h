#pragma once

#include "UI/Browser/BrowserWindow.h"

namespace slade
{
namespace game
{
	class ThingType;
}

class ThingBrowserItem : public BrowserItem
{
public:
	ThingBrowserItem(const string& name, const game::ThingType& type, unsigned index) :
		BrowserItem{ name, index },
		type_{ type }
	{
	}
	~ThingBrowserItem() = default;

	bool loadImage() override;

private:
	game::ThingType const& type_;
};

class ThingTypeBrowser : public BrowserWindow
{
public:
	ThingTypeBrowser(wxWindow* parent, int type = -1);
	~ThingTypeBrowser() = default;

	void setupViewOptions();
	int  selectedType() const;

private:
	wxCheckBox* cb_view_tiles_ = nullptr;

	// Events
	void onViewTilesClicked(wxCommandEvent& e);
};
} // namespace slade
