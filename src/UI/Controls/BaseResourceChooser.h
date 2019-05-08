#pragma once

#include "General/ListenerAnnouncer.h"

class BaseResourceChooser : public wxChoice, public Listener
{
public:
	BaseResourceChooser(wxWindow* parent, bool load_change = true);
	~BaseResourceChooser() = default;

	void populateChoices();
	void onAnnouncement(Announcer* announcer, string_view event_name, MemChunk& event_data) override;

private:
	bool load_change_;
};
