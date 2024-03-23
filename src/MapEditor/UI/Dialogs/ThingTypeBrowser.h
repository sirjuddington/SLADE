#pragma once

#include "UI/Browser/BrowserItem.h"
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
	ThingBrowserItem(const wxString& name, const game::ThingType& type, unsigned index) :
		BrowserItem{ name, index },
		thing_type_{ &type }
	{
	}
	~ThingBrowserItem() override = default;

	bool loadImage() override;

private:
	game::ThingType const* thing_type_;
};

class ThingTypeBrowser : public BrowserWindow
{
public:
	ThingTypeBrowser(wxWindow* parent, int type = -1);
	~ThingTypeBrowser() override = default;

	void setupViewOptions();
	int  selectedType() const;

private:
	wxCheckBox* cb_view_tiles_ = nullptr;

	// Events
	void onViewTilesClicked(wxCommandEvent& e);
};
} // namespace slade
