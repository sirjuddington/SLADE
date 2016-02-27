
#ifndef __ENTRYTYPE_H__
#define __ENTRYTYPE_H__

#include "Utility/PropertyList/PropertyList.h"
#include "EntryDataFormat.h"
class ArchiveEntry;

class EntryType
{
private:
	// Type info
	string		id;
	string		name;
	string		extension;		// File extension to use when exporting entries of this type
	string		icon;			// Icon to use in entry list
	string		editor;			// The in-program editor to use (hardcoded ids, see *EntryPanel constructors)
	string		category;		// The type 'category', used for type filtering
	int			index;
	rgba_t		colour;
	bool		detectable;		// False only for special types that should be set not detected
	uint8_t		reliability;	// How 'reliable' this type's detection is. A higher value means it's less
								// likely this type can be detected erroneously. 0-255 (default is 255)
	PropertyList	extra;		// Extra properties for types with certain special behaviours
								// Current recognised properties listed below:
								// bool "image": Can be loaded into a SImage, therefore can be converted to other supported image formats
								// bool "patch": Can be used as a TEXTUREx patch
								// string "image_format": An SIFormat type id 'hint', mostly used for Misc::loadImageFromEntry

	// Type matching criteria
	EntryDataFormat*	format;				// To be of this type, the entry data must match the specified format*
	bool				matchextorname;		// If true, the type fits if it matches the name OR the extension,
											// rather than having to match both. Many definition lumps need this.
	vector<string>	match_extension;
	vector<string>	match_name;
	vector<int>		match_size;
	int				size_limit[2];		// Minimum/maximum size
	vector<int>		size_multiple;		// Entry size must be a multiple of this
	string			section;			// The 'section' of the archive the entry is in, eg "sprites" for entries
										// between SS_START/SS_END in a wad, or the 'sprites' folder in a zip
	vector<string>	match_archive;		// The types of archive the entry can be found in (e.g., wad or zip)

public:
	EntryType(string id = "Unknown");
	~EntryType();

	// Getters
	string			getId()				{ return id; }
	string			getName()			{ return name; }
	string			getExtension()		{ return extension; }
	string			getFormat()			{ return format->getId(); }
	string			getEditor()			{ return editor; }
	string			getCategory()		{ return category; }
	string			getIcon()			{ return icon; }
	int				getIndex()			{ return index; }
	uint8_t			getReliability()	{ return reliability; }
	PropertyList&	extraProps()		{ return extra; }
	rgba_t			getColour()			{ return colour; }

	// Misc
	void	addToList();
	void	dump();
	void	copyToType(EntryType* target);
	string	getFileFilterString();

	// Magic goes here
	int		isThisType(ArchiveEntry* entry);

	// Static functions
	static bool 				readEntryTypeDefinition(MemChunk& mc);
	static bool 				loadEntryTypes();
	static bool 				detectEntryType(ArchiveEntry* entry);
	static EntryType*			getType(string id);
	static EntryType*			unknownType();
	static EntryType*			folderType();
	static EntryType*			mapMarkerType();
	static wxArrayString		getIconList();
	static void					cleanupEntryTypes();
	static vector<EntryType*>	allTypes();
	static vector<string>		allCategories();
};

#endif//__ENTRYTYPE_H__
