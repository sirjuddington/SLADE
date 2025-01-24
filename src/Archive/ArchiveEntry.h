#pragma once

#include "EntryState.h"
#include "Utility/PropertyList.h"

namespace slade
{
class EntryType;
struct ArchiveFormatInfo;

enum class EntryEncryption
{
	None = 0,
	Jaguar,
	Blood,
	SCRLE0,
	TXB,
};

class ArchiveEntry : public std::enable_shared_from_this<ArchiveEntry>
{
	friend class ArchiveDir;
	friend class Archive;
	friend class ArchiveFormatHandler;

public:
	// Constructor/Destructor
	ArchiveEntry(string_view name = "", uint32_t size = 0);
	ArchiveEntry(const ArchiveEntry& copy);
	~ArchiveEntry() = default;

	// Accessors
	const string&            name() const { return name_; }
	string_view              nameNoExt() const;
	const string&            upperName() const { return upper_name_; }
	string_view              upperNameNoExt() const;
	uint32_t                 size() const { return data_.size(); }
	const MemChunk&          data() const { return data_; }
	const uint8_t*           rawData() const { return data_.data(); }
	ArchiveDir*              parentDir() const { return parent_; }
	Archive*                 parent() const;
	Archive*                 topParent() const;
	string                   path(bool name = false) const;
	EntryType*               type() const { return type_; }
	PropertyList&            exProps() { return ex_props_; }
	const PropertyList&      exProps() const { return ex_props_; }
	Property&                exProp(const string& key) { return ex_props_[key]; }
	template<typename T> T   exProp(const string& key);
	EntryState               state() const { return state_; }
	bool                     isLocked() const { return locked_; }
	EntryEncryption          encryption() const { return encrypted_; }
	ArchiveEntry*            nextEntry();
	ArchiveEntry*            prevEntry();
	shared_ptr<ArchiveEntry> getShared() const;
	int                      index();

	// Modifiers (won't change entry state, except setState of course :P)
	void setName(string_view name);
	void setType(EntryType* type, int r = 0)
	{
		type_        = type;
		reliability_ = r;
	}
	void setState(EntryState state, bool silent = false);
	void setEncryption(EntryEncryption enc) { encrypted_ = enc; }
	void lock();
	void unlock();
	void lockState() { state_locked_ = true; }
	void unlockState() { state_locked_ = false; }
	void formatName(const ArchiveFormatInfo& format);

	// Entry modification (will change entry state)
	bool rename(string_view new_name);
	bool resize(uint32_t new_size, bool preserve_data);

	// Data modification
	bool clearData();

	// Data import
	bool importMem(const void* data, uint32_t size);
	bool importMemChunk(const MemChunk& mc, uint32_t offset = 0, uint32_t size = 0);
	bool importFile(string_view filename, uint32_t offset = 0, uint32_t size = 0);
	bool importFileStream(wxFile& file, uint32_t len = 0);
	bool importEntry(const ArchiveEntry* entry);

	// Data export
	bool exportFile(string_view filename) const;

	// Data access
	bool     write(const void* data, uint32_t size);
	bool     read(void* buf, uint32_t size) const;
	bool     seek(uint32_t offset, uint32_t start) const { return data_.seek(offset, start); }
	uint32_t currentPos() const { return data_.currentPos(); }

	// Data on disk
	int  sizeOnDisk() const { return ex_props_.getOr("SizeOnDisk", -1); }
	void setSizeOnDisk(int size) { ex_props_["SizeOnDisk"] = size; }
	void setSizeOnDisk() { ex_props_["SizeOnDisk"] = data_.size(); } // Parameterless version, use data size
	int  offsetOnDisk() const { return ex_props_.getOr("OffsetOnDisk", -1); }
	void setOffsetOnDisk(int offset) { ex_props_["OffsetOnDisk"] = offset; }

	// Misc
	string        sizeString() const;
	string        typeString() const;
	void          stateChanged();
	void          setExtensionByType();
	int           typeReliability() const;
	bool          isInNamespace(string_view ns);
	ArchiveEntry* relativeEntry(string_view path, bool allow_absolute_path = true) const;
	bool          isFolderType() const;
	bool          isArchive() const;

private:
	// Entry Info
	string       name_;
	string       upper_name_;
	MemChunk     data_;
	EntryType*   type_   = nullptr;
	ArchiveDir*  parent_ = nullptr;
	PropertyList ex_props_;

	// Entry status
	EntryState      state_        = EntryState::New;
	bool            state_locked_ = false; // If true the entry state cannot be changed (used for initial loading)
	bool            locked_       = false; // If true the entry data+info cannot be changed
	EntryEncryption encrypted_    = EntryEncryption::None; // Is there some encrypting on the archive?

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
