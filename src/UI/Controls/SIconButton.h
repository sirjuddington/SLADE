#pragma once

#include "Graphics/Icons.h"

class SIconButton : public wxBitmapButton
{
public:
	SIconButton(wxWindow* parent, Icons::Type icon_type, const wxString& icon, const wxString& tooltip = "");
	SIconButton(wxWindow* parent, const wxString& icon, const wxString& tooltip = "") :
		SIconButton(parent, Icons::General, icon, tooltip)
	{
	}
	~SIconButton() = default;
};
