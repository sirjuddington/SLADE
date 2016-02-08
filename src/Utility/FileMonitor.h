
#ifndef __FILE_MONITOR_H__
#define __FILE_MONITOR_H__

#include "General/ListenerAnnouncer.h"
#include <wx/timer.h>
#include <wx/process.h>

class FileMonitor : public wxTimer
{
private:
	wxProcess*	process;

protected:
	string	filename;
	time_t	file_modified;

public:
	FileMonitor(string filename);
	~FileMonitor();

	wxProcess*	getProcess() { return process; }
	string		getFilename() { return filename; }

	virtual void	fileModified() {}
	virtual void	processTerminated() {}

	void	Notify();
	void	onEndProcess(wxProcessEvent& e);
};

class Archive;
class DB2MapFileMonitor : public FileMonitor
{
private:
	Archive*	archive;
	string		map_name;

public:
	DB2MapFileMonitor(string filename, Archive* archive, string map_name);
	~DB2MapFileMonitor();

	void	fileModified();
	void	processTerminated();
};

class ArchiveEntry;
class ExternalEditFileMonitor : public FileMonitor, Listener
{
private:
	ArchiveEntry*	entry;

public:
	ExternalEditFileMonitor(string filename, ArchiveEntry* entry);
	~ExternalEditFileMonitor();

	void	fileModified();
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);
};

#endif//__FILE_MONITOR_H__
