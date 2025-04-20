#pragma once

#include "General/UI.h"
#include "Graphics/Palette/Palette.h"

namespace slade::wxutil
{
wxMenuItem* createMenuItem(wxMenu* menu, int id, const string& label, const string& help = "", const string& icon = "");
wxFont      monospaceFont(wxFont base);
wxImageList* createSmallImageList();
int          addImageListIcon(wxImageList* list, int icon_type, string_view icon);
wxPanel*     createPadPanel(wxWindow* parent, wxWindow* control, int pad = -1);
wxSpinCtrl*  createSpinCtrl(wxWindow* parent, int value, int min, int max);

wxSizer* createLabelHBox(wxWindow* parent, const string& label, wxWindow* widget);
wxSizer* createLabelHBox(wxWindow* parent, const string& label, wxSizer* sizer);
wxSizer* createLabelVBox(wxWindow* parent, const string& label, wxWindow* widget);
wxSizer* createLabelVBox(wxWindow* parent, const string& label, wxSizer* sizer);

wxSizer* createDialogButtonBox(wxButton* btn_ok, wxButton* btn_cancel);
wxSizer* createDialogButtonBox(wxWindow* parent, const string& text_ok = "OK", const string& text_cancel = "Cancel");

wxSizer* layoutHorizontally(vector<wxObject*> widgets, int expand_col = -1);
void     layoutHorizontally(wxSizer* sizer, vector<wxObject*> widgets, wxSizerFlags flags = {}, int expand_col = -1);

wxSizer* layoutVertically(vector<wxObject*> widgets, int expand_row = -1);
void     layoutVertically(wxSizer* sizer, vector<wxObject*> widgets, wxSizerFlags flags = {}, int expand_row = -1);

// Strings
wxArrayString arrayString(vector<wxString> vector);
wxArrayString arrayStringStd(const vector<string>& vector);
wxString      strFromView(string_view str);

// Scaling
wxSize  scaledSize(int x, int y);
wxPoint scaledPoint(int x, int y);
wxRect  scaledRect(int x, int y, int width, int height);

// Misc
void      setWindowIcon(wxTopLevelWindow* window, string_view icon);
wxImage   createImageFromSVG(const string& svg_text, int width, int height);
Palette   paletteFromWx(const wxPalette& palette);
wxPalette paletteToWx(const Palette& palette);
} // namespace slade::wxutil
