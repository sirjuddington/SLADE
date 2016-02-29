
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
	FileMonitor(string filename, bool start = true);
	virtual ~FileMonitor();

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

#endif//__FILE_MONITOR_H__
