#pragma once

#include "Graphics/Icons.h"

namespace slade
{
class SIconButton : public wxBitmapButton
{
public:
	SIconButton(
		wxWindow*       parent,
		icons::Type     icon_type,
		const wxString& icon,
		const wxString& tooltip   = "",
		int             icon_size = 16);
	SIconButton(wxWindow* parent, const wxString& icon, const wxString& tooltip = "", int icon_size = 16) :
		SIconButton(parent, icons::General, icon, tooltip, icon_size)
	{
	}
	~SIconButton() override = default;
};
} // namespace slade
