#pragma once

namespace slade
{
class Archive;

namespace library
{
	void           addOrUpdateArchive(string_view file_path, const Archive& archive);
	vector<string> recentFiles(unsigned count = 20);

	sigslot::signal<>& signalUpdated();
} // namespace library
} // namespace slade
