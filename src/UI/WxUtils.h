#pragma once

namespace slade::wxutil
{
wxMenuItem* createMenuItem(
	wxMenu*         menu,
	int             id,
	const wxString& label,
	const wxString& help = wxEmptyString,
	const wxString& icon = wxEmptyString);
wxFont       monospaceFont(wxFont base);
wxImageList* createSmallImageList();
int          addImageListIcon(wxImageList* list, int icon_type, string_view icon);
wxPanel*     createPadPanel(wxWindow* parent, wxWindow* control, int pad = -1);
wxSpinCtrl*  createSpinCtrl(wxWindow* parent, int value, int min, int max);

wxSizer* createLabelHBox(wxWindow* parent, const wxString& label, wxWindow* widget);
wxSizer* createLabelHBox(wxWindow* parent, const wxString& label, wxSizer* sizer);
wxSizer* createLabelVBox(wxWindow* parent, const wxString& label, wxWindow* widget);
wxSizer* createLabelVBox(wxWindow* parent, const wxString& label, wxSizer* sizer);

wxSizer* createDialogButtonBox(wxButton* btn_ok, wxButton* btn_cancel);
wxSizer* createDialogButtonBox(
	wxWindow*       parent,
	const wxString& text_ok     = "OK",
	const wxString& text_cancel = "Cancel");

// Strings
inline string_view strToView(const wxString& str)
{
	return { str.data(), str.size() };
}
inline wxString strFromView(string_view view)
{
	return { view.data(), view.size() };
}
wxArrayString arrayString(const vector<wxString>& vector);
wxArrayString arrayStringStd(const vector<string>& vector);

// Misc
void setWindowIcon(wxTopLevelWindow* window, string_view icon);

// From CodeLite
wxColour systemPanelBGColour();
wxColour systemMenuTextColour();
wxColour systemMenuBarBGColour();
wxColour lightColour(const wxColour& colour, float percent);
wxColour darkColour(const wxColour& colour, float percent);
} // namespace slade::wxutil
