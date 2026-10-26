#pragma once

namespace slade::colourconfig
{
struct MapEditorTheme;
struct Colour;
enum class Group;
} // namespace slade::colourconfig

namespace slade::ui
{
class ColourConfigPanel : public wxPanel
{
public:
	ColourConfigPanel(wxWindow* parent, colourconfig::Group group, bool allow_customize_default = true, int border = 0);
	~ColourConfigPanel() override = default;

	void loadConfig();
	bool apply();

private:
	colourconfig::Group                      group_;
	vector<colourconfig::Colour>             colours_;
	unique_ptr<colourconfig::MapEditorTheme> map_editor_theme_;

	wxChoice*       choice_config_           = nullptr;
	wxButton*       btn_customize_           = nullptr;
	wxPropertyGrid* pg_colours_              = nullptr;
	bool            allow_customize_default_ = true;

	void refreshPropGrid(bool readonly = true) const;
	void applyCustomColours();

	void onConfigChanged(wxCommandEvent& e);
};
} // namespace slade::ui
