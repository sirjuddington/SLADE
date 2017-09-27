#pragma once

#undef MIN
#undef MAX
#include "common.h"
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#include "General/UI.h"

namespace WxUtils
{
	wxMenuItem*		createMenuItem(
						wxMenu* menu,
						int id,
						const string& label,
						const string& help = wxEmptyString,
						const string& icon = wxEmptyString
					);
	wxFont			getMonospaceFont(wxFont base);

	wxSizer*	createLabelHBox(wxWindow* parent, const string& label, wxWindow* widget);
	wxSizer*	createLabelHBox(wxWindow* parent, const string& label, wxSizer* sizer);
	wxSizer*	createLabelVBox(wxWindow* parent, const string& label, wxWindow* widget);
	wxSizer*	createLabelVBox(wxWindow* parent, const string& label, wxSizer* sizer);

	wxSizer*		layoutHorizontally(vector<wxObject*> widgets, int expand_col = -1);
	void			layoutHorizontally(
						wxSizer* sizer,
						vector<wxObject*> widgets,
						wxSizerFlags flags = {},
						int expand_col = -1
					);

	wxSizer*		layoutVertically(vector<wxObject*> widgets, int expand_row = -1);
	void			layoutVertically(
						wxSizer* sizer,
						vector<wxObject*> widgets,
						wxSizerFlags flags = {},
						int expand_row = -1
					);

	wxArrayString	arrayString(vector<string> vector);

	class VerticalSizer : public wxBoxSizer
	{
	public:
		VerticalSizer() : wxBoxSizer(wxVERTICAL) {}

		void add(wxWindow* widget, wxSizerFlags flags);

	private:
		bool first_ = true;
	};
}
