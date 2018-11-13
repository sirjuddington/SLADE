#pragma once

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

public:
	typedef std::unique_ptr<ArchiveEntry> UPtr;
	typedef std::shared_ptr<ArchiveEntry> SPtr;
	typedef std::weak_ptr<ArchiveEntry>   WPtr;

	// Constructor/Destructor
	ArchiveEntry(string name = "", uint32_t size = 0);
	ArchiveEntry(ArchiveEntry& copy);
	~ArchiveEntry();

	// Accessors
	string   getName(bool cut_ext = false) const;
	string   getUpperName();
	string   getUpperNameNoExt();
	uint32_t getSize()
	{
		if (data_loaded_)
			return data_.getSize();
		else
			return size_;
	}
	MemChunk&        getMCData(bool allow_load = true);
	const uint8_t*   getData(bool allow_load = true);
	ArchiveTreeNode* getParentDir() { return parent_; }
	Archive*         getParent();
	Archive*         getTopParent();
	string           getPath(bool name = false) const;
	EntryType*       getType() { return type_; }
	PropertyList&    exProps() { return ex_props_; }
	Property&        exProp(string key) { return ex_props_[key]; }
	uint8_t          getState() { return state_; }
	bool             isLocked() { return locked_; }
	bool             isLoaded() { return data_loaded_; }
	int              isEncrypted() { return encrypted_; }
	ArchiveEntry*    nextEntry() { return next_; }
	ArchiveEntry*    prevEntry() { return prev_; }
	SPtr             getShared();

	// Modifiers (won't change entry state, except setState of course :P)
	void setName(string name)
	{
		this->name_ = name;
		upper_name_ = name.Upper();
	}
	void setLoaded(bool loaded = true) { data_loaded_ = loaded; }
	void setType(EntryType* type, int r = 0)
	{
		this->type_  = type;
		reliability_ = r;
	}
	void setState(uint8_t state, bool silent = false);
	void setEncryption(int enc) { encrypted_ = enc; }
	void unloadData();
	void lock();
	void unlock();
	void lockState() { state_locked_ = true; }
	void unlockState() { state_locked_ = false; }

	// Entry modification (will change entry state)
	bool rename(string new_name);
	bool resize(uint32_t new_size, bool preserve_data);

	// Data modification
	void clearData();

	// Data import
	bool importMem(const void* data, uint32_t size);
	bool importMemChunk(MemChunk& mc);
	bool importFile(string filename, uint32_t offset = 0, uint32_t size = 0);
	bool importFileStream(wxFile& file, uint32_t len = 0);
	bool importEntry(ArchiveEntry* entry);

	// Data export
	bool exportFile(string filename);

	// Data access
	bool     write(const void* data, uint32_t size);
	bool     read(void* buf, uint32_t size);
	bool     seek(uint32_t offset, uint32_t start) { return data_.seek(offset, start); }
	uint32_t currentPos() { return data_.currentPos(); }

	// Misc
	string getSizeString();
	string getTypeString()
	{
		if (type_)
			return type_->name();
		else
			return "Unknown";
	}
	void          stateChanged();
	void          setExtensionByType();
	int           getTypeReliability() { return (type_ ? (getType()->reliability() * reliability_ / 255) : 0); }
	bool          isInNamespace(string ns);
	ArchiveEntry* relativeEntry(const string& path, bool allow_absolute_path = true) const;

private:
	// Entry Info
	string           name_;
	string           upper_name_;
	uint32_t         size_;
	MemChunk         data_;
	EntryType*       type_;
	ArchiveTreeNode* parent_;
	PropertyList     ex_props_;

	// Entry status
	uint8_t state_;        // 0 = unmodified, 1 = modified, 2 = newly created (not saved to disk)
	bool    state_locked_; // If true the entry state cannot be changed (used for initial loading)
	bool    locked_;       // If true the entry data+info cannot be changed
	bool    data_loaded_;  // True if the entry's data is currently loaded into the data MemChunk
	int     encrypted_;    // Is there some encrypting on the archive?

	// Misc stuff
	int           reliability_; // The reliability of the entry's identification
	ArchiveEntry* next_;
	ArchiveEntry* prev_;

	size_t index_guess_; // for speed
};
