#pragma once

#include "Graphics/Icons.h"

namespace slade
{
class SIconButton : public wxBitmapButton
{
public:
	SIconButton(wxWindow* parent, icons::Type icon_type, const wxString& icon, const wxString& tooltip = "");
	SIconButton(wxWindow* parent, const wxString& icon, const wxString& tooltip = "") :
		SIconButton(parent, icons::General, icon, tooltip)
	{
	}
	~SIconButton() = default;
};
} // namespace slade
