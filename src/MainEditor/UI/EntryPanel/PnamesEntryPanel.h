
#ifndef PNAMESENTRYPANEL_H
#define PNAMESENTRYPANEL_H

#include "EntryPanel.h"
#include "ListView.h"

enum PnameType {
	PNAMES,	// Patch names
	FNAMES,	// Flat names, used by some alpha lumps
	TNAMES,	// Texture names, implicitly used by Doom64
};

class PnamesEntryPanel : public EntryPanel {
private:
	vector<string>	names;

	uint8_t			type;
	ListView*		list_entries;
	wxTextCtrl*		text_name;

public:
	PnamesEntryPanel(wxWindow* parent, uint8_t mode = PNAMES);
	~PnamesEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();

	void	populateEntryList();
};

#endif//PNAMESENTRYPANEL_H
