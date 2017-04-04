
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ExternalEditManager.cpp
 * Description: ExternalEditManager class, keeps track of all
 *              entries currently being edited externally for a
 *              single ArchivePanel. Also contains some FileMonitor
 *              subclasses for handling export/import of various
 *              entry types (conversions, etc.)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "App.h"
#include "ExternalEditManager.h"
#include "Archive/Archive.h"
#include "Conversions.h"
#include "EntryOperations.h"
#include "General/Misc.h"
#include "General/Executables.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/SImage/SIFormat.h"
#include "MainEditor.h"
#include "Utility/FileMonitor.h"


/*******************************************************************
 * EXTERNALEDITFILEMONITOR CLASS
 *******************************************************************
 * FileMonitor subclass to handle exporting, monitoring and
 * re-importing an entry
 */
class ExternalEditFileMonitor : public FileMonitor, Listener
{
public:
	ExternalEditFileMonitor(ArchiveEntry* entry, ExternalEditManager* manager)
		: FileMonitor("", false),
		manager(manager),
		entry(entry)
	{
		// Listen to entry parent archive
		listenTo(entry->getParent());
	}

	virtual ~ExternalEditFileMonitor()
	{
		manager->monitorStopped(this);
	}

	ArchiveEntry*	getEntry() { return entry; }
	void			fileModified() { updateEntry(); }

	virtual void updateEntry()
	{
		entry->importFile(filename);
	}

	virtual bool exportEntry()
	{
		// Determine export filename/path
		wxFileName fn(App::path(entry->getName(), App::Dir::Temp));
		fn.SetExt(entry->getType()->getExtension());

		// Export entry and start monitoring
		bool ok = entry->exportFile(fn.GetFullPath());
		if (ok)
		{
			filename = fn.GetFullPath();
			file_modified = wxFileModificationTime(filename);
			Start(1000);
		}
		else
			Global::error = "Failed to export entry";

		return ok;
	}

	void onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
	{
		if (announcer != entry->getParent())
			return;

		bool finished = false;

		// Entry removed
		if (event_name == "entry_removed")
		{
			int index;
			wxUIntPtr ptr;
			event_data.read(&index, sizeof(int));
			event_data.read(&ptr, sizeof(wxUIntPtr));
			if (wxUIntToPtr(ptr) == entry)
				finished = true;
		}

		if (finished)
			delete this;
	}

protected:
	ArchiveEntry*			entry;
	ExternalEditManager*	manager;
	string					gfx_format;
};


/*******************************************************************
 * GFXEXTERNALFILEMONITOR CLASS
 *******************************************************************
 * ExternalEditFileMonitor subclass to handle gfx entries
 */
class GfxExternalFileMonitor : public ExternalEditFileMonitor
{
public:
	GfxExternalFileMonitor(ArchiveEntry* entry, ExternalEditManager* manager)
		: ExternalEditFileMonitor(entry, manager) {}
	virtual ~GfxExternalFileMonitor() {}

	void updateEntry()
	{
		// Read file
		MemChunk data;
		data.importFile(filename);

		// Read image
		SImage image;
		image.open(data, 0, "png");
		image.convertPaletted(&palette);

		// Convert image to entry gfx format
		SIFormat* format = SIFormat::getFormat(gfx_format);
		if (format)
		{
			MemChunk conv_data;
			if (format->saveImage(image, conv_data, &palette))
			{
				// Update entry data
				entry->importMemChunk(conv_data);
				EntryOperations::setGfxOffsets(entry, offsets.x, offsets.y);
			}
			else
			{
				LOG_MESSAGE(1, "Unable to convert external png to %s", format->getName());
			}
		}
	}

	bool exportEntry()
	{
		wxFileName fn(App::path(entry->getName(), App::Dir::Temp));

		fn.SetExt("png");

		// Create image from entry
		SImage image;
		if (!Misc::loadImageFromEntry(&image, entry))
		{
			Global::error = "Could not read graphic";
			return false;
		}

		// Set export info
		gfx_format = image.getFormat()->getId();
		offsets = image.offset();
		palette.copyPalette(MainEditor::currentPalette(entry));

		// Write png data
		MemChunk png;
		SIFormat* fmt_png = SIFormat::getFormat("png");
		if (!fmt_png->saveImage(image, png, &palette))
		{
			Global::error = "Error converting to png";
			return false;
		}

		// Export file and start monitoring if successful
		filename = fn.GetFullPath();
		if (png.exportFile(filename))
		{
			file_modified = wxFileModificationTime(filename);
			Start(1000);
			return true;
		}

		return false;
	}

private:
	string		gfx_format;
	point2_t	offsets;
	Palette8bit	palette;
};


/*******************************************************************
 * MIDIEXTERNALFILEMONITOR CLASS
 *******************************************************************
 * ExternalEditFileMonitor subclass to handle MIDI entries
 */
class MIDIExternalFileMonitor : public ExternalEditFileMonitor
{
public:
	MIDIExternalFileMonitor(ArchiveEntry* entry, ExternalEditManager* manager)
		: ExternalEditFileMonitor(entry, manager) {}
	virtual ~MIDIExternalFileMonitor() {}

	void updateEntry()
	{
		// Can't convert back, just import the MIDI
		entry->importFile(filename);
	}

	bool exportEntry()
	{
		wxFileName fn(App::path(entry->getName(), App::Dir::Temp));
		fn.SetExt("mid");

		// Convert to MIDI data
		MemChunk convdata;

		// MUS
		if (entry->getType()->getFormat() == "midi_mus")
			Conversions::musToMidi(entry->getMCData(), convdata);

		// HMI/HMP/XMI
		else if (entry->getType()->getFormat() == "midi_xmi" || 
			entry->getType()->getFormat() == "midi_hmi" || entry->getType()->getFormat() == "midi_hmp")
			Conversions::zmusToMidi(entry->getMCData(), convdata, 0);

		// GMID
		else if (entry->getType()->getFormat() == "midi_gmid")
			Conversions::gmidToMidi(entry->getMCData(), convdata);

		else
		{
			Global::error = S_FMT("Type %s can not be converted to MIDI", CHR(entry->getType()->getName()));
			return false;
		}

		// Export file and start monitoring if successful
		filename = fn.GetFullPath();
		if (convdata.exportFile(filename))
		{
			file_modified = wxFileModificationTime(filename);
			Start(1000);
			return true;
		}

		return false;
	}

	static bool canHandleEntry(ArchiveEntry* entry)
	{
		if (entry->getType()->getFormat() == "midi" ||
			entry->getType()->getFormat() == "midi_mus" ||
			entry->getType()->getFormat() == "midi_xmi" ||
			entry->getType()->getFormat() == "midi_hmi" ||
			entry->getType()->getFormat() == "midi_hmp" ||
			entry->getType()->getFormat() == "midi_gmid"
			)
			return true;
		
		return false;
	}
};


/*******************************************************************
 * SFXEXTERNALFILEMONITOR CLASS
 *******************************************************************
 * ExternalEditFileMonitor subclass to handle sfx entries
 */
class SfxExternalFileMonitor : public ExternalEditFileMonitor
{
public:
	SfxExternalFileMonitor(ArchiveEntry* entry, ExternalEditManager* manager)
		: ExternalEditFileMonitor(entry, manager), doom_sound(true) {}
	virtual ~SfxExternalFileMonitor() {}

	void updateEntry()
	{
		// Convert back to doom sound if it was originally
		if (doom_sound)
		{
			MemChunk in, out;
			in.importFile(filename);
			if (Conversions::wavToDoomSnd(in, out))
			{
				// Import converted data to entry if successful
				entry->importMemChunk(out);
				return;
			}
		}

		// Just import wav to entry if conversion to doom sound
		// failed or the entry was not a convertable type
		entry->importFile(filename);
	}

	bool exportEntry()
	{
		wxFileName fn(App::path(entry->getName(), App::Dir::Temp));
		fn.SetExt("mid");

		// Convert to WAV data
		MemChunk convdata;

		// Doom Sound
		if (entry->getType()->getFormat() == "snd_doom" ||
			entry->getType()->getFormat() == "snd_doom_mac")
			Conversions::doomSndToWav(entry->getMCData(), convdata);

		// Doom PC Speaker Sound
		else if (entry->getType()->getFormat() == "snd_speaker")
			Conversions::spkSndToWav(entry->getMCData(), convdata);

		// AudioT PC Speaker Sound
		else if (entry->getType()->getFormat() == "snd_audiot")
			Conversions::spkSndToWav(entry->getMCData(), convdata, true);

		// Wolfenstein 3D Sound
		else if (entry->getType()->getFormat() == "snd_wolf")
			Conversions::wolfSndToWav(entry->getMCData(), convdata);

		// Creative Voice File
		else if (entry->getType()->getFormat() == "snd_voc")
			Conversions::vocToWav(entry->getMCData(), convdata);

		// Jaguar Doom Sound
		else if (entry->getType()->getFormat() == "snd_jaguar")
			Conversions::jagSndToWav(entry->getMCData(), convdata);

		// Blood Sound
		else if (entry->getType()->getFormat() == "snd_bloodsfx")
			Conversions::bloodToWav(entry, convdata);

		else
		{
			Global::error = S_FMT("Type %s can not be converted to WAV", CHR(entry->getType()->getName()));
			return false;
		}

		// Export file and start monitoring if successful
		filename = fn.GetFullPath();
		if (convdata.exportFile(filename))
		{
			file_modified = wxFileModificationTime(filename);
			Start(1000);
			return true;
		}

		return false;
	}

	static bool canHandleEntry(ArchiveEntry* entry)
	{
		if (entry->getType()->getFormat() == "snd_doom" ||
			entry->getType()->getFormat() == "snd_doom_mac" ||
			entry->getType()->getFormat() == "snd_speaker" ||
			entry->getType()->getFormat() == "snd_audiot" ||
			entry->getType()->getFormat() == "snd_wolf" ||
			entry->getType()->getFormat() == "snd_voc" ||
			entry->getType()->getFormat() == "snd_jaguar" ||
			entry->getType()->getFormat() == "snd_bloodsfx")
			return true;

		return false;
	}

private:
	bool	doom_sound;
};


/*******************************************************************
 * EXTERNALEDITMANAGER CLASS FUNCTIONS
 *******************************************************************/

/* ExternalEditManager::ExternalEditManager
 * ExternalEditManager class constructor
 *******************************************************************/
ExternalEditManager::ExternalEditManager()
{
}

/* ExternalEditManager::~ExternalEditManager
 * ExternalEditManager class destructor
 *******************************************************************/
ExternalEditManager::~ExternalEditManager()
{
	for (unsigned a = 0; a < file_monitors.size(); a++)
		delete file_monitors[a];
}

/* ExternalEditManager::openEntryExternal
 * Opens [entry] for external editing with [editor] for [category]
 *******************************************************************/
bool ExternalEditManager::openEntryExternal(ArchiveEntry* entry, string editor, string category)
{
	// Check the entry isn't already opened externally
	for (unsigned a = 0; a < file_monitors.size(); a++)
		if (file_monitors[a]->getEntry() == entry)
		{
			LOG_MESSAGE(1, "Entry %s is already open in an external editor", entry->getName());
			return true;
		}

	// Setup file monitor depending on entry type
	ExternalEditFileMonitor* monitor = NULL;

	// Gfx entry
	if (entry->getType()->getEditor() == "gfx" && entry->getType()->getId() != "png")
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
	string exe_path = Executables::getExternalExe(editor, category).path;
#ifdef WIN32
	if (exe_path.IsEmpty() || !wxFileExists(exe_path))
#else
	if (exe_path.IsEmpty())
#endif
	{
		Global::error = S_FMT("External editor %s has invalid path", editor);
		delete monitor;
		return false;
	}

	// Run external editor
	string command = S_FMT("\"%s\" \"%s\"", exe_path, monitor->getFilename());
	long success = wxExecute(command, wxEXEC_ASYNC, monitor->getProcess());
	if (success == 0)
	{
		Global::error = S_FMT("Failed to launch %s", editor);
		delete monitor;
		return false;
	}

	// Add to list of file monitors for tracking
	file_monitors.push_back(monitor);

	return true;
}

/* ExternalEditManager::monitorStopped
 * Called when a FileMonitor is stopped/deleted
 *******************************************************************/
void ExternalEditManager::monitorStopped(ExternalEditFileMonitor* monitor)
{
	if (VECTOR_EXISTS(file_monitors, monitor))
		VECTOR_REMOVE(file_monitors, monitor);
}
