#pragma once

#include "EntryType/EntryType.h"
#include "Utility/PropertyList/PropertyList.h"

struct ArchiveFormat;
class ArchiveTreeNode;
class Archive;

class ArchiveEntry
{
	friend class ArchiveTreeNode;
	friend class Archive;

public:
	enum class Encryption
	{
		None = 0,
		Jaguar,
		Blood,
		SCRLE0,
		TXB,
	};
	enum class State
	{
		Unmodified,
		Modified,
		New // Newly created (not saved on disk yet)
	};

	typedef std::unique_ptr<ArchiveEntry> UPtr;
	typedef std::shared_ptr<ArchiveEntry> SPtr;
	typedef std::weak_ptr<ArchiveEntry>   WPtr;

	// Constructor/Destructor
	ArchiveEntry(const wxString& name = "", uint32_t size = 0);
	ArchiveEntry(ArchiveEntry& copy);
	~ArchiveEntry() = default;

	// Accessors
	wxString            name(bool cut_ext = false) const;
	wxString            upperName() const { return upper_name_; }
	wxString            upperNameNoExt() const;
	uint32_t            size() const { return data_loaded_ ? data_.size() : size_; }
	MemChunk&           data(bool allow_load = true);
	const uint8_t*      rawData(bool allow_load = true);
	ArchiveTreeNode*    parentDir() const { return parent_; }
	Archive*            parent() const;
	Archive*            topParent() const;
	wxString            path(bool name = false) const;
	EntryType*          type() const { return type_; }
	PropertyList&       exProps() { return ex_props_; }
	const PropertyList& exProps() const { return ex_props_; }
	Property&           exProp(const wxString& key) { return ex_props_[key]; }
	State               state() const { return state_; }
	bool                isLocked() const { return locked_; }
	bool                isLoaded() const { return data_loaded_; }
	Encryption          encryption() const { return encrypted_; }
	ArchiveEntry*       nextEntry() const { return next_; }
	ArchiveEntry*       prevEntry() const { return prev_; }
	SPtr                getShared();

	// Modifiers (won't change entry state, except setState of course :P)
	void setName(const wxString& name)
	{
		name_       = name;
		upper_name_ = name.Upper();
	}
	void setLoaded(bool loaded = true) { data_loaded_ = loaded; }
	void setType(EntryType* type, int r = 0)
	{
		type_        = type;
		reliability_ = r;
	}
	void setState(State state, bool silent = false);
	void setEncryption(Encryption enc) { encrypted_ = enc; }
	void unloadData();
	void lock();
	void unlock();
	void lockState() { state_locked_ = true; }
	void unlockState() { state_locked_ = false; }
	void formatName(const ArchiveFormat& format);

	// Entry modification (will change entry state)
	bool rename(const wxString& new_name);
	bool resize(uint32_t new_size, bool preserve_data);

	// Data modification
	void clearData();

	// Data import
	bool importMem(const void* data, uint32_t size);
	bool importMemChunk(MemChunk& mc);
	bool importFile(const wxString& filename, uint32_t offset = 0, uint32_t size = 0);
	bool importFileStream(wxFile& file, uint32_t len = 0);
	bool importEntry(ArchiveEntry* entry);

	// Data export
	bool exportFile(const wxString& filename);

	// Data access
	bool     write(const void* data, uint32_t size);
	bool     read(void* buf, uint32_t size);
	bool     seek(uint32_t offset, uint32_t start) { return data_.seek(offset, start); }
	uint32_t currentPos() { return data_.currentPos(); }

	// Misc
	wxString      sizeString() const;
	wxString      typeString() const { return type_ ? type_->name() : "Unknown"; }
	void          stateChanged();
	void          setExtensionByType();
	int           typeReliability() const { return (type_ ? (type()->reliability() * reliability_ / 255) : 0); }
	bool          isInNamespace(wxString ns);
	ArchiveEntry* relativeEntry(const wxString& path, bool allow_absolute_path = true) const;

private:
	// Entry Info
	wxString         name_;
	wxString         upper_name_;
	uint32_t         size_ = 0;
	MemChunk         data_;
	EntryType*       type_   = nullptr;
	ArchiveTreeNode* parent_ = nullptr;
	PropertyList     ex_props_;

	// Entry status
	State      state_        = State::New;
	bool       state_locked_ = false;            // If true the entry state cannot be changed (used for initial loading)
	bool       locked_       = false;            // If true the entry data+info cannot be changed
	bool       data_loaded_  = true;             // True if the entry's data is currently loaded into the data MemChunk
	Encryption encrypted_    = Encryption::None; // Is there some encrypting on the archive?

	// Misc stuff
	int           reliability_ = 0; // The reliability of the entry's identification
	ArchiveEntry* next_        = nullptr;
	ArchiveEntry* prev_        = nullptr;

	size_t index_guess_ = 0; // for speed
};
