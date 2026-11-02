#pragma once

namespace slade
{
class ArgsControl;
namespace game
{
	struct ArgSpec;
}

class ArgsPanel : public wxScrolled<wxPanel>
{
public:
	ArgsPanel(wxWindow* parent);
	~ArgsPanel() override = default;

	void setup(const game::ArgSpec& args, bool udmf);
	void setValues(int args[5]) const;
	int  argValue(int index) const;
	void onSize(wxSizeEvent& event);

private:
	wxFlexGridSizer* fg_sizer_           = nullptr;
	ArgsControl*     control_args_[5]    = {};
	wxStaticText*    label_args_[5]      = {};
	wxStaticText*    label_args_desc_[5] = {};
};
} // namespace slade
