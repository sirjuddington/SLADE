#pragma once

namespace slade
{
class Archive;
class ArchiveEntry;
namespace ui
{
	class NewEntryDialog : public wxDialog
	{
	public:
		NewEntryDialog(
			wxWindow*           parent,
			const Archive&      archive,
			const ArchiveEntry* selected_entry,
			bool                new_dir = false);

		wxString entryName() const;
		int      entryType() const;
		wxString parentDirPath() const;

	private:
		wxComboBox* combo_parent_dir_  = nullptr;
		wxChoice*   choice_entry_type_ = nullptr;
		wxTextCtrl* text_entry_name_   = nullptr;

		bool Validate() override;
	};
} // namespace ui
} // namespace slade
