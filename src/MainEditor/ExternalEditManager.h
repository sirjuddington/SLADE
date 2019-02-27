#pragma once

class ArchiveEntry;
class ExternalEditFileMonitor;

class ExternalEditManager
{
	friend class ExternalEditFileMonitor;

public:
	ExternalEditManager() = default;
	~ExternalEditManager();

	bool openEntryExternal(ArchiveEntry* entry, std::string_view editor, std::string_view category);

	typedef std::unique_ptr<ExternalEditManager> UPtr;

private:
	vector<ExternalEditFileMonitor*> file_monitors_;

	void monitorStopped(ExternalEditFileMonitor* monitor);
};
