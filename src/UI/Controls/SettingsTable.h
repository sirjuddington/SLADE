#pragma once

namespace slade::ui
{
class LayoutHelper;
struct SettingRow;

class SettingsTable : public wxPanel
{
public:
	SettingsTable(wxWindow* parent, bool border = true, const string& top_section_title = {});
	~SettingsTable() override = default;

	void addSectionSeparator(const string& label, bool top = false);
	void addCheckBox(const string& label, const string& cvar);
	void addRadioButtons(const string& label, const string& cvar, const vector<string>& options);
	void addTextBox(const string& label, const string& cvar);
	void addSpinControl(const string& label, const string& cvar, int min = 0, int max = 100, int initial = 0);
	void addSlider(
		const string& label,
		const string& cvar,
		bool          decimal,
		int           min   = 0,
		int           max   = 100,
		int           step  = 10,
		int           scale = 1);
	void addCustomControl(const string& label, wxWindow* control, int flags = wxEXPAND | wxALIGN_CENTER_VERTICAL);
	void addCustomSizer(const string& label, wxSizer* sizer, int flags = wxEXPAND | wxALIGN_CENTER_VERTICAL);

	void loadSettings() const;
	void applySettings() const;

private:
	wxGridBagSizer* sizer_      = nullptr;
	LayoutHelper*   layout_     = nullptr;
	bool            in_section_ = false;

	vector<unique_ptr<SettingRow>> settings_;

	void addLabel(const string& label, int row);
};
} // namespace slade::ui
