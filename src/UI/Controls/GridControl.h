#pragma once

namespace slade::ui
{
class GridControl : public wxControl
{
public:
	GridControl(wxWindow* parent, bool allow_decimal = true, double min = 0.0, double max = 65536.0);
	~GridControl() override = default;

	wxComboBox* sizeCombo() const { return combo_grid_size_; }
	wxCheckBox* snapCheckBox() const { return cb_snap_; }

	double gridSize() const;
	bool   snapEnabled() const;

	void setGridSize(double size) const;
	void incrementGridSize() const;
	void decrementGridSize() const;
	void snapToGrid(bool snap) const;

private:
	wxComboBox* combo_grid_size_ = nullptr;
	wxCheckBox* cb_snap_         = nullptr;

	double min_ = 0.0;
	double max_ = 65536.0;
};
} // namespace slade::ui
