#pragma once

#include "EntryDataFormat.h"
#include "Utility/PropertyList/PropertyList.h"
class ArchiveEntry;

class EntryType
{
public:
	EntryType(const wxString& id = "Unknown");
	~EntryType() = default;

	// Getters
	const wxString& id() const { return id_; }
	const wxString& name() const { return name_; }
	const wxString& extension() const { return extension_; }
	const wxString& formatId() const { return format_->id(); }
	const wxString& editor() const { return editor_; }
	const wxString& category() const { return category_; }
	const wxString& icon() const { return icon_; }
	int             index() const { return index_; }
	uint8_t         reliability() const { return reliability_; }
	PropertyList&   extraProps() { return extra_; }
	ColRGBA         colour() const { return colour_; }

	// Misc
	void     addToList();
	void     dump();
	void     copyToType(EntryType* target);
	wxString fileFilterString() const;

	// Magic goes here
	int isThisType(ArchiveEntry* entry);

	// Static functions
	static bool               readEntryTypeDefinition(MemChunk& mc, const wxString& source);
	static bool               loadEntryTypes();
	static bool               detectEntryType(ArchiveEntry* entry);
	static EntryType*         fromId(const wxString& id);
	static EntryType*         unknownType();
	static EntryType*         folderType();
	static EntryType*         mapMarkerType();
	static wxArrayString      iconList();
	static void               cleanupEntryTypes();
	static vector<EntryType*> allTypes();
	static vector<wxString>   allCategories();

private:
	// Type info
	wxString id_;
	wxString name_        = "Unknown";
	wxString extension_   = "dat";     // File extension to use when exporting entries of this type
	wxString icon_        = "default"; // Icon to use in entry list
	wxString editor_      = "default"; // The in-program editor to use (hardcoded ids, see *EntryPanel constructors)
	wxString category_    = "Data";    // The type 'category', used for type filtering
	int      index_       = -1;
	ColRGBA  colour_      = COL_WHITE;
	bool     detectable_  = true; // False only for special types that should be set not detected
	uint8_t  reliability_ = 255;  // How 'reliable' this type's detection is. A higher value means it's less likely this
								  // type can be detected erroneously. 0-255 (default is 255)
	PropertyList extra_;          // Extra properties for types with certain special behaviours
								  // Current recognised properties listed below:
								  // bool "image": Can be loaded into a SImage, therefore can be converted to other
								  // supported image formats
								  // bool "patch": Can be used as a TEXTUREx patch
								  // string "image_format": An SIFormat type id 'hint', mostly used for
								  // Misc::loadImageFromEntry

	// Type matching criteria
	EntryDataFormat* format_;        // To be of this type, the entry data must match the specified format
	bool match_ext_or_name_ = false; // If true, the type fits if it matches the name OR the extension, rather
									 // than having to match both. Many definition lumps need this.
	vector<wxString> match_extension_;
	vector<wxString> match_name_;
	vector<int>      match_size_;
	int              size_limit_[2] = { -1, -1 }; // Minimum/maximum size
	vector<int>      size_multiple_;              // Entry size must be a multiple of this
	vector<wxString> section_;       // The 'section' of the archive the entry must be in, eg "sprites" for entries
									 // between SS_START/SS_END in a wad, or the 'sprites' folder in a zip
	vector<wxString> match_archive_; // The types of archive the entry can be found in (e.g., wad or zip)
};
