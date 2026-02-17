#pragma once

namespace slade
{
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
	bool                             destructing_ = false;

	void monitorStopped(const ExternalEditFileMonitor* monitor);
};
} // namespace slade
