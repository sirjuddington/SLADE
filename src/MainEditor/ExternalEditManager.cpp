
#include "Main.h"
#include "ExternalEditManager.h"
#include "Archive/Archive.h"
#include "Conversions.h"
#include "EntryOperations.h"
#include "General/Misc.h"
#include "General/Executables.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/SImage/SIFormat.h"
#include "MainWindow.h"
#include "Utility/FileMonitor.h"
#include <wx/filename.h>


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
		wxFileName fn(appPath(entry->getName(), DIR_TEMP));
		fn.SetExt(entry->getType()->getExtension());

		// Export entry and start monitoring
		bool ok = entry->exportFile(fn.GetFullPath());
		if (ok)
		{
			filename = fn.GetFullPath();
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



class GfxExternalFileMonitor : public ExternalEditFileMonitor
{
public:
	GfxExternalFileMonitor(ArchiveEntry* entry, ExternalEditManager* manager)
		: ExternalEditFileMonitor(entry, manager) {}
	~GfxExternalFileMonitor() {}

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
		wxFileName fn(appPath(entry->getName(), DIR_TEMP));

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
		palette.copyPalette(theMainWindow->getPaletteChooser()->getSelectedPalette(entry));

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


class MIDIExternalFileMonitor : public ExternalEditFileMonitor
{
public:
	MIDIExternalFileMonitor(ArchiveEntry* entry, ExternalEditManager* manager)
		: ExternalEditFileMonitor(entry, manager) {}
	~MIDIExternalFileMonitor() {}

	void updateEntry()
	{
		// Can't convert back, just import the MIDI
		entry->importFile(filename);
	}

	bool exportEntry()
	{
		wxFileName fn(appPath(entry->getName(), DIR_TEMP));
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







ExternalEditManager::ExternalEditManager()
{
}

ExternalEditManager::~ExternalEditManager()
{
	for (unsigned a = 0; a < file_monitors.size(); a++)
		delete file_monitors[a];
}

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
	if (exe_path.IsEmpty() || !wxFileExists(exe_path))
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

void ExternalEditManager::monitorStopped(ExternalEditFileMonitor* monitor)
{
	for (unsigned a = 0; a < file_monitors.size(); a++)
		if (file_monitors[a] == monitor)
		{
			file_monitors.erase(file_monitors.begin() + a);
			return;
		}
}
