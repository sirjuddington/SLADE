#pragma once

#include "SettingsPanel.h"

class wxPropertyGrid;

namespace slade::colourconfig
{
struct Colour;
}

namespace slade::ui
{
class ColourSettingsPanel : public SettingsPanel
{
public:
	ColourSettingsPanel(wxWindow* parent);
	~ColourSettingsPanel() override = default;

	string icon() const override { return "palette"; }
	string title() const override { return "Colours & Theme"; }

	void loadSettings() override;
	void applySettings() override;

private:
	vector<colourconfig::Colour> colours_map_preview_;
	vector<colourconfig::Colour> colours_map_image_;

	wxChoice*       choice_map_preview_cfg_    = nullptr;
	wxButton*       btn_map_preview_customize_ = nullptr;
	wxPropertyGrid* pg_map_preview_            = nullptr;

	wxChoice*       choice_map_image_cfg_    = nullptr;
	wxButton*       btn_map_image_customize_ = nullptr;
	wxPropertyGrid* pg_map_image_            = nullptr;

	void refreshPropGrids(bool map_preview = true, bool map_image = true, bool readonly = true) const;

	void onMapPreviewConfigChanged(wxCommandEvent& e);
	void onMapImageConfigChanged(wxCommandEvent& e);
};
} // namespace slade::ui
