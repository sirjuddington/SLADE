#pragma once

#include "Graphics/Palette/Palette.h"

class ArchiveEntry;
class ExternalEditFileMonitor;
class ExternalEditManager
{
friend class ExternalEditFileMonitor;
public:
	ExternalEditManager();
	~ExternalEditManager();

	bool	openEntryExternal(ArchiveEntry* entry, string editor, string category);

	typedef std::unique_ptr<ExternalEditManager> UPtr;

private:
	vector<ExternalEditFileMonitor*>	file_monitors;

	void	monitorStopped(ExternalEditFileMonitor* monitor);
};
