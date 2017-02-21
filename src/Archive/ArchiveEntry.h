
#ifndef __ARCHIVEENTRY_H__
#define __ARCHIVEENTRY_H__

#include "EntryType/EntryType.h"
#include "Utility/PropertyList/PropertyList.h"

class ArchiveTreeNode;
class Archive;

enum EncryptionModes
{
	ENC_NONE = 0,
	ENC_JAGUAR,
	ENC_BLOOD,
	ENC_SCRLE0,
	ENC_TXB,
};

class ArchiveEntry
{
	friend class ArchiveTreeNode;
	friend class Archive;
private:
	// Entry Info
	string				name;
	string				upper_name;
	uint32_t			size;
	MemChunk			data;
	EntryType*			type;
	ArchiveTreeNode*	parent;
	PropertyList		ex_props;

	// Entry status
	uint8_t			state;			// 0 = unmodified, 1 = modified, 2 = newly created (not saved to disk)
	bool			state_locked;	// If true the entry state cannot be changed (used for initial loading)
	bool			locked;			// If true the entry data+info cannot be changed
	bool			data_loaded;	// True if the entry's data is currently loaded into the data MemChunk
	int				encrypted;		// Is there some encrypting on the archive?

	// Misc stuff
	int				reliability;	// The reliability of the entry's identification
	ArchiveEntry*	next;
	ArchiveEntry*	prev;

public:
	typedef	std::unique_ptr<ArchiveEntry>	UPtr;
	typedef	std::shared_ptr<ArchiveEntry>	SPtr;

	// Constructor/Destructor
	ArchiveEntry(string name = "", uint32_t size = 0);
	ArchiveEntry(ArchiveEntry& copy);
	~ArchiveEntry();

	// Accessors
	string				getName(bool cut_ext = false);
	string				getUpperName();
	string				getUpperNameNoExt();
	uint32_t			getSize()			{ if (data_loaded) return data.getSize(); else return size; }
	MemChunk&			getMCData(bool allow_load = true);
	const uint8_t*		getData(bool allow_load = true);
	ArchiveTreeNode*	getParentDir()		{ return parent; }
	Archive*			getParent();
	Archive*			getTopParent();
	string				getPath(bool name = false);
	EntryType*			getType()			{ return type; }
	PropertyList&		exProps()			{ return ex_props; }
	Property&			exProp(string key)	{ return ex_props[key]; }
	uint8_t				getState()			{ return state; }
	bool				isLocked()			{ return locked; }
	bool				isLoaded()			{ return data_loaded; }
	int					isEncrypted()		{ return encrypted; }
	ArchiveEntry*		nextEntry()			{ return next; }
	ArchiveEntry*		prevEntry()			{ return prev; }
	SPtr				getShared();

	// Modifiers (won't change entry state, except setState of course :P)
	void		setName(string name) { this->name = name; upper_name = name.Upper(); }
	void		setLoaded(bool loaded = true) { data_loaded = loaded; }
	void		setType(EntryType* type, int r = 0) { this->type = type; reliability = r; }
	void		setState(uint8_t state);
	void		setEncryption(int enc) { encrypted = enc; }
	void		unloadData();
	void		lock();
	void		unlock();
	void		lockState() { state_locked = true; }
	void		unlockState() { state_locked = false; }

	// Entry modification (will change entry state)
	bool	rename(string new_name);
	bool	resize(uint32_t new_size, bool preserve_data);

	// Data modification
	void	clearData();

	// Data import
	bool	importMem(const void* data, uint32_t size);
	bool	importMemChunk(MemChunk& mc);
	bool	importFile(string filename, uint32_t offset = 0, uint32_t size = 0);
	bool	importFileStream(wxFile& file, uint32_t len = 0);
	bool	importEntry(ArchiveEntry* entry);

	// Data export
	bool	exportFile(string filename);

	// Data access
	bool		write(const void* data, uint32_t size);
	bool		read(void* buf, uint32_t size);
	bool		seek(uint32_t offset, uint32_t start) { return data.seek(offset, start); }
	uint32_t	currentPos() { return data.currentPos(); }

	// Misc
	string	getSizeString();
	string	getTypeString() { if (type) return type->getName(); else return "Unknown"; }
	void	stateChanged();
	void	setExtensionByType();
	int		getTypeReliability() { return (type ? (getType()->getReliability() * reliability / 255) : 0); }
	bool	isInNamespace(string ns);

	size_t	index_guess; // for speed
};

#endif//__ARCHIVEENTRY_H__
