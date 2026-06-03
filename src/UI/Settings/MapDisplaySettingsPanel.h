#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class SettingsTable;
struct LayoutHelper;
class RadioButtonPanel;

class MapDisplaySettingsPanel : public SettingsPanel
{
public:
	MapDisplaySettingsPanel(wxWindow* parent);
	~MapDisplaySettingsPanel() override = default;

	string title() const override { return "Map Editor Display Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	SettingsTable*    st_general_       = nullptr;
	SettingsTable*    st_vertices_      = nullptr;
	SettingsTable*    st_lines_         = nullptr;
	SettingsTable*    st_things_        = nullptr;
	SettingsTable*    st_sectors_       = nullptr;
	SettingsTable*    st_3d_            = nullptr;
	RadioButtonPanel* rbp_3d_highlight_ = nullptr;

	wxPanel* createGeneralPanel(wxWindow* parent, const LayoutHelper& lh);
	wxPanel* createVerticesPanel(wxWindow* parent, const LayoutHelper& lh);
	wxPanel* createLinesPanel(wxWindow* parent, const LayoutHelper& lh);
	wxPanel* createThingsPanel(wxWindow* parent, const LayoutHelper& lh);
	wxPanel* createSectorsPanel(wxWindow* parent, const LayoutHelper& lh);
	wxPanel* create3DPanel(wxWindow* parent, const LayoutHelper& lh);
};
} // namespace slade::ui
