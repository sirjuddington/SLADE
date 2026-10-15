#pragma once

#include "Graphics/Icons.h"

namespace slade
{
class SIconButton : public wxBitmapButton
{
public:
	SIconButton(
		wxWindow*     parent,
		icons::Type   icon_type,
		const string& icon,
		const string& tooltip     = "",
		int           icon_size   = 16,
		int           button_size = -1);
	SIconButton(
		wxWindow*     parent,
		const string& icon,
		const string& tooltip     = "",
		int           icon_size   = 16,
		int           button_size = -1) :
		SIconButton(parent, icons::General, icon, tooltip, icon_size, button_size)
	{
	}
	~SIconButton() override = default;

protected:
	void DoSetSize(int x, int y, int width, int height, int sizeFlags = wxSIZE_AUTO) override;
};
} // namespace slade
