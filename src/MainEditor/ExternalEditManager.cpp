
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ExternalEditManager.cpp
// Description: ExternalEditManager class, keeps track of all entries currently
//              being edited externally for a single ArchivePanel. Also contains
//              some FileMonitor subclasses for handling export/import of
//              various entry types (conversions, etc.)
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
#include "ExternalEditManager.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Conversions.h"
#include "General/Executables.h"
#include "General/Misc.h"
#include "Graphics/Graphics.h"
#include "Graphics/SImage/SIFormat.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor.h"
#include "Utility/FileMonitor.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;

namespace slade
{
// -----------------------------------------------------------------------------
// ExternalEditFileMonitor Class
//
// FileMonitor subclass to handle exporting, monitoring and reimporting an entry
// -----------------------------------------------------------------------------
class ExternalEditFileMonitor : public FileMonitor
{
public:
	ExternalEditFileMonitor(ArchiveEntry& entry, ExternalEditManager* manager) :
		FileMonitor("", false),
		entry_(&entry),
		archive_{ entry.parent() },
		manager_(manager)
	{
		// Stop monitoring if the entry is removed
		sc_entry_removed_ = archive_->signals().entry_removed.connect(
			[this](Archive&, ArchiveDir&, ArchiveEntry& entry)
			{
				if (&entry == entry_)
					delete this;
			});
	}

	virtual ~ExternalEditFileMonitor() { manager_->monitorStopped(this); }

	ArchiveEntry* getEntry() const { return entry_; }
	void          fileModified() override { updateEntry(); }

	virtual void updateEntry() { entry_->importFile(filename_); }

	virtual bool exportEntry()
	{
		// Determine export filename/path
		strutil::Path fn(app::path(entry_->name(), app::Dir::Temp));
		fn.setExtension(entry_->type()->extension());

		// Export entry and start monitoring
		bool ok = entry_->exportFile(fn.fullPath());
		if (ok)
		{
			filename_      = fn.fullPath();
			file_modified_ = fileutil::fileModifiedTime(filename_);
			Start(1000);
		}
		else
			global::error = "Failed to export entry";

		return ok;
	}

protected:
	ArchiveEntry*              entry_   = nullptr;
	Archive*                   archive_ = nullptr;
	ExternalEditManager*       manager_ = nullptr;
	string                     gfx_format_;
	sigslot::scoped_connection sc_entry_removed_;
};


// -----------------------------------------------------------------------------
// GfxExternalFileMonitor Class
//
// ExternalEditFileMonitor subclass to handle gfx entries
// -----------------------------------------------------------------------------
class GfxExternalFileMonitor : public ExternalEditFileMonitor
{
public:
	GfxExternalFileMonitor(ArchiveEntry& entry, ExternalEditManager* manager) : ExternalEditFileMonitor(entry, manager)
	{
	}
	virtual ~GfxExternalFileMonitor() = default;

	void updateEntry() override
	{
		// Read file
		MemChunk data;
		data.importFile(filename_);

		// Read image
		SImage image;
		image.open(data, 0, "png");
		image.convertPaletted(&palette_);

		// Convert image to entry gfx format
		auto format = SIFormat::getFormat(gfx_format_);
		if (format)
		{
			MemChunk conv_data;
			if (format->saveImage(image, conv_data, &palette_))
			{
				// Update entry data
				entry_->importMemChunk(conv_data);
				gfx::setImageOffsets(entry_->data(), offsets_.x, offsets_.y);
			}
			else
			{
				log::error("Unable to convert external png to {}", format->name());
			}
		}
	}

	bool exportEntry() override
	{
		strutil::Path fn(app::path(entry_->name(), app::Dir::Temp));

		fn.setExtension("png");

		// Create image from entry
		SImage image;
		if (!misc::loadImageFromEntry(&image, entry_))
		{
			global::error = "Could not read graphic";
			return false;
		}

		// Set export info
		gfx_format_ = image.format()->id();
		offsets_    = image.offset();
		palette_.copyPalette(maineditor::currentPalette(entry_));

		// Write png data
		MemChunk png;
		auto     fmt_png = SIFormat::getFormat("png");
		if (!fmt_png->saveImage(image, png, &palette_))
		{
			global::error = "Error converting to png";
			return false;
		}

		// Export file and start monitoring if successful
		filename_ = fn.fullPath();
		if (png.exportFile(filename_))
		{
			file_modified_ = fileutil::fileModifiedTime(filename_);
			Start(1000);
			return true;
		}

		return false;
	}

private:
	string  gfx_format_;
	Vec2i   offsets_;
	Palette palette_;
};


// -----------------------------------------------------------------------------
// MIDIExternalFileMonitor Class
//
// ExternalEditFileMonitor subclass to handle MIDI entries
// -----------------------------------------------------------------------------
class MIDIExternalFileMonitor : public ExternalEditFileMonitor
{
public:
	MIDIExternalFileMonitor(ArchiveEntry& entry, ExternalEditManager* manager) : ExternalEditFileMonitor(entry, manager)
	{
	}
	virtual ~MIDIExternalFileMonitor() = default;

	void updateEntry() override
	{
		// Can't convert back, just import the MIDI
		entry_->importFile(filename_);
	}

	bool exportEntry() override
	{
		strutil::Path fn(app::path(entry_->name(), app::Dir::Temp));
		fn.setExtension("mid");

		// Convert to MIDI data
		MemChunk convdata;

		// MUS
		if (entry_->type()->formatId() == "midi_mus")
			conversion::musToMidi(entry_->data(), convdata);

		// HMI/HMP/XMI
		else if (
			entry_->type()->formatId() == "midi_xmi" || entry_->type()->formatId() == "midi_hmi"
			|| entry_->type()->formatId() == "midi_hmp")
			conversion::zmusToMidi(entry_->data(), convdata, 0);

		// GMID
		else if (entry_->type()->formatId() == "midi_gmid")
			conversion::gmidToMidi(entry_->data(), convdata);

		else
		{
			global::error = fmt::format("Type {} can not be converted to MIDI", entry_->type()->name());
			return false;
		}

		// Export file and start monitoring if successful
		filename_ = fn.fullPath();
		if (convdata.exportFile(filename_))
		{
			file_modified_ = fileutil::fileModifiedTime(filename_);
			Start(1000);
			return true;
		}

		return false;
	}

	static bool canHandleEntry(ArchiveEntry& entry)
	{
		const auto& format_id = entry.type()->formatId();
		return format_id == "midi" || format_id == "midi_mus" || format_id == "midi_xmi" || format_id == "midi_hmi"
			   || format_id == "midi_hmp" || format_id == "midi_gmid";
	}
};


// -----------------------------------------------------------------------------
// SfxExternalFileMonitor Class
//
// ExternalEditFileMonitor subclass to handle sfx entries
// -----------------------------------------------------------------------------
class SfxExternalFileMonitor : public ExternalEditFileMonitor
{
public:
	SfxExternalFileMonitor(ArchiveEntry& entry, ExternalEditManager* manager) :
		ExternalEditFileMonitor(entry, manager),
		doom_sound_(true)
	{
	}
	virtual ~SfxExternalFileMonitor() = default;

	void updateEntry() override
	{
		// Convert back to doom sound if it was originally
		if (doom_sound_)
		{
			MemChunk in, out;
			in.importFile(filename_);
			if (conversion::wavToDoomSnd(in, out))
			{
				// Import converted data to entry if successful
				entry_->importMemChunk(out);
				return;
			}
		}

		// Just import wav to entry if conversion to doom sound
		// failed or the entry was not a convertable type
		entry_->importFile(filename_);
	}

	bool exportEntry() override
	{
		strutil::Path fn(app::path(entry_->name(), app::Dir::Temp));
		fn.setExtension("mid");

		// Convert to WAV data
		MemChunk convdata;

		// Doom Sound
		if (entry_->type()->formatId() == "snd_doom" || entry_->type()->formatId() == "snd_doom_mac")
			conversion::doomSndToWav(entry_->data(), convdata);

		// Doom PC Speaker Sound
		else if (entry_->type()->formatId() == "snd_speaker")
			conversion::spkSndToWav(entry_->data(), convdata);

		// AudioT PC Speaker Sound
		else if (entry_->type()->formatId() == "snd_audiot")
			conversion::spkSndToWav(entry_->data(), convdata, true);

		// Wolfenstein 3D Sound
		else if (entry_->type()->formatId() == "snd_wolf")
			conversion::wolfSndToWav(entry_->data(), convdata);

		// Creative Voice File
		else if (entry_->type()->formatId() == "snd_voc")
			conversion::vocToWav(entry_->data(), convdata);

		// Jaguar Doom Sound
		else if (entry_->type()->formatId() == "snd_jaguar")
			conversion::jagSndToWav(entry_->data(), convdata);

		// Blood Sound
		else if (entry_->type()->formatId() == "snd_bloodsfx")
			conversion::bloodToWav(entry_, convdata);

		else
		{
			global::error = fmt::format("Type {} can not be converted to WAV", entry_->type()->name());
			return false;
		}

		// Export file and start monitoring if successful
		filename_ = fn.fullPath();
		if (convdata.exportFile(filename_))
		{
			file_modified_ = fileutil::fileModifiedTime(filename_);
			Start(1000);
			return true;
		}

		return false;
	}

	static bool canHandleEntry(ArchiveEntry& entry)
	{
		auto& format_id = entry.type()->formatId();
		return format_id == "snd_doom" || format_id == "snd_doom_mac" || format_id == "snd_speaker"
			   || format_id == "snd_audiot" || format_id == "snd_wolf" || format_id == "snd_voc"
			   || format_id == "snd_jaguar" || format_id == "snd_bloodsfx";
	}

private:
	bool doom_sound_;
};
} // namespace slade


// -----------------------------------------------------------------------------
//
// ExternalEditManager Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ExternalEditManager class destructor
// -----------------------------------------------------------------------------
ExternalEditManager::~ExternalEditManager()
{
	destructing_ = true;
	for (auto& file_monitor : file_monitors_)
		delete file_monitor;
}

// -----------------------------------------------------------------------------
// Opens [entry] for external editing with [editor] for [category]
// -----------------------------------------------------------------------------
bool ExternalEditManager::openEntryExternal(ArchiveEntry& entry, string_view editor, string_view category)
{
	// Check the entry isn't already opened externally
	for (auto& file_monitor : file_monitors_)
		if (file_monitor->getEntry() == &entry)
		{
			log::warning("Entry {} is already open in an external editor", entry.name());
			return true;
		}

	// Setup file monitor depending on entry type
	ExternalEditFileMonitor* monitor = nullptr;

	// Gfx entry
	if (entry.type()->editor() == "gfx" && entry.type()->id() != "png")
		monitor = new GfxExternalFileMonitor(entry, this);
	// MIDI entry
	else if (MIDIExternalFileMonitor::canHandleEntry(entry))
		monitor = new MIDIExternalFileMonitor(entry, this);
	// Sfx entry
	else if (SfxExternalFileMonitor::canHandleEntry(entry))
		monitor = new SfxExternalFileMonitor(entry, this);
	// Other entry
	else
		monitor = new ExternalEditFileMonitor(entry, this);

	// Export entry to temp file and start monitoring if successful
	if (!monitor->exportEntry())
	{
		delete monitor;
		return false;
	}

	// Get external editor path
	auto exe_path = executables::externalExe(editor, category).path;
#ifdef WIN32
	if (exe_path.empty() || !fileutil::fileExists(exe_path))
#else
	if (exe_path.empty())
#endif
	{
		global::error = fmt::format("External editor {} has invalid path", editor);
		delete monitor;
		return false;
	}

	// Run external editor
	auto command = fmt::format("\"{}\" \"{}\"", exe_path, monitor->filename());
	long success = wxExecute(wxString::FromUTF8(command), wxEXEC_ASYNC, monitor->process());
	if (success == 0)
	{
		global::error = fmt::format("Failed to launch {}", editor);
		delete monitor;
		return false;
	}

	// Add to list of file monitors for tracking
	file_monitors_.push_back(monitor);

	return true;
}

// -----------------------------------------------------------------------------
// Called when a FileMonitor is stopped/deleted
// -----------------------------------------------------------------------------
void ExternalEditManager::monitorStopped(ExternalEditFileMonitor* monitor)
{
	// Ignore if we're in the destructor, to avoid modifying file_monitors_
	// while we're iterating through it
	if (destructing_)
		return;

	if (VECTOR_EXISTS(file_monitors_, monitor))
		VECTOR_REMOVE(file_monitors_, monitor);
}
