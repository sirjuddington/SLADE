#pragma once

namespace slade
{
class MapSide;
class NumberTextCtrl;
class SideTexCanvas;
class TextureComboBox;

class SidePropsPanel : public wxPanel
{
public:
	SidePropsPanel(wxWindow* parent);
	~SidePropsPanel() override = default;

	void openSides(const vector<MapSide*>& sides) const;
	void applyTo(const vector<MapSide*>& sides) const;

private:
	SideTexCanvas*   gfx_lower_    = nullptr;
	SideTexCanvas*   gfx_middle_   = nullptr;
	SideTexCanvas*   gfx_upper_    = nullptr;
	TextureComboBox* tcb_lower_    = nullptr;
	TextureComboBox* tcb_middle_   = nullptr;
	TextureComboBox* tcb_upper_    = nullptr;
	NumberTextCtrl*  text_offsetx_ = nullptr;
	NumberTextCtrl*  text_offsety_ = nullptr;

	// Events
	void onTextureChanged(wxCommandEvent& e);
	void onTextureClicked(wxMouseEvent& e);
};
} // namespace slade
