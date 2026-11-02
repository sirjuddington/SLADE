#pragma once

#include "UI/Browser/BrowserWindow.h"

namespace slade
{
namespace game
{
	class ThingType;
}

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
