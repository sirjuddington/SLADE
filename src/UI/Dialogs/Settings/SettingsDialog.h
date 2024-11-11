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
class SettingsDialog : public SDialog
{
public:
	SettingsDialog(wxWindow* parent);

private:
	SToolBarButton* tbb_general_   = nullptr;
	SToolBarButton* tbb_interface_ = nullptr;
	SToolBarButton* tbb_keybinds_  = nullptr;
	SToolBarButton* tbb_editing_   = nullptr;
	SToolBarButton* tbb_text_      = nullptr;
	SToolBarButton* tbb_gfx_       = nullptr;
	SToolBarButton* tbb_audio_     = nullptr;
	SToolBarButton* tbb_scripting_ = nullptr;
	SToolBarButton* tbb_advanced_  = nullptr;
	wxSizer*        content_sizer_ = nullptr;
	wxStaticText*   title_text_    = nullptr;

	wxWindow*      current_page_   = nullptr;
	wxPanel*       blank_page_     = nullptr;
	SettingsPanel* general_page_   = nullptr;
	SettingsPanel* interface_page_ = nullptr;
	SettingsPanel* graphics_page_  = nullptr;
	SettingsPanel* audio_page_     = nullptr;
	SettingsPanel* text_page_      = nullptr;
	SettingsPanel* scripts_page_   = nullptr;
	SettingsPanel* input_page_     = nullptr;
	SettingsPanel* advanced_page_  = nullptr;

	wxPanel* createSectionsPanel();

	// Events
	void onSectionButtonClicked(wxCommandEvent& e);
};
} // namespace slade::ui
