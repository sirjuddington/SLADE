#pragma once

#include "EntryType/EntryType.h"
#include "Utility/Property.h"

namespace slade
{
struct ArchiveFormat;
class ArchiveDir;
class Archive;

class ArchiveEntry
{
	friend class ArchiveDir;
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

	// Constructor/Destructor
	ArchiveEntry(string_view name = "", uint32_t size = 0);
	ArchiveEntry(ArchiveEntry& copy);
	~ArchiveEntry() = default;

	// Accessors
	const string&            name() const { return name_; }
	string_view              nameNoExt() const;
	const string&            upperName() const { return upper_name_; }
	string_view              upperNameNoExt() const;
	uint32_t                 size() const { return data_loaded_ ? data_.size() : size_; }
	MemChunk&                data(bool allow_load = true);
	const uint8_t*           rawData(bool allow_load = true);
	ArchiveDir*              parentDir() const { return parent_; }
	Archive*                 parent() const;
	Archive*                 topParent() const;
	string                   path(bool name = false) const;
	EntryType*               type() const { return type_; }
	PropertyList&            exProps() { return ex_props_; }
	const PropertyList&      exProps() const { return ex_props_; }
	Property&                exProp(const string& key) { return ex_props_[key]; }
	template<typename T> T   exProp(const string& key);
	State                    state() const { return state_; }
	bool                     isLocked() const { return locked_; }
	bool                     isLoaded() const { return data_loaded_; }
	Encryption               encryption() const { return encrypted_; }
	ArchiveEntry*            nextEntry();
	ArchiveEntry*            prevEntry();
	shared_ptr<ArchiveEntry> getShared();
	int                      index();

	// Modifiers (won't change entry state, except setState of course :P)
	void setName(string_view name);
	void setLoaded(bool loaded = true) { data_loaded_ = loaded; }
	void setType(EntryType* type, int r = 0)
	{
		type_        = type;
		reliability_ = r;
	}
	void setState(State state, bool silent = false);
	void setEncryption(Encryption enc) { encrypted_ = enc; }
	void unloadData(bool force = false);
	void lock();
	void unlock();
	void lockState() { state_locked_ = true; }
	void unlockState() { state_locked_ = false; }
	void formatName(const ArchiveFormat& format);
	void updateSize() { size_ = data_.size(); }

	// Entry modification (will change entry state)
	bool rename(string_view new_name);
	bool resize(uint32_t new_size, bool preserve_data);

	// Data modification
	void clearData();

	// Data import
	bool importMem(const void* data, uint32_t size);
	bool importMemChunk(MemChunk& mc);
	bool importFile(string_view filename, uint32_t offset = 0, uint32_t size = 0);
	bool importFileStream(wxFile& file, uint32_t len = 0);
	bool importEntry(ArchiveEntry* entry);

	// Data export
	bool exportFile(string_view filename);

	// Data access
	bool     write(const void* data, uint32_t size);
	bool     read(void* buf, uint32_t size);
	bool     seek(uint32_t offset, uint32_t start) { return data_.seek(offset, start); }
	uint32_t currentPos() const { return data_.currentPos(); }

	// Misc
	string        sizeString() const;
	string        typeString() const { return type_ ? type_->name() : "Unknown"; }
	void          stateChanged();
	void          setExtensionByType();
	int           typeReliability() const { return (type_ ? (type()->reliability() * reliability_ / 255) : 0); }
	bool          isInNamespace(string_view ns);
	ArchiveEntry* relativeEntry(string_view path, bool allow_absolute_path = true) const;

private:
	// Entry Info
	string       name_;
	string       upper_name_;
	uint32_t     size_ = 0;
	MemChunk     data_;
	EntryType*   type_   = nullptr;
	ArchiveDir*  parent_ = nullptr;
	PropertyList ex_props_;

	// Entry status
	State      state_        = State::New;
	bool       state_locked_ = false;            // If true the entry state cannot be changed (used for initial loading)
	bool       locked_       = false;            // If true the entry data+info cannot be changed
	bool       data_loaded_  = true;             // True if the entry's data is currently loaded into the data MemChunk
	Encryption encrypted_    = Encryption::None; // Is there some encrypting on the archive?

	// Misc stuff
	int    reliability_ = 0; // The reliability of the entry's identification
	size_t index_guess_ = 0; // for speed
};

template<typename T> T ArchiveEntry::exProp(const string& key)
{
	if (auto val = std::get_if<T>(&ex_props_[key]))
		return *val;

	return T{};
}

} // namespace slade
