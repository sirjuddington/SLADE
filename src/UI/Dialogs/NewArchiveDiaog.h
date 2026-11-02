#pragma once

namespace slade::ui
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
} // namespace slade::ui
