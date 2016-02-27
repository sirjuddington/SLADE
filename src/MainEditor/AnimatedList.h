#ifndef ANIMATEDLIST_H
#define	ANIMATEDLIST_H

#include "Archive/ArchiveEntry.h"
#include "BinaryControlLump.h"

class AnimatedEntry
{
private:
	uint8_t				type;
	string				first;
	string				last;
	uint32_t			speed;
	bool				decals;
	int					status;

public:
	AnimatedEntry(animated_t entry);
	~AnimatedEntry();

	string		getFirst()	{ return first; }
	string		getLast()	{ return last; }
	uint8_t		getType()	{ return type; }
	uint32_t	getSpeed()	{ return speed; }
	bool		getDecals()	{ return decals; }
	int			getStatus()	{ return status; }

	void		setFirst(string f)		{ first = f; }
	void		setLast(string l)		{ last = l; }
	void		setType(uint8_t t)		{ type = t; }
	void		setSpeed(uint32_t s)	{ speed = s; }
	void		setDecals(bool d)		{ decals = d; }
	void		setStatus(int s)		{ status = s; }
};

class AnimatedList
{
private:
	vector<AnimatedEntry*>		entries;

public:
	AnimatedList();
	~AnimatedList();

	uint32_t		nEntries() { return entries.size(); }
	AnimatedEntry*	getEntry(size_t index);
	AnimatedEntry*	getEntry(string name);
	void			clear();
	bool			readANIMATEDData(ArchiveEntry* animated);
	bool			addEntry(AnimatedEntry* entry, size_t pos);
	bool			removeEntry(size_t pos);
	bool			swapEntries(size_t pos1, size_t pos2);

	// Static functions
	static bool convertAnimated(ArchiveEntry* entry, MemChunk* animdata, bool animdefs);
	static bool convertSwanTbls(ArchiveEntry* entry, MemChunk* animdata);
};

#endif //ANIMATEDLIST_H
