#pragma once

#include "ListView.h"

namespace slade::ui
{
class ArchiveListView : public ListView
{
public:
	ArchiveListView(wxWindow* parent);
	~ArchiveListView() override = default;

	int findArchive(const wxString& path) const;
	int findArchive(string_view path) const;

	void append(string_view path);
	void append(const Archive* archive);
	void insert(int index, string_view path);
	void insert(int index, const Archive* archive);
	void setItem(int index, string_view path);
	void setItem(int index, const Archive* archive);
};
} // namespace slade::ui
