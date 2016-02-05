
#ifndef __BASE_RESOURCE_CHOOSER_H__
#define __BASE_RESOURCE_CHOOSER_H__

#include "General/ListenerAnnouncer.h"


class BaseResourceChooser : public wxChoice, public Listener
{
private:
	bool	load_change;

public:
	BaseResourceChooser(wxWindow* parent, bool load_change = true);
	~BaseResourceChooser();

	void	populateChoices();
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);

	// Events
	void onChoiceChanged(wxCommandEvent& e);
};

#endif//__BASE_RESOURCE_CHOOSER_H__
