#pragma once

class wxPropertyGrid;

namespace slade::ui
{
class ColourSettingsPanel : public wxPanel
{
public:
	ColourSettingsPanel(wxWindow* parent);
	~ColourSettingsPanel() override = default;

	void apply() const;

private:
	wxChoice*       choice_configs_ = nullptr;
	wxPropertyGrid* pg_colours_     = nullptr;

	void init() const;
	void refreshPropGrid() const;
};
} // namespace slade::ui
