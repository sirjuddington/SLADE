#pragma once

#include "UI/SDialog.h"

namespace slade
{
class SectorSpecialPanel;

class SectorSpecialDialog : public SDialog
{
public:
	SectorSpecialDialog(wxWindow* parent);
	~SectorSpecialDialog() override = default;

	void setup(int special) const;
	int  getSelectedSpecial() const;

private:
	SectorSpecialPanel* panel_special_ = nullptr;
};
} // namespace slade
