#pragma once

class AboutDialog : public wxDialog
{
public:
	AboutDialog(wxWindow* parent);

private:
	wxBitmap	logo_bitmap_;
};
