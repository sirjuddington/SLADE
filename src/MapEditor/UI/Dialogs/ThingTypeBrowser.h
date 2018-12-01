#pragma once

#include "UI/Browser/BrowserWindow.h"

namespace Game
{
class ThingType;
}

class ThingBrowserItem : public BrowserItem
{
public:
	ThingBrowserItem(string name, const Game::ThingType& type, unsigned index) :
		BrowserItem{ name, index },
		type_{ type }
	{
	}
	~ThingBrowserItem() {}

	bool loadImage() override;

private:
	Game::ThingType const& type_;
};

class ThingTypeBrowser : public BrowserWindow
{
public:
	ThingTypeBrowser(wxWindow* parent, int type = -1);
	~ThingTypeBrowser() {}

	void setupViewOptions();
	int  selectedType();

private:
	wxCheckBox* cb_view_tiles_;

	// Events
	void onViewTilesClicked(wxCommandEvent& e);
};
