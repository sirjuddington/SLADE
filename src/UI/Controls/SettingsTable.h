#pragma once

namespace slade::ui
{
class LayoutHelper;
struct SettingRow;

class SettingsTable : public wxPanel
{
public:
	SettingsTable(wxWindow* parent);
	~SettingsTable() override = default;

	void addSectionSeparator(const string& label);
	void addCheckBox(const string& label, const string& cvar);
	void addRadioButtons(const string& label, const string& cvar, const vector<string>& options);
	void addTextBox(const string& label, const string& cvar);
	void addCustomControl(const string& label, wxWindow* control, int flags = wxEXPAND | wxALIGN_CENTER_VERTICAL);

	void loadSettings() const;
	void applySettings() const;

private:
	wxGridBagSizer* sizer_  = nullptr;
	LayoutHelper*   layout_ = nullptr;

	vector<unique_ptr<SettingRow>> settings_;

	void addLabel(const string& label, int row);
};
} // namespace slade::ui
