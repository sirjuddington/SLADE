
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveManager.cpp
// Description: ArchiveManager class. Manages all open archives and the
//              interactions between them
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ArchiveManager.h"
#include "App.h"
#include "Archive.h"
#include "ArchiveDir.h"
#include "ArchiveEntry.h"
#include "ArchiveFormatHandler.h"
#include "Database/Tables/ArchiveFile.h"
#include "EntryType/EntryType.h"
#include "General/Console.h"
#include "General/ResourceManager.h"
#include "MainEditor/MainEditor.h"
#include "UI/UI.h"
#include "Utility/DateTime.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, base_resource, -1, CVar::Flag::Save)
CVAR(Bool, auto_open_wads_root, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Updates/Adds [archive] in/to the database and updates last opened time if
// requested.
// -----------------------------------------------------------------------------
i64 updateArchiveInDatabase(Archive& archive, bool update_last_opened)
{
	auto db_id = database::archiveFileId(archive);
	if (db_id < 0)
		db_id = database::writeArchiveFile(archive);

	if (update_last_opened)
		database::setArchiveFileLastOpened(db_id, datetime::now());

	return db_id;
}
} // namespace


// -----------------------------------------------------------------------------
//
// ArchiveManager Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks that the given directory is actually a suitable resource directory
// for SLADE 3, and not just a directory named 'res' that happens to be there
// (possibly because the user installed SLADE in the same folder as an
// installation of SLumpEd).
// -----------------------------------------------------------------------------
bool ArchiveManager::validResDir(string_view dir) const
{
	// Assortment of resources that the program expects to find.
	// If at least one is missing, then probably more are missing
	// too, so the res folder cannot be used.
	static string paths[] = {
		"animated.lmp",
		"config/executables.cfg",
		"config/nodebuilders.cfg",
		"fonts/dejavu_sans.ttf",
		"html/box-title-back.png",
		"html/startpage.htm",
		"icons/entry_list/archive.svg",
		"icons/general/wiki.svg",
		"images/arrow.png",
		"logo.png",
		"palettes/Doom .pal",
		"s3dummy.lmp",
		"slade.ico",
		"switches.lmp",
		"tips.txt",
		"vga-rom-font.16",
	};

	for (const auto& path : paths)
	{
		if (!fileutil::fileExists(fmt::format("{}/{}", dir, path)))
		{
			log::warning(
				"Resource {} was not found in dir {}!\n"
				"This resource folder cannot be used. "
				"(Did you install SLADE 3 in a SLumpEd folder?)",
				path,
				dir);
			return false;
		}
	}
	return true;
}

// -----------------------------------------------------------------------------
// Initialised the ArchiveManager. Finds and opens the program resource archive
// -----------------------------------------------------------------------------
bool ArchiveManager::init()
{
#ifdef __WXOSX__
	auto resdir = app::path("../Resources", app::Dir::Executable); // Use Resources dir within bundle on mac
#else
	auto resdir = app::path("res", app::Dir::Executable);
#endif

	if (fileutil::dirExists(resdir) && validResDir(resdir))
	{
		program_resource_archive_ = std::make_shared<Archive>(ArchiveFormat::Dir);
		program_resource_archive_->setFilename(app::path("slade.pk3", app::Dir::Executable));
		program_resource_archive_->importDir(resdir);
		res_archive_open_ = (program_resource_archive_->numEntries() > 0);

		if (!initArchiveFormats())
			log::error("An error occurred reading archive formats configuration");

		program_resource_archive_->setReadOnly();
		return res_archive_open_;
	}

	program_resource_archive_ = std::make_shared<Archive>(ArchiveFormat::Zip);
	program_resource_archive_->setFilename(app::path("slade.pk3", app::Dir::Executable));

	// Find slade3.pk3 directory
	auto dir_slade_pk3 = app::path("slade.pk3", app::Dir::Resources);
	if (!fileutil::fileExists(dir_slade_pk3))
		dir_slade_pk3 = app::path("slade.pk3", app::Dir::Data);
	if (!fileutil::fileExists(dir_slade_pk3))
		dir_slade_pk3 = app::path("slade.pk3", app::Dir::Executable);
	if (!fileutil::fileExists(dir_slade_pk3))
		dir_slade_pk3 = app::path("slade.pk3", app::Dir::User);
	if (!fileutil::fileExists(dir_slade_pk3))
		dir_slade_pk3 = "slade.pk3";

	// Open slade.pk3
	if (!program_resource_archive_->open(dir_slade_pk3))
	{
		log::error("Unable to find slade.pk3!");
		res_archive_open_ = false;
	}
	else
		res_archive_open_ = true;

	if (!initArchiveFormats())
		log::error("An error occurred reading archive formats configuration");

	program_resource_archive_->setReadOnly();
	return res_archive_open_;
}

// -----------------------------------------------------------------------------
// Loads archive formats configuration from the program resource
// -----------------------------------------------------------------------------
bool ArchiveManager::initArchiveFormats() const
{
	return archive::loadFormatInfo(program_resource_archive_->entryAtPath("config/archive_formats.cfg")->data());
}

// -----------------------------------------------------------------------------
// Initialises the base resource archive
// -----------------------------------------------------------------------------
bool ArchiveManager::initBaseResource()
{
	return openBaseResource((int)base_resource);
}

// -----------------------------------------------------------------------------
// Adds an archive to the archive list
// -----------------------------------------------------------------------------
bool ArchiveManager::addArchive(shared_ptr<Archive> archive)
{
	// Only add if archive is a valid pointer
	if (archive)
	{
		// Add to the list
		OpenArchive n_archive;
		n_archive.archive  = archive;
		n_archive.resource = true;
		n_archive.db_id    = -1;
		open_archives_.push_back(n_archive);

		// Emit archive changed/saved signal when received from the archive
		archive->signals().modified.connect([this](Archive& archive, bool modified)
											{ signals_.archive_modified(archiveIndex(&archive), modified); });
		archive->signals().saved.connect(
			[this](Archive& archive)
			{
				// Update in database
				if (archive.isOnDisk())
					setArchiveDbId(archive, database::writeArchiveFile(archive));

				signals_.archive_saved(archiveIndex(&archive));
			});

		// Announce the addition
		signals_.archive_added(open_archives_.size() - 1);

		// Add to resource manager
		app::resources().addArchive(archive.get());

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns the archive at the index specified (nullptr if it doesn't exist)
// -----------------------------------------------------------------------------
shared_ptr<Archive> ArchiveManager::getArchive(int index)
{
	// Check that index is valid
	if (index < 0 || index >= static_cast<int>(open_archives_.size()))
		return nullptr;
	else
		return open_archives_[index].archive;
}

// -----------------------------------------------------------------------------
// Returns the archive with the specified filename
// (nullptr if it doesn't exist)
// -----------------------------------------------------------------------------
shared_ptr<Archive> ArchiveManager::getArchive(string_view filename)
{
	// Go through all open archives
	for (auto& open_archive : open_archives_)
	{
		// If the filename matches, return it
		if (strutil::Path::filePathsMatch(open_archive.archive->filename(), filename))
			return open_archive.archive;
	}

	// If no archive is found with a matching filename, return nullptr
	return nullptr;
}

// ----------------------------------------------------------------------------
// Returns the archive opened from the given parent entry
// (nullptr if it doesn't exist)
// ----------------------------------------------------------------------------
shared_ptr<Archive> ArchiveManager::getArchive(const ArchiveEntry* parent)
{
	for (unsigned a = 0; a < open_archives_.size(); ++a)
	{
		if (open_archives_[a].archive->parentEntry() == parent)
			return open_archives_[a].archive;
	}

	return nullptr;
}

// ----------------------------------------------------------------------------
// Opens and adds a archive to the list, returns a pointer to the newly opened
// and added archive, or nullptr if an error occurred
// -----------------------------------------------------------------------------
shared_ptr<Archive> ArchiveManager::openArchive(string_view filename, bool manage, bool silent)
{
	// Check for directory
	if (fileutil::dirExists(filename))
		return openDirArchive(filename, manage, silent);

	auto new_archive = getArchive(filename);

	log::info("Opening archive {}", filename);

	// If the archive is already open, just return it
	if (new_archive)
	{
		// Announce open
		if (!silent)
			signals_.archive_opened(archiveIndex(new_archive.get()));

		return new_archive;
	}

	// Check the file exists
	if (!fileutil::fileExists(filename))
	{
		global::error = fmt::format("File \"{}\" does not exist", filename);
		return nullptr;
	}

	// Determine file format
	string std_fn{ filename };
	auto   format_id = archive::detectArchiveFormat(std_fn);
	if (format_id == ArchiveFormat::Unknown)
	{
		// Unsupported/unknown format
		global::error = "Unsupported or invalid Archive format";
		return nullptr;
	}

	// Create archive
	new_archive = std::make_shared<Archive>(format_id);

	// If it opened successfully, add it to the list if needed & return it,
	// Otherwise, delete it and return nullptr
	if (new_archive->open(filename))
	{
		if (manage)
		{
			// Add the archive
			auto index = open_archives_.size();
			addArchive(new_archive);

			// Update in database
			auto db_id = updateArchiveInDatabase(*new_archive, true);
			setArchiveDbId(*new_archive, db_id);

			// ZDoom also loads any WADs found in the root of a PK3
			if (auto_open_wads_root && new_archive->formatId() == "zip")
				openWadsInRoot(*new_archive);

			// Announce open
			if (!silent)
				signals_.archive_opened(index);
		}

		// Return the opened archive
		return new_archive;
	}
	else
	{
		log::error(global::error);
		return nullptr;
	}
}

// -----------------------------------------------------------------------------
// Same as the above function, except it opens from an ArchiveEntry
// -----------------------------------------------------------------------------
shared_ptr<Archive> ArchiveManager::openArchive(ArchiveEntry* entry, bool manage, bool silent)
{
	// Check entry was given
	if (!entry)
		return nullptr;

	// Check if the entry is already opened
	for (auto& open_archive : open_archives_)
	{
		if (open_archive.archive->parentEntry() == entry)
		{
			// Announce open
			if (!silent)
				signals_.archive_opened(archiveIndex(open_archive.archive.get()));

			return open_archive.archive;
		}
	}

	// Check entry type
	auto format_id = archive::detectArchiveFormat(entry->data());
	if (format_id == ArchiveFormat::Unknown)
	{
		// Unsupported/unknown format
		global::error = "Unsupported or invalid Archive format";
		return nullptr;
	}

	// Create archive
	auto new_archive = std::make_shared<Archive>(format_id);

	// If it opened successfully, add it to the list & return it,
	// Otherwise, delete it and return nullptr
	if (new_archive->open(entry))
	{
		if (manage)
		{
			// Add to parent's child list if parent is open in the manager (it should be)
			int index_parent = -1;
			if (entry->parent())
				index_parent = archiveIndex(entry->parent());
			if (index_parent >= 0)
				open_archives_[index_parent].open_children.emplace_back(new_archive);

			// Add the new archive
			auto index = open_archives_.size();
			addArchive(new_archive);

			// Update in database
			auto db_id = updateArchiveInDatabase(*new_archive, true);
			setArchiveDbId(*new_archive, db_id);

			// Announce open
			if (!silent)
				signals_.archive_opened(index);

			entry->lock();
		}

		return new_archive;
	}
	else
	{
		log::error(global::error);
		return nullptr;
	}
}

// -----------------------------------------------------------------------------
// Opens [dir] as a DirArchive and adds it to the list.
// Returns a pointer to the archive or nullptr if an error occurred.
// -----------------------------------------------------------------------------
shared_ptr<Archive> ArchiveManager::openDirArchive(string_view dir, bool manage, bool silent)
{
	auto new_archive = getArchive(dir);

	log::info("Opening directory {} as archive", dir);

	// If the archive is already open, just return it
	if (new_archive)
	{
		// Announce open
		if (!silent)
			signals_.archive_opened(archiveIndex(new_archive.get()));

		return new_archive;
	}

	new_archive = std::make_shared<Archive>(ArchiveFormat::Dir);

	// If it opened successfully, add it to the list if needed & return it,
	// Otherwise, delete it and return nullptr
	if (new_archive->open(dir))
	{
		if (manage)
		{
			// Add the archive
			auto index = open_archives_.size();
			addArchive(new_archive);

			// Update in database
			auto db_id = updateArchiveInDatabase(*new_archive, true);
			setArchiveDbId(*new_archive, db_id);

			// ZDoom also loads any WADs found in the root of a directory
			if (auto_open_wads_root)
				openWadsInRoot(*new_archive);

			// Announce open
			if (!silent)
				signals_.archive_opened(index);
		}

		// Return the opened archive
		return new_archive;
	}
	else
	{
		log::error(global::error);
		return nullptr;
	}
}

// -----------------------------------------------------------------------------
// Creates a new archive of the specified format and adds it to the list of open
// archives. Returns the created archive, or nullptr if an invalid archive type
// was given
// -----------------------------------------------------------------------------
shared_ptr<Archive> ArchiveManager::newArchive(ArchiveFormat format)
{
	// Check if the format specified allows archive creation
	auto format_info = archive::formatInfo(format);
	if (!format_info.create)
	{
		global::error = fmt::format("Can not create archive of format: {}", format_info.name);
		log::error(global::error);
		return nullptr;
	}

	// Create a new archive depending on the format specified
	auto new_archive = std::make_shared<Archive>(format);

	// Set its filename and add it to the list
	new_archive->setFilename(fmt::format("UNSAVED ({})", new_archive->formatInfo().name));
	addArchive(new_archive);

	// Return the created archive, if any
	return new_archive;
}
shared_ptr<Archive> ArchiveManager::newArchive(string_view format)
{
	return newArchive(archive::formatFromId(format));
}

// -----------------------------------------------------------------------------
// Closes the archive at index, and removes it from the list if the index is
// valid. Returns false on invalid index, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveManager::closeArchive(int index)
{
	// Check for invalid index
	if (index < 0 || index >= static_cast<int>(open_archives_.size()))
		return false;

	// Announce archive closing
	signals_.archive_closing(index);

	// Delete any bookmarked entries contained in the archive
	deleteBookmarksInArchive(open_archives_[index].archive.get());

	// Remove from resource manager
	app::resources().removeArchive(open_archives_[index].archive.get());

	// Close any open child archives
	// Clear out the open_children vector first, lest the children try to remove themselves from it
	auto open_children = open_archives_[index].open_children;
	open_archives_[index].open_children.clear();
	for (auto& archive : open_children)
	{
		int ci = archiveIndex(archive.lock().get());
		if (ci >= 0)
			closeArchive(ci);
	}

	// Remove ourselves from our parent's open-child list
	if (auto parent = open_archives_[index].archive->parentEntry())
	{
		if (auto gp = parent->parent())
		{
			int pi = archiveIndex(gp);
			if (pi >= 0)
			{
				auto& children = open_archives_[pi].open_children;
				for (auto it = children.begin(); it < children.end(); ++it)
				{
					if (it->lock() == open_archives_[index].archive)
					{
						children.erase(it, it + 1);
						break;
					}
				}
			}
		}

		parent->unlock();
	}

	// Close the archive
	open_archives_[index].archive->close();

	// Remove the archive at index from the list
	open_archives_.erase(open_archives_.begin() + index);

	// Announce closed
	signals_.archive_closed(index);

	return true;
}

// -----------------------------------------------------------------------------
// Finds the archive with a matching filename, deletes it and removes it from
// the list.
// Returns false if it doesn't exist or can't be removed, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveManager::closeArchive(string_view filename)
{
	// Go through all open archives
	for (int a = 0; a < static_cast<int>(open_archives_.size()); a++)
	{
		// If the filename matches, remove it
		if (open_archives_[a].archive->filename() == filename)
			return closeArchive(a);
	}

	// If no archive is found with a matching filename, return false
	return false;
}

// -----------------------------------------------------------------------------
// Closes the specified archive and removes it from the list, if it exists in
// the list. Returns false if it doesn't exist, else true
// -----------------------------------------------------------------------------
bool ArchiveManager::closeArchive(const Archive* archive)
{
	// Go through all open archives
	for (int a = 0; a < static_cast<int>(open_archives_.size()); a++)
	{
		// If the archive exists in the list, remove it
		if (open_archives_[a].archive.get() == archive)
			return closeArchive(a);
	}

	// If the archive isn't in the list, return false
	return false;
}

// -----------------------------------------------------------------------------
// Closes all opened archives
// -----------------------------------------------------------------------------
void ArchiveManager::closeAll()
{
	// Close the first archive in the list until no archives are open
	while (!open_archives_.empty())
		closeArchive(0);
}

// -----------------------------------------------------------------------------
// Returns the index in the list of the given archive, or -1 if the archive
// doesn't exist in the list
// -----------------------------------------------------------------------------
int ArchiveManager::archiveIndex(const Archive* archive) const
{
	// Go through all open archives
	for (size_t a = 0; a < open_archives_.size(); a++)
	{
		// If the archive we're looking for is this one, return the index
		if (open_archives_[a].archive.get() == archive)
			return static_cast<int>(a);
	}

	// If we get to here the archive wasn't found, so return -1
	return -1;
}

// -----------------------------------------------------------------------------
// Returns all open archives that live inside this one, recursively.
// -----------------------------------------------------------------------------
// This is the recursive bit, separate from the public entry point
void ArchiveManager::getDependentArchivesInternal(const Archive* archive, vector<shared_ptr<Archive>>& vec)
{
	int ai = archiveIndex(archive);

	for (auto& child : open_archives_[ai].open_children)
	{
		if (auto child_sptr = child.lock())
		{
			vec.push_back(child_sptr);
			getDependentArchivesInternal(child_sptr.get(), vec);
		}
	}
}
vector<shared_ptr<Archive>> ArchiveManager::getDependentArchives(const Archive* archive)
{
	vector<shared_ptr<Archive>> vec;
	getDependentArchivesInternal(archive, vec);
	return vec;
}

// -----------------------------------------------------------------------------
// Sets [archive]'s database ID to [db_id]
// (in the ArchiveManager, not the database itself)
// -----------------------------------------------------------------------------
void ArchiveManager::setArchiveDbId(const Archive& archive, i64 db_id)
{
	for (auto& oa : open_archives_)
		if (oa.archive.get() == &archive)
			oa.db_id = db_id;
}

// -----------------------------------------------------------------------------
// Opens all wad archives in the root directory of [archive]
// -----------------------------------------------------------------------------
void ArchiveManager::openWadsInRoot(const Archive& archive)
{
	for (const auto& entry : archive.rootDir()->entries())
	{
		if (entry->type() == EntryType::unknownType())
			EntryType::detectEntryType(*entry);

		if (entry->type()->id() == "wad")
			// First true: yes, manage this
			// Second true: open silently, don't open a tab for it
			openArchive(entry.get(), true, true);
	}
}

// -----------------------------------------------------------------------------
// Returns a string containing the extensions of all supported archive formats,
// that can be used for wxWidgets file dialogs
// -----------------------------------------------------------------------------
string ArchiveManager::getArchiveExtensionsString() const
{
	auto           formats = archive::allFormatsInfo();
	vector<string> ext_strings;
	string         ext_all = "Any supported file|";
	for (const auto& fmt : formats)
	{
		for (const auto& ext : fmt.extensions)
		{
			auto ext_case = fmt::format("*.{};", strutil::lower(ext.first));
			ext_case += fmt::format("*.{};", strutil::upper(ext.first));
			ext_case += fmt::format("*.{}", strutil::capitalize(ext.first));

			ext_all += fmt::format("{};", ext_case);
			ext_strings.push_back(fmt::format("{} files (*.{})|{}", ext.second, ext.first, ext_case));
		}
	}

	ext_all.pop_back();
	for (const auto& ext : ext_strings)
	{
		ext_all += '|';
		ext_all += ext;
	}

	return ext_all;
}

// -----------------------------------------------------------------------------
// Returns true if [archive] is set to be used as a resource, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveManager::archiveIsResource(const Archive* archive) const
{
	int index = archiveIndex(archive);
	if (index < 0)
		return false;
	else
		return open_archives_[index].resource;
}

// -----------------------------------------------------------------------------
// Sets/unsets [archive] to be used as a resource
// -----------------------------------------------------------------------------
void ArchiveManager::setArchiveResource(Archive* archive, bool resource)
{
	int index = archiveIndex(archive);
	if (index >= 0)
	{
		bool was_resource              = open_archives_[index].resource;
		open_archives_[index].resource = resource;

		// Update resource manager
		if (resource && !was_resource)
			app::resources().addArchive(archive);
		else if (!resource && was_resource)
			app::resources().removeArchive(archive);
	}
}

// -----------------------------------------------------------------------------
// Returns a vector of all open archives.
// If [resources_only] is true, only includes archives marked as resources
// -----------------------------------------------------------------------------
vector<shared_ptr<Archive>> ArchiveManager::allArchives(bool resource_only) const
{
	vector<shared_ptr<Archive>> ret;

	for (const auto& archive : open_archives_)
		if (!resource_only || archive.resource)
			ret.push_back(archive.archive);

	return ret;
}

// -----------------------------------------------------------------------------
// Returns a shared_ptr to the given [archive], or nullptr if it isn't open in
// the archive manager
// -----------------------------------------------------------------------------
shared_ptr<Archive> ArchiveManager::shareArchive(const Archive* const archive)
{
	if (archive == base_resource_archive_.get())
		return base_resource_archive_;

	if (archive == program_resource_archive_.get())
		return program_resource_archive_;

	for (const auto& oa : open_archives_)
		if (oa.archive.get() == archive)
			return oa.archive;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Adds [path] to the list of base resource paths
// -----------------------------------------------------------------------------
bool ArchiveManager::addBaseResourcePath(string_view path)
{
	// Firstly, check the file exists
	if (!fileutil::fileExists(path))
		return false;

	// Second, check the path doesn't already exist
	for (auto& base_resource_path : base_resource_paths_)
	{
		if (base_resource_path == path)
			return false;
	}

	// Add it
	base_resource_paths_.emplace_back(path);

	// Announce
	signals_.base_res_path_added(base_resource_paths_.size() - 1);

	return true;
}

// -----------------------------------------------------------------------------
// Removes the base resource path at [index]
// -----------------------------------------------------------------------------
void ArchiveManager::removeBaseResourcePath(unsigned index)
{
	// Check index
	if (index >= base_resource_paths_.size())
		return;

	// Unload base resource if removed is open
	if (index == static_cast<unsigned>(base_resource))
		openBaseResource(-1);

	// Modify base_resource cvar if needed
	else if (base_resource > static_cast<signed>(index))
		base_resource = base_resource - 1;

	// Remove the path
	base_resource_paths_.erase(base_resource_paths_.begin() + index);

	// Announce
	signals_.base_res_path_removed(index);
}

// -----------------------------------------------------------------------------
// Returns the base resource path at [index]
// -----------------------------------------------------------------------------
string ArchiveManager::getBaseResourcePath(unsigned index)
{
	// Check index
	if (index >= base_resource_paths_.size())
		return "INVALID INDEX";

	return base_resource_paths_[index];
}

// -----------------------------------------------------------------------------
// Returns the currently selected base resource path
// -----------------------------------------------------------------------------
string ArchiveManager::currentBaseResourcePath()
{
	return getBaseResourcePath(base_resource);
}

// -----------------------------------------------------------------------------
// Opens the base resource archive [index]
// -----------------------------------------------------------------------------
bool ArchiveManager::openBaseResource(int index)
{
	// Check we're opening a different archive
	if (base_resource_archive_ && base_resource == index)
		return true;

	// Close/delete current base resource archive
	if (base_resource_archive_)
	{
		app::resources().removeArchive(base_resource_archive_.get());
		base_resource_archive_ = nullptr;
	}

	// Check index
	if (index < 0 || static_cast<unsigned>(index) >= base_resource_paths_.size())
	{
		base_resource = -1;
		signals_.base_res_current_cleared();
		return false;
	}

	// Create archive based on file type
	auto filename = base_resource_paths_[index];
	if (archive::isFormat(filename, ArchiveFormat::Wad))
		base_resource_archive_ = std::make_unique<Archive>(ArchiveFormat::Wad);
	else if (archive::isFormat(filename, ArchiveFormat::Zip))
		base_resource_archive_ = std::make_unique<Archive>(ArchiveFormat::Zip);
	else
		return false;

	// Attempt to open the file
	ui::showSplash(fmt::format("Opening {}...", filename), true, maineditor::windowWx());
	if (base_resource_archive_->open(filename))
	{
		// Update in database
		auto db_id = updateArchiveInDatabase(*base_resource_archive_, false);
		setArchiveDbId(*base_resource_archive_, db_id);

		base_resource = index;
		ui::hideSplash();
		app::resources().addArchive(base_resource_archive_.get());
		signals_.base_res_current_changed(index);
		return true;
	}
	base_resource_archive_ = nullptr;
	ui::hideSplash();
	signals_.base_res_current_changed(index);
	return false;
}

// -----------------------------------------------------------------------------
// Returns the first entry matching [name] in the resource archives.
// Resource archives = open archives -> base resource archives.
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveManager::getResourceEntry(string_view name, const Archive* ignore) const
{
	// Go through all open archives
	for (auto& open_archive : open_archives_)
	{
		// If it isn't a resource archive, skip it
		if (!open_archive.resource)
			continue;

		// If it's being ignored, skip it
		if (open_archive.archive.get() == ignore)
			continue;

		// Try to find the entry in the archive
		if (auto entry = open_archive.archive->entry(name))
			return entry;
	}

	// If entry isn't found yet, search the base resource archive
	if (base_resource_archive_)
		return base_resource_archive_->entry(name);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Searches for an entry matching [options] in the resource archives
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveManager::findResourceEntry(ArchiveSearchOptions& options, const Archive* ignore) const
{
	// Go through all open archives
	for (auto& open_archive : open_archives_)
	{
		// If it isn't a resource archive, skip it
		if (!open_archive.resource)
			continue;

		// If it's being ignored, skip it
		if (open_archive.archive.get() == ignore)
			continue;

		// Try to find the entry in the archive
		if (auto entry = open_archive.archive->findLast(options))
			return entry;
	}

	// If entry isn't found yet, search the base resource archive
	if (base_resource_archive_)
		return base_resource_archive_->findLast(options);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Searches for entries matching [options] in the resource archives
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> ArchiveManager::findAllResourceEntries(ArchiveSearchOptions& options, const Archive* ignore) const
{
	vector<ArchiveEntry*> ret;

	// Search the base resource archive first
	if (base_resource_archive_)
	{
		auto vec = base_resource_archive_->findAll(options);
		ret.insert(ret.end(), vec.begin(), vec.end());
	}

	// Go through all open archives
	for (auto& open_archive : open_archives_)
	{
		// If it isn't a resource archive, skip it
		if (!open_archive.resource)
			continue;

		// If it's being ignored, skip it
		if (open_archive.archive.get() == ignore)
			continue;

		// Add matching entries from this archive
		auto vec = open_archive.archive->findAll(options);
		ret.insert(ret.end(), vec.begin(), vec.end());
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Adds [entry] to the bookmark list
// -----------------------------------------------------------------------------
void ArchiveManager::addBookmark(const shared_ptr<ArchiveEntry>& entry)
{
	if (!entry)
		return;

	// Check the bookmark isn't already in the list
	for (auto& bookmark : bookmarks_)
	{
		if (bookmark.lock() == entry)
			return;
	}

	// Add bookmark
	bookmarks_.push_back(entry);

	// Announce
	signals_.bookmark_added(entry.get());
}

// -----------------------------------------------------------------------------
// Removes [entry] from the bookmarks list
// -----------------------------------------------------------------------------
bool ArchiveManager::deleteBookmark(ArchiveEntry* entry)
{
	// Find bookmark to remove
	for (unsigned a = 0; a < bookmarks_.size(); a++)
	{
		if (bookmarks_[a].lock().get() == entry)
		{
			// Remove it
			bookmarks_.erase(bookmarks_.begin() + a);

			// Announce
			signals_.bookmarks_removed(vector<ArchiveEntry*>(1, entry));

			return true;
		}
	}

	// Entry not in bookmarks list
	return false;
}

// -----------------------------------------------------------------------------
// Removes the bookmarked entry [index]
// -----------------------------------------------------------------------------
bool ArchiveManager::deleteBookmark(unsigned index)
{
	// Check index
	if (index >= bookmarks_.size())
		return false;

	// Remove bookmark
	auto* entry = bookmarks_[index].lock().get();
	bookmarks_.erase(bookmarks_.begin() + index);

	// Announce
	signals_.bookmarks_removed(vector<ArchiveEntry*>(1, entry));

	return true;
}

// -----------------------------------------------------------------------------
// Removes any bookmarked entries in [archive] from the list
// -----------------------------------------------------------------------------
bool ArchiveManager::deleteBookmarksInArchive(const Archive* archive)
{
	// Go through bookmarks
	bool                  removed = false;
	vector<ArchiveEntry*> removed_entries;
	for (unsigned a = 0; a < bookmarks_.size(); a++)
	{
		// Check bookmarked entry's parent archive
		auto bookmark = bookmarks_[a].lock();
		if (!bookmark || bookmark->parent() == archive)
		{
			removed_entries.push_back(bookmark.get());
			bookmarks_.erase(bookmarks_.begin() + a);
			a--;
			removed = true;
		}
	}

	if (removed)
	{
		// Announce
		signals_.bookmarks_removed(removed_entries);
		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Removes any bookmarked entries in [node] from the list
// -----------------------------------------------------------------------------
bool ArchiveManager::deleteBookmarksInDir(const ArchiveDir* node)
{
	auto archive = node->archive();
	bool removed = deleteBookmark(node->dirEntry());

	vector<ArchiveEntry*> removed_entries;
	if (removed)
		removed_entries.push_back(node->dirEntry());

	// Go through bookmarks
	for (unsigned a = 0; a < bookmarks_.size(); ++a)
	{
		// Check bookmarked entry's parent archive
		auto bookmark = bookmarks_[a].lock();
		if (!bookmark || bookmark->parent() == archive)
		{
			bool remove = true;

			// Now check if the bookmarked entry is within
			// the removed dir or one of its descendants
			if (bookmark)
			{
				remove     = false;
				auto anode = bookmark->parentDir();
				while (anode != archive->rootDir().get() && !remove)
				{
					if (anode == node)
						remove = true;
					else
						anode = anode->parent().get();
				}
			}

			if (remove)
			{
				removed_entries.push_back(bookmark.get());
				bookmarks_.erase(bookmarks_.begin() + a);
				--a;
				removed = true;
			}
		}
	}

	if (removed)
	{
		// Announce
		signals_.bookmarks_removed(removed_entries);
		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Clears all bookmarks
// -----------------------------------------------------------------------------
void ArchiveManager::deleteAllBookmarks()
{
	if (!bookmarks_.empty())
	{
		vector<ArchiveEntry*> removed;
		for (const auto& entry : bookmarks_)
			removed.push_back(entry.lock().get());

		bookmarks_.clear();
		signals_.bookmarks_removed(removed);
	}
}

// -----------------------------------------------------------------------------
// Returns the bookmarked entry at [index]
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveManager::getBookmark(unsigned index) const
{
	// Check index
	if (index >= bookmarks_.size())
		return nullptr;

	return bookmarks_[index].lock().get();
}

// -----------------------------------------------------------------------------
// Returns true if [entry] exists in the bookmarks list
// -----------------------------------------------------------------------------
bool ArchiveManager::isBookmarked(const ArchiveEntry* entry) const
{
	for (const auto& bm : bookmarks_)
		if (bm.lock().get() == entry)
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Returns the database ID of the given [archive], or -1 if it doesn't have one
// -----------------------------------------------------------------------------
i64 ArchiveManager::archiveDbId(const Archive& archive) const
{
	for (const auto& oa : open_archives_)
		if (oa.archive.get() == &archive)
			return oa.db_id;

	return -1;
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Lists the filenames of all open archives
// -----------------------------------------------------------------------------
CONSOLE_COMMAND(list_archives, 0, true)
{
	log::info("{} Open Archives:", app::archiveManager().numArchives());

	for (int a = 0; a < app::archiveManager().numArchives(); a++)
	{
		auto archive = app::archiveManager().getArchive(a);
		log::info("{}: \"{}\"", a + 1, archive->filename());
	}
}

// -----------------------------------------------------------------------------
// Attempts to open each given argument (filenames)
// -----------------------------------------------------------------------------
void c_open(const vector<string>& args)
{
	for (const auto& arg : args)
		app::archiveManager().openArchive(arg);
}
ConsoleCommand am_open("open", &c_open, 1, true); // Can't use the macro with this name
