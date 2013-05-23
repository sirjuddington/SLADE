#ifndef SWITCHESLIST_H
#define	SWITCHESLIST_H

#include "ArchiveEntry.h"
#include "BinaryControlLump.h"

class SwitchesEntry
{
private:
	uint16_t			type;
	string				off;
	string				on;
	int					status;

public:
	SwitchesEntry(switches_t entry);
	~SwitchesEntry();

	string		getOff()	{ return off; }
	string		getOn()		{ return on; }
	uint8_t		getType()	{ return type; }
	int			getStatus()	{ return status; }
	void		setOff(string o)	{ off = o; }
	void		setOn(string o)		{ on = o; }
	void		setType(uint16_t t)	{ type = t; }
	void		setStatus(int s)	{ status = s; }
};

class SwitchesList
{
private:
	vector<SwitchesEntry*>		entries;

public:
	SwitchesList();
	~SwitchesList();

	uint32_t		nEntries() { return entries.size(); }
	SwitchesEntry*	getEntry(size_t index);
	SwitchesEntry*	getEntry(string name);
	void			clear();
	bool			readSWITCHESData(ArchiveEntry* switches);
	bool			addEntry(SwitchesEntry* entry, size_t pos);
	bool			removeEntry(size_t pos);
	bool			swapEntries(size_t pos1, size_t pos2);

	// Static functions
	static bool convertSwitches(ArchiveEntry* entry, MemChunk* animdata);
};

#endif //SWITCHESLIST_H
