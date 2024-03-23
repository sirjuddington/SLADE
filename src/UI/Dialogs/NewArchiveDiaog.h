#pragma once

namespace slade
{
class Archive;

namespace ui
{
	class NewArchiveDialog : public wxDialog
	{
	public:
		NewArchiveDialog(wxWindow* parent);
		~NewArchiveDialog() override = default;

		Archive* createdArchive() const;

	private:
		Archive* archive_created_ = nullptr;
	};
} // namespace ui
} // namespace slade
