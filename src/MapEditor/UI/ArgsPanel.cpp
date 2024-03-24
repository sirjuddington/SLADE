
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    ArgsPanel.cpp
// Description: UI for editing thing/line args
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ArgsPanel.h"
#include "Game/Args.h"
#include "General/UI.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
// ArgsControl Class
//
// Helper class that contains controls specific to a particular argument.
// Usually this is a text box, but some args take one of a list of choices,
// flags, etc.
// -----------------------------------------------------------------------------
namespace slade
{
class ArgsControl : public wxPanel
{
public:
	ArgsControl(wxWindow* parent, game::Arg arg) : wxPanel(parent, -1), arg_(std::move(arg))
	{
		SetSizer(new wxBoxSizer(wxVERTICAL));
	}
	~ArgsControl() override = default;

	virtual long getArgValue()         = 0;
	virtual void setArgValue(long val) = 0;

protected:
	game::Arg arg_; // Original arg configuration
};

// -----------------------------------------------------------------------------
// ArgsTextControl Class
//
// Trivial case of an arg control: a text box that can hold a number from 0 to
// 255
// -----------------------------------------------------------------------------
class ArgsTextControl : public ArgsControl
{
public:
	ArgsTextControl(wxWindow* parent, const game::Arg& arg, bool limit_byte) : ArgsControl(parent, arg)
	{
		text_control_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxutil::scaledSize(40, -1));
		if (limit_byte)
			text_control_->SetValidator(wxIntegerValidator<unsigned char>());
		else
			text_control_->SetValidator(wxIntegerValidator<int>());
		GetSizer()->Add(text_control_, wxSizerFlags().Expand());
	}

	// Get the value of the argument from the textbox
	long getArgValue() override
	{
		wxString val = text_control_->GetValue();

		// Empty string means ignore it
		if (val.empty())
			return -1;

		long ret;
		val.ToLong(&ret);
		return ret;
	}

	// Set the value in the textbox
	void setArgValue(long val) override
	{
		if (val < 0)
			text_control_->ChangeValue("");
		else
			text_control_->ChangeValue(wxString::Format("%ld", val));
	}

protected:
	wxTextCtrl* text_control_; // This is the control holding the "real" value
};

// -----------------------------------------------------------------------------
// ComboBoxAwareIntegerValidator
//
// Helper for the combo box.  wxIntegerValidator, by default, will erase the
// entire combo box if one of the labeled numbers is selected, because the
// label isn't a valid number.
// -----------------------------------------------------------------------------
template<typename T> class ComboBoxAwareIntegerValidator : public wxIntegerValidator<T>
{
public:
	ComboBoxAwareIntegerValidator() : wxIntegerValidator<T>() {}
	ComboBoxAwareIntegerValidator(const ComboBoxAwareIntegerValidator& validator) : wxIntegerValidator<T>(validator) {}

	wxObject* Clone() const override { return new ComboBoxAwareIntegerValidator(*this); }

	wxString NormalizeString(const wxString& s) const override
	{
		// If there's a valid selection in the combobox, don't "normalize".
		// This is a highly inappropriate place for this check, but everything
		// else is private and non-virtual.
		auto control = static_cast<wxComboBox*>(wxNumValidatorBase::GetTextEntry());
		if (control->GetSelection() != wxNOT_FOUND)
			return s;

		return wxIntegerValidator<T>::NormalizeString(s);
	}
};

// -----------------------------------------------------------------------------
// ArgsChoiceControl Class
//
// Combo box for an argument that takes one of a set of predefined values.
// -----------------------------------------------------------------------------
class ArgsChoiceControl : public ArgsControl
{
public:
	ArgsChoiceControl(wxWindow* parent, const game::Arg& arg) : ArgsControl(parent, arg)
	{
		choice_control_ = new wxComboBox(this, -1, "", wxDefaultPosition, wxutil::scaledSize(100, -1));
		choice_control_->SetValidator(ComboBoxAwareIntegerValidator<unsigned char>());

		for (const auto& custom_value : arg.custom_values)
			choice_control_->Append(wxString::Format("%d: %s", custom_value.value, custom_value.name));

		GetSizer()->Add(choice_control_, wxSizerFlags().Expand());
		wxWindowBase::Fit();
	}

	long getArgValue() override
	{
		int selected = choice_control_->GetSelection();
		if (selected == wxNOT_FOUND)
		{
			// No match.  User must have entered a value themselves
			wxString val = choice_control_->GetValue();

			// Empty string means ignore it
			if (val.empty())
				return -1;

			long ret;
			val.ToLong(&ret);
			return ret;
		}
		else
		{
			return arg_.custom_values[selected].value;
		}
	}

	void setArgValue(long val) override
	{
		if (val < 0)
		{
			choice_control_->ChangeValue("");
			return;
		}

		// Look for a name for this value
		for (unsigned i = 0; i < arg_.custom_values.size(); i++)
		{
			if (val == arg_.custom_values[i].value)
			{
				choice_control_->SetSelection(i);
				return;
			}
		}
		choice_control_->ChangeValue(wxString::Format("%ld", val));
	}

protected:
	wxComboBox* choice_control_;
};

// -----------------------------------------------------------------------------
// ArgsFlagsControl Class
//
// Set of checkboxes, for an argument that contains flags.
// -----------------------------------------------------------------------------
class ArgsFlagsControl : public ArgsTextControl
{
public:
	ArgsFlagsControl(wxWindow* parent, const game::Arg& arg, bool limit_byte) :
		ArgsTextControl(parent, arg, limit_byte),
		flag_to_bit_group_(arg.custom_flags.size(), 0),
		controls_(arg.custom_flags.size(), nullptr)
	{
		text_control_->Bind(wxEVT_KEY_UP, &ArgsFlagsControl::onKeypress, this);

		// Sometimes multiple bits are used for a set of more than two flags.
		// For example, if 3 is a flag, then it must be one of /four/ flags
		// along with values 0, 1, and 2.  In such cases, we need radio buttons
		// instead of a checkbox.
		// This is not as robust as it could be, but to my knowledge, the only
		// place this gets used is the "type" argument to ZDoom's
		// Sector_Set3DFloor, where the first two bits are an enum.
		vector<int> bit_groups;
		for (const auto& custom_flag : arg.custom_flags)
		{
			int value = custom_flag.value;
			if (isPowerOfTwo(value))
				continue;

			bool found_match = false;
			for (auto& bit_group : bit_groups)
			{
				if (bit_group & value)
				{
					bit_group |= value;
					found_match = true;
					break;
				}
			}
			if (!found_match)
				bit_groups.push_back(value);
		}

		vector<uint8_t> flag_done(arg.custom_flags.size(), 0);
		for (unsigned i = 0; i < arg.custom_flags.size(); i++)
		{
			if (flag_done[i] > 0)
				continue;

			// Check if this flag is part of a group
			int group         = 0;
			int check_against = arg.custom_flags[i].value;
			// Special case: if the value is 0, it has no bits, so assume it's
			// part of the next flag's group
			if (arg.custom_flags[i].value == 0 && i < arg.custom_flags.size() - 1)
				check_against = arg.custom_flags[i + 1].value;
			for (auto bit_group : bit_groups)
				if (bit_group & check_against)
				{
					group = bit_group;
					break;
				}

			if (group)
			{
				addControl(
					new wxRadioButton(
						this,
						-1,
						wxString::Format("%d: %s", arg.custom_flags[i].value, arg.custom_flags[i].name),
						wxDefaultPosition,
						wxDefaultSize,
						wxRB_GROUP),
					i,
					group);
				// Find all the other (later) flags that are part of this same bit group
				for (unsigned ii = i + 1; ii < arg.custom_flags.size(); ii++)
				{
					if (flag_done[ii] > 0)
						continue;
					if (arg.custom_flags[ii].value & group)
					{
						addControl(
							new wxRadioButton(
								this,
								-1,
								wxString::Format("%d: %s", arg.custom_flags[ii].value, arg.custom_flags[ii].name)),
							ii,
							group);
						flag_done[ii] = 1;
					}
				}
			}
			else // not in a group
			{
				wxControl* control = new wxCheckBox(
					this, -1, wxString::Format("%d: %s", arg.custom_flags[i].value, arg.custom_flags[i].name));
				addControl(control, i, 0);
			}
		}

		wxWindowBase::Fit();
	}

	void setArgValue(long val) override
	{
		ArgsTextControl::setArgValue(val);
		updateCheckState(val);
	}

private:
	vector<int> flag_to_bit_group_; // Vector of bitmasks for the groups each flag belongs to, or 0 for an
									// independent flag, parallel to the arg's custom_flags
	vector<wxControl*> controls_;   // Parallel vector of the checkboxes and radio buttons we create

	static bool isPowerOfTwo(long n) { return (n & (n - 1)) == 0; }

	// -------------------------------------------------------------------------
	// Add a checkbox or radio button to the sizer, and perform some
	// bookkeeping.
	// -------------------------------------------------------------------------
	void addControl(wxControl* control, int index, int group)
	{
		GetSizer()->Add(control);
		controls_[index]          = control;
		flag_to_bit_group_[index] = group;
		control->Bind(wxEVT_CHECKBOX, &ArgsFlagsControl::onCheck, this);
		control->Bind(wxEVT_RADIOBUTTON, &ArgsFlagsControl::onCheck, this);
	}

	// -------------------------------------------------------------------------
	// Event handler called when a checkbox or radio button is toggled.
	// Update the value in the textbox.
	// -------------------------------------------------------------------------
	void onCheck(wxCommandEvent& event)
	{
		// Note that this function does NOT recompute the arg value from
		// scratch!  There might be newer flags we don't know about, and
		// blindly erasing them would be rude.  Instead, only twiddle the
		// single flag corresponding to this checkbox.
		event.Skip();

		int val = getArgValue();
		if (val < 0)
			return;

		// Doesn't matter what type of pointer this is; only need it to find
		// the flag index
		auto control = event.GetEventObject();
		for (unsigned i = 0; i < arg_.custom_flags.size(); i++)
		{
			if (controls_[i] == control)
			{
				// Remove the entire group
				if (flag_to_bit_group_[i])
					val &= ~flag_to_bit_group_[i];
				else
					val &= ~arg_.custom_flags[i].value;

				// Then re-add if appropriate
				if (event.IsChecked())
					val |= arg_.custom_flags[i].value;
				ArgsTextControl::setArgValue(val);
				return;
			}
		}
	}

	// -------------------------------------------------------------------------
	// Event handler called when a key is pressed in the textbox.
	// Refresh all the flag states.
	// -------------------------------------------------------------------------
	void onKeypress(wxKeyEvent& event)
	{
		event.Skip();
		updateCheckState(getArgValue());
	}

	// -------------------------------------------------------------------------
	// Do the actual work of updating the checkbox states.
	// -------------------------------------------------------------------------
	void updateCheckState(long val) const
	{
		for (unsigned i = 0; i < arg_.custom_flags.size(); i++)
		{
			if (flag_to_bit_group_[i])
			{
				bool checked = (val >= 0 && (val & flag_to_bit_group_[i]) == arg_.custom_flags[i].value);
				dynamic_cast<wxRadioButton*>(controls_[i])->SetValue(checked);
			}
			else
			{
				bool checked = (val >= 0 && (val & arg_.custom_flags[i].value) == arg_.custom_flags[i].value);
				dynamic_cast<wxCheckBox*>(controls_[i])->SetValue(checked);
			}
		}
	}
};

// -----------------------------------------------------------------------------
// ArgsSpeedControl Class
//
// Arg control that shows a slider for selecting a flat movement speed.
// -----------------------------------------------------------------------------
class ArgsSpeedControl : public ArgsChoiceControl
{
public:
	ArgsSpeedControl(wxWindow* parent, const game::Arg& arg) : ArgsChoiceControl(parent, arg)
	{
		auto row = new wxBoxSizer(wxHORIZONTAL);

		slider_control_ = new wxSlider(this, -1, 0, 0, 255);
		slider_control_->SetLineSize(2);
		slider_control_->SetPageSize(8);
		// Add a tic for every predefined value
		for (const auto& custom_flag : arg.custom_flags)
			slider_control_->SetTick(custom_flag.value);
		slider_control_->Bind(wxEVT_SLIDER, &ArgsSpeedControl::onSlide, this);
		speed_label_ = new wxStaticText(this, -1, "");

		GetSizer()->Detach(choice_control_);
		row->Add(choice_control_, wxSizerFlags(0).Expand());
		row->AddSpacer(ui::pad());
		row->Add(slider_control_, wxSizerFlags(1).Align(wxALIGN_CENTER_VERTICAL));
		GetSizer()->Add(row, wxSizerFlags(1).Expand());
		GetSizer()->Add(speed_label_, wxSizerFlags(1).Expand());

		// The label has its longest value at 0, which makes for an appropriate
		// minimum size
		syncControls(0);

		wxWindowBase::Fit();
	}

	// Set the value in the textbox
	void setArgValue(long val) override { syncControls(val); }

protected:
	wxSlider*     slider_control_;
	wxStaticText* speed_label_;

	void onSlide(wxCommandEvent& event) { syncControls(slider_control_->GetValue()); }

	void syncControls(int value)
	{
		ArgsChoiceControl::setArgValue(value);

		if (value < 0)
		{
			slider_control_->SetValue(0);
			speed_label_->SetLabel("");
		}
		else
		{
			slider_control_->SetValue(value);
			speed_label_->SetLabel(wxString::Format(
				"%s (%.1f units per tic, %.1f units per sec)",
				arg_.speedLabel(value),
				value / 8.0,
				// A tic is 28ms, slightly less than 1/35 of a second
				value / 8.0 * 1000.0 / 28.0));
		}
	}
};
} // namespace slade


// -----------------------------------------------------------------------------
//
// ArgsPanel Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ArgsPanel class constructor
// -----------------------------------------------------------------------------
ArgsPanel::ArgsPanel(wxWindow* parent) : wxScrolled<wxPanel>{ parent, -1, wxDefaultPosition, wxDefaultSize, wxVSCROLL }
{
	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add arg controls
	fg_sizer_ = new wxFlexGridSizer(2, ui::pad(), ui::pad());
	fg_sizer_->AddGrowableCol(1);
	sizer->Add(fg_sizer_, 1, wxEXPAND);

	for (unsigned a = 0; a < 5; a++)
	{
		label_args_[a]      = new wxStaticText(this, -1, "");
		control_args_[a]    = nullptr;
		label_args_desc_[a] = new wxStaticText(this, -1, "", wxDefaultPosition, wxutil::scaledSize(100, -1));
	}

	// Set up vertical scrollbar
	SetScrollRate(0, 10);

	Bind(wxEVT_SIZE, &ArgsPanel::onSize, this);
}

// -----------------------------------------------------------------------------
// Sets up the arg names and descriptions from specification in [args]
// -----------------------------------------------------------------------------
void ArgsPanel::setup(const game::ArgSpec& args, bool udmf)
{
	using game::Arg;

	// Reset stuff (but preserve the values)
	int old_values[5];
	fg_sizer_->Clear();
	for (unsigned a = 0; a < 5; a++)
	{
		if (control_args_[a])
		{
			old_values[a] = control_args_[a]->getArgValue();
			control_args_[a]->Destroy();
		}
		else
			old_values[a] = -1;

		control_args_[a] = nullptr;
		label_args_[a]->SetLabelText(wxString::Format("Arg %d:", a + 1));
		label_args_desc_[a]->Show(false);
	}

	// Setup layout
	for (unsigned a = 0; a < 5; a++)
	{
		auto& arg      = args[a];
		bool  has_desc = false;

		if (static_cast<int>(a) < args.count)
		{
			has_desc = !arg.desc.empty();

			if (arg.type == Arg::Type::Choice)
				control_args_[a] = new ArgsChoiceControl(this, arg);
			else if (arg.type == Arg::Type::Flags)
				control_args_[a] = new ArgsFlagsControl(this, arg, !udmf);
			else if (arg.type == Arg::Type::Speed)
				control_args_[a] = new ArgsSpeedControl(this, arg);
			else
				control_args_[a] = new ArgsTextControl(this, arg, !udmf);
		}
		else
		{
			control_args_[a] = new ArgsTextControl(this, arg, !udmf);
		}

		// Arg name
		label_args_[a]->SetLabelText(wxString::Format("%s:", arg.name));
		fg_sizer_->Add(label_args_[a], wxSizerFlags().Align(wxALIGN_TOP | wxALIGN_RIGHT).Border(wxALL, 4));

		// Arg value
		control_args_[a]->setArgValue(old_values[a]);
		fg_sizer_->Add(control_args_[a], wxSizerFlags().Expand());

		// Arg description
		if (has_desc)
		{
			// Add an empty spacer to the first column
			fg_sizer_->Add(0, 0);
			fg_sizer_->Add(label_args_desc_[a], wxSizerFlags().Expand());
		}
	}

	// We may have changed the minimum size of the window by adding new big
	// controls, so we need to ask the top-level parent to recompute its
	// minimum size
	wxWindow* toplevel = this;
	while (!toplevel->IsTopLevel() && toplevel->GetParent())
		toplevel = toplevel->GetParent();
	auto toplevel_sizer = toplevel->GetSizer();
	if (toplevel_sizer)
	{
		// This is more or less what SetSizerAndFit does, but without resizing
		// the window if not necessary
		toplevel->SetMinClientSize(toplevel_sizer->ComputeFittingClientSize(toplevel));
		auto toplevel_size = toplevel->GetSize();
		auto toplevel_best = toplevel_size;
		toplevel_best.IncTo(toplevel->GetBestSize());
		if (toplevel_best != toplevel_size)
			toplevel->SetSize(toplevel_best);
	}

	// Set the label text last, so very long labels will wrap naturally and not
	// force the window to be ridiculously wide
	Layout();
	int available_width = fg_sizer_->GetColWidths()[1];
	for (int a = 0; a < args.count; a++)
	{
		auto& arg = args[a];

		if (!arg.desc.empty())
		{
			label_args_desc_[a]->Show(true);
			label_args_desc_[a]->SetLabelText(arg.desc);
			label_args_desc_[a]->Wrap(available_width);
		}
	}

	FitInside(); // for wxScrolled's benefit
}

// -----------------------------------------------------------------------------
// Sets the arg values
// -----------------------------------------------------------------------------
void ArgsPanel::setValues(int args[5]) const
{
	for (unsigned a = 0; a < 5; a++)
	{
		control_args_[a]->setArgValue(args[a]);
	}
}

// -----------------------------------------------------------------------------
// Returns the current value for arg [index]
// -----------------------------------------------------------------------------
int ArgsPanel::argValue(int index) const
{
	// Check index
	if (index < 0 || index > 4 || !control_args_[index])
		return -1;

	return control_args_[index]->getArgValue();
}

// -----------------------------------------------------------------------------
// Rewrap the descriptions when the panel is resized
// -----------------------------------------------------------------------------
void ArgsPanel::onSize(wxSizeEvent& event)
{
	event.Skip();

	Layout();
	if (fg_sizer_->GetColWidths().size() > 1)
	{
		int available_width = fg_sizer_->GetColWidths()[1];
		for (auto& text : label_args_desc_)
		{
			// Wrap() puts hard newlines in the label, so we need to remove them
			wxString label = text->GetLabelText();
			label.Replace("\n", " ");
			text->SetLabelText(label);
			text->Wrap(available_width);
		}
	}
}
