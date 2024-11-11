#pragma once

namespace slade::ui
{
class SettingsPanel : public wxPanel
{
public:
	SettingsPanel(wxWindow* parent) : wxPanel(parent) { Hide(); }
	~SettingsPanel() override = default;

	virtual void applySettings() = 0;
};
} // namespace slade::ui
