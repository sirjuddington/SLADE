#pragma once

#include "common.h"

class DockPanel : public wxPanel
{
public:
	DockPanel(wxWindow* parent);
	~DockPanel() {}

	virtual void	layoutNormal() {}
	virtual void	layoutVertical() { layoutNormal(); }
	virtual void	layoutHorizontal() { layoutNormal(); }

protected:
	enum class Orient
	{
		Normal,
		Horizontal,
		Vertical,
		Uninitialised
	};
	Orient	current_layout_;
};
