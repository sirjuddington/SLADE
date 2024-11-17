#pragma once

#include "UI/SDialog.h"

namespace slade
{
class SToolBarButton;
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
	Advanced
};
constexpr unsigned SETTINGS_PAGE_COUNT = static_cast<unsigned>(SettingsPage::Advanced) + 1;

class SettingsDialog : public SDialog
{
public:
	SettingsDialog(wxWindow* parent);

	void applySettings() const;

private:
	std::array<SToolBarButton*, SETTINGS_PAGE_COUNT> section_buttons_;
	std::array<SettingsPanel*, SETTINGS_PAGE_COUNT>  settings_pages_;

	wxSizer*      content_sizer_ = nullptr;
	wxStaticText* title_text_    = nullptr;
	wxWindow*     current_page_  = nullptr;
	wxButton*     btn_apply_     = nullptr;
	wxButton*     btn_cancel_    = nullptr;
	wxButton*     btn_ok_        = nullptr;

	void reloadSettings() const;

	SToolBarButton* sectionButton(SettingsPage page) const { return section_buttons_[static_cast<size_t>(page)]; }
	SettingsPanel*  settingsPanel(SettingsPage page) const { return settings_pages_[static_cast<size_t>(page)]; }

	void createSectionButton(
		wxWindow*     parent,
		SettingsPage  page,
		const string& action,
		const string& text,
		const string& icon);
	wxPanel* createSectionsPanel();

	// Events
	void onSectionButtonClicked(wxCommandEvent& e);
};
} // namespace slade::ui
