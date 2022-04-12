#pragma once

namespace slade
{
class DockPanel : public wxPanel
{
public:
	DockPanel(wxWindow* parent);
	~DockPanel() = default;

	virtual void layoutNormal() {}
	virtual void layoutVertical() { layoutNormal(); }
	virtual void layoutHorizontal() { layoutNormal(); }

protected:
	enum class Orient
	{
		Normal,
		Horizontal,
		Vertical,
		Uninitialised
	};
	Orient current_layout_ = Orient::Uninitialised;
};
} // namespace slade
