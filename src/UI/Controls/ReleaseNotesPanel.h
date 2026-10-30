#pragma once

namespace slade
{
class SIconButton;
}
namespace slade::ui
{
class NoFocusHtmlWindow;

class ReleaseNotesPanel : public wxPanel
{
public:
	ReleaseNotesPanel(wxWindow* parent, string_view version = "");
	~ReleaseNotesPanel() override = default;

private:
	NoFocusHtmlWindow* html_notes_     = nullptr;
	wxChoice*          choice_version_ = nullptr;
	SIconButton*       btn_prev_       = nullptr;
	SIconButton*       btn_next_       = nullptr;

	void loadSelectedVersion() const;
};

void popupWhatsNewDialog(wxWindow* parent, string_view version);
} // namespace slade::ui
