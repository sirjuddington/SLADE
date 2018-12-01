#pragma once

class Archive;

class FileMonitor : public wxTimer
{
public:
	FileMonitor(string filename, bool start = true);
	virtual ~FileMonitor();

	wxProcess* process() { return process_; }
	string     filename() { return filename_; }

	virtual void fileModified() {}
	virtual void processTerminated() {}

	void Notify();
	void onEndProcess(wxProcessEvent& e);

protected:
	string filename_;
	time_t file_modified_;

private:
	wxProcess* process_;
};

class DB2MapFileMonitor : public FileMonitor
{
public:
	DB2MapFileMonitor(string filename, Archive* archive, string map_name);
	~DB2MapFileMonitor();

	void fileModified();
	void processTerminated();

private:
	Archive* archive_;
	string   map_name_;
};
