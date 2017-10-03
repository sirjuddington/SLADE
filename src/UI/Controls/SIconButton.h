#pragma once

#include "Graphics/Icons.h"

class SIconButton : public wxBitmapButton
{
public:
	SIconButton(wxWindow* parent, const string& icon, int icon_type = Icons::GENERAL, const string& tooltip = "");
	~SIconButton() {}
};
