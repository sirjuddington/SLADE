#pragma once

namespace slade
{
class Archive;
class ArchiveDir;
namespace ui
{
	class NewEntryDialog : public wxDialog
	{
	public:
		NewEntryDialog(wxWindow* parent, const Archive& archive, const ArchiveDir* current_dir, bool new_dir = false);

		string entryName() const;
		int    entryType() const;
		string parentDirPath() const;

	private:
		wxComboBox* combo_parent_dir_  = nullptr;
		wxChoice*   choice_entry_type_ = nullptr;
		wxTextCtrl* text_entry_name_   = nullptr;

		bool Validate() override;
	};
} // namespace ui
} // namespace slade
