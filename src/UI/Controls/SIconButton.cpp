
#include "Main.h"
#include "SIconButton.h"
#include "Graphics/Icons.h"
#include "MapEditor/UI/MapCanvas.h"

SIconButton::SIconButton(wxWindow* parent, const string& icon, int icon_type, const string& tooltip) :
	wxBitmapButton{ parent, -1, wxNullBitmap }
{
	// Create icon
	auto bmp = Icons::getIcon(icon_type, icon, UI::scaleFactor() > 1.);
	
	// Scale icon if required
	auto size = UI::scalePx(16);
	if (bmp.GetWidth() != size)
	{
		auto img = bmp.ConvertToImage();
		img.Rescale(size, size, wxIMAGE_QUALITY_BICUBIC);
		bmp = wxBitmap(img);
	}

	// Set button image and tooltip
	SetBitmap(bmp);
	if (!tooltip.empty())
		SetToolTip(tooltip);
}
