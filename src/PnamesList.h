#ifndef PNAMESLIST_H
#define	PNAMESLIST_H

#include "ArchiveEntry.h"

class PnamesEntry
{
private:
	string				name;

public:
	PnamesEntry(char * nam);
	~PnamesEntry();

	string		getName()	{ return name; }
};

class PnamesList {
private:
	vector<PnamesEntry*>		entries;

public:
	PnamesList();
	~PnamesList();

	uint32_t		nEntries() { return entries.size(); }
	PnamesEntry*	getEntry(size_t index);
	PnamesEntry*	getEntry(string name);
	void			clear();
	bool			readPNAMESData(ArchiveEntry* pnames);
	bool			writePNAMESData(ArchiveEntry* pnames);
};

#endif //PNAMESLIST_H
