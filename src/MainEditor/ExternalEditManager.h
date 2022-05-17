#pragma once

namespace slade
{
class ArchiveEntry;
class ExternalEditFileMonitor;

class ExternalEditManager
{
	friend class ExternalEditFileMonitor;

public:
	ExternalEditManager() = default;
	~ExternalEditManager();

	bool openEntryExternal(ArchiveEntry& entry, string_view editor, string_view category);

private:
	vector<ExternalEditFileMonitor*> file_monitors_;

	void monitorStopped(const ExternalEditFileMonitor* monitor);
};
} // namespace slade
