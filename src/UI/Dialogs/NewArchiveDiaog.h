#pragma once

#include "UI/SDialog.h"

namespace slade
{
class Archive;

namespace ui
{
	class NewArchiveDialog : public SDialog
	{
	public:
		NewArchiveDialog(wxWindow* parent);
		~NewArchiveDialog() = default;

		Archive* createdArchive() const;

	private:
		Archive* archive_created_ = nullptr;
	};
} // namespace ui
} // namespace slade
