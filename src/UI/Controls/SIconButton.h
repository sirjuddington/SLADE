#pragma once

#include "Graphics/Icons.h"

class SIconButton : public wxBitmapButton
{
public:
	SIconButton(wxWindow* parent, int icon_type, const string& icon, const string& tooltip = "");
	SIconButton(wxWindow* parent, const string& icon, const string& tooltip = "") :
		SIconButton(parent, Icons::GENERAL, icon, tooltip) {}
	~SIconButton() {}
};
