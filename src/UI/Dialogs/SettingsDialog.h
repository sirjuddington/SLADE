#pragma once

#include "UI/SDialog.h"

namespace slade
{
class SToolButton;
namespace ui
{
	class SettingsPanel;
}
} // namespace slade

namespace slade::ui
{
enum class SettingsPage : u8
{
	General,
	Interface,
	Keybinds,
	Editing,
	Text,
	Graphics,
	Audio,
	Scripting,
	MapGeneral,
	MapDisplay,
	Advanced,

	BaseResource,
	Colour,
	Colorimetry,
	TextStyle,
	Map3d,
	NodeBuilders,
	ExternalEditors,
};
constexpr unsigned SETTINGS_PAGE_COUNT = static_cast<unsigned>(SettingsPage::Advanced) + 1;

class SettingsDialog : public SDialog
{
public:
	SettingsDialog(wxWindow* parent, SettingsPage initial_page = SettingsPage::General);

	void applySettings() const;

	static bool popupSettingsPage(wxWindow* parent, SettingsPage page = SettingsPage::General);

private:
	std::array<SToolButton*, SETTINGS_PAGE_COUNT> section_buttons_;
	std::array<SettingsPanel*, SETTINGS_PAGE_COUNT>  settings_pages_;

	wxSizer*      content_sizer_ = nullptr;
	wxStaticText* title_text_    = nullptr;
	wxWindow*     current_page_  = nullptr;
	wxButton*     btn_apply_     = nullptr;
	wxButton*     btn_cancel_    = nullptr;
	wxButton*     btn_ok_        = nullptr;

	void reloadSettings() const;

	SToolButton* sectionButton(SettingsPage page) const { return section_buttons_[static_cast<size_t>(page)]; }
	SettingsPanel*  settingsPanel(SettingsPage page) const { return settings_pages_[static_cast<size_t>(page)]; }

	void     createSectionButton(wxWindow* parent, SettingsPage page, const string& text, const string& icon);
	wxPanel* createSectionsPanel();

	static string pageId(SettingsPage page);

	// Events
	void onSectionButtonClicked(wxCommandEvent& e);
};
} // namespace slade::ui
