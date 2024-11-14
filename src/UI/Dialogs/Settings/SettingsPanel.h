#pragma once

namespace slade::ui
{
class SettingsPanel : public wxPanel
{
public:
	SettingsPanel(wxWindow* parent) : wxPanel(parent) { Hide(); }
	~SettingsPanel() override = default;

	virtual string title() const   = 0;
	virtual void   applySettings() = 0;
};
} // namespace slade::ui
