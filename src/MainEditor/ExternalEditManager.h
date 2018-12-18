#pragma once

class ArchiveEntry;
class ExternalEditFileMonitor;

class ExternalEditManager
{
	friend class ExternalEditFileMonitor;

public:
	ExternalEditManager() = default;
	~ExternalEditManager();

	bool openEntryExternal(ArchiveEntry* entry, const string& editor, const string& category);

	typedef std::unique_ptr<ExternalEditManager> UPtr;

private:
	vector<ExternalEditFileMonitor*> file_monitors_;

	void monitorStopped(ExternalEditFileMonitor* monitor);
};
