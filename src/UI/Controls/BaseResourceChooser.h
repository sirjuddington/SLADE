#pragma once

#include "General/Sigslot.h"

namespace slade
{
class BaseResourceChooser : public wxChoice
{
public:
	BaseResourceChooser(wxWindow* parent, bool load_change = true);
	~BaseResourceChooser() = default;

	void populateChoices();

private:
	bool                 load_change_;
	ScopedConnectionList signal_connections_;
};
} // namespace slade
