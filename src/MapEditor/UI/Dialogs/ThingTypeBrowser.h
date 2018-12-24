#pragma once

#include "UI/Browser/BrowserWindow.h"

namespace Game
{
class ThingType;
}

class ThingBrowserItem : public BrowserItem
{
public:
	ThingBrowserItem(const string& name, const Game::ThingType& type, unsigned index) :
		BrowserItem{ name, index },
		type_{ type }
	{
	}
	~ThingBrowserItem() = default;

	bool loadImage() override;

private:
	Game::ThingType const& type_;
};

class ThingTypeBrowser : public BrowserWindow
{
public:
	ThingTypeBrowser(wxWindow* parent, int type = -1);
	~ThingTypeBrowser() = default;

	void setupViewOptions();
	int  selectedType();

private:
	wxCheckBox* cb_view_tiles_ = nullptr;

	// Events
	void onViewTilesClicked(wxCommandEvent& e);
};
