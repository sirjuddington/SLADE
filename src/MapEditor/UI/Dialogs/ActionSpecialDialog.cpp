
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    ActionSpecialDialog.cpp
// Description: A dialog that allows selection of an action special
//              (and other related classes)
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
#include "ActionSpecialDialog.h"
#include "Game/Configuration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/GenLineSpecialPanel.h"
#include "SpecialPresetDialog.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/WxUtils.h"
#include <charconv>
#include <utility>

using namespace slade;


// -----------------------------------------------------------------------------
//
// ActionSpecialtreeView Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ActionSpecialTreeView class constructor
// -----------------------------------------------------------------------------
ActionSpecialTreeView::ActionSpecialTreeView(wxWindow* parent) : wxDataViewTreeCtrl{ parent, -1 }, root_{ nullptr }
{
	// Add 'None'
	item_none_ = AppendItem(root_, "0: None");

	// Computing the minimum width of the tree is slightly complicated, since
	// wx doesn't expose it to us directly
	wxClientDC dc(this);
	dc.SetFont(GetFont());
	wxSize textsize;

	// Sort specials in config order
	sorted_specials_.reserve(game::configuration().allActionSpecials().size());
	for (const auto& i : game::configuration().allActionSpecials())
	{
		if (!i.second.defined())
			continue;

		wxString label = wxString::Format("%d: %s", i.second.number(), i.second.name());
		textsize.IncTo(dc.GetTextExtent(label));
		sorted_specials_.push_back({ &i.second, label, {} });
	}
	std::sort(
		sorted_specials_.begin(),
		sorted_specials_.end(),
		[](const auto& a, const auto& b) { return a.special->order() < b.special->order(); });

	filterSpecials();
	Expand(root_);

	// Bind events
	Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, [&](wxDataViewEvent& e) { e.Veto(); });
	Bind(
		wxEVT_DATAVIEW_ITEM_ACTIVATED,
		[&](wxDataViewEvent&)
		{
			if (parent_dialog_)
				parent_dialog_->EndModal(wxID_OK);
		});

	// 64 is an arbitrary fudge factor -- should be at least the width of a
	// scrollbar plus the expand icons plus any extra padding
	int min_width = textsize.GetWidth() + GetIndent() + ui::scalePx(64);
	wxWindowBase::SetMinSize({ min_width, ui::scalePx(200) });
}

// -----------------------------------------------------------------------------
// Returns the action special value for [item]
// -----------------------------------------------------------------------------
int ActionSpecialTreeView::specialNumber(wxDataViewItem item) const
{
	wxString num = GetItemText(item).BeforeFirst(':');
	long     s;
	if (!num.ToLong(&s))
		s = -1;

	return s;
}

// -----------------------------------------------------------------------------
// Finds the item for [special], selects it and ensures it is shown
// -----------------------------------------------------------------------------
void ActionSpecialTreeView::showSpecial(int special, bool focus)
{
	if (special == 0)
	{
		EnsureVisible(item_none_);
		Select(item_none_);
		if (focus)
			SetFocus();
		return;
	}

	// Go through every item
	for (const auto& i : sorted_specials_)
	{
		// Select+show if match
		if (i.special->number() == special)
		{
			EnsureVisible(i.item);
			Select(i.item);
			if (focus)
				SetFocus();
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Returns the currently selected action special value
// -----------------------------------------------------------------------------
int ActionSpecialTreeView::selectedSpecial() const
{
	auto item = GetSelection();
	if (item.IsOk())
		return specialNumber(item);
	else
		return -1;
}

// -----------------------------------------------------------------------------
// Limit the visible specials, based on a filter string.  If it's blank or
// numeric, show everything and select that special; otherwise, split it on
// whitespace, only show specials whose names contain each word, and select the
// first visible special
// -----------------------------------------------------------------------------
void ActionSpecialTreeView::filterSpecials(string filter)
{
	// Unfortunately, there's no filtering on a wxDataViewTreeCtrl, so we must empty the tree (of
	// leaves only, not groups) and then repopulate it
	for (auto& i : sorted_specials_)
	{
		if (i.item.IsOk())
		{
			DeleteItem(i.item);
			i.item = wxDataViewItem(nullptr);
		}
	}

	const int           current_special = selectedSpecial();
	bool                current_visible = false;
	bool                show_everything = false;
	int                 typed_special   = -1;
	vector<string_view> filter_words;
	if (current_special == 0)
		current_visible = true;

	// Check what kind of filter we have: nothing, a number, or text
	strutil::trimIP(filter);
	if (filter.empty())
		show_everything = true;
	else
	{
		// See if it's a number first
		const auto result = std::from_chars(filter.data(), filter.data() + filter.size(), typed_special, 10);
		// This is a weird API
		if (result.ec == std::errc())
			show_everything = true;
	}

	if (!show_everything)
		// Split on spaces and filter by name
		filter_words = strutil::splitV(filter, ' ', true);

	// Now add the items back to the tree, skipping any that don't match the filter
	for (auto& i : sorted_specials_)
	{
		bool show = true;
		for (const auto& word : filter_words)
		{
			if (!strutil::containsCI(i.special->name(), word))
			{
				show = false;
				break;
			}
		}
		if (!show)
			continue;

		if (i.special->number() == current_special)
			current_visible = true;
		i.item = AppendItem(getGroup(i.special->group()), i.label);
	}

	// If we're filtering, expand all the groups
	if (!show_everything)
	{
		for (const auto& g : groups_)
		{
			Expand(g.item);
		}
	}

	// If a number was typed, select that
	if (typed_special >= 0)
		showSpecial(typed_special, false);
	// If not, but the previous selection is still visible, re-select it
	else if (current_special != 0 && current_visible)
		showSpecial(current_special, false);
	// Otherwise, select the first available special
	else
	{
		for (const auto& i : sorted_specials_)
		{
			if (i.item.IsOk())
			{
				Select(i.item);
				EnsureVisible(i.item);
				break;
			}
		}
	}

	// If nothing was a viable selection, fall back to 0
	if (!GetSelection().IsOk())
	{
		Select(item_none_);
		EnsureVisible(item_none_);
	}
}

// -----------------------------------------------------------------------------
// Returns the parent wxDataViewItem representing action special group [group]
// -----------------------------------------------------------------------------
wxDataViewItem ActionSpecialTreeView::getGroup(const wxString& group_name)
{
	// Check if group was already made
	for (auto& group : groups_)
	{
		if (group_name == group.name)
			return group.item;
	}

	// Split group into subgroups
	auto path = wxSplit(group_name, '/');

	// Create group needed
	auto     current  = root_;
	wxString fullpath = "";
	for (unsigned p = 0; p < path.size(); p++)
	{
		if (p > 0)
			fullpath += "/";
		fullpath += path[p];

		bool found = false;
		for (auto& group : groups_)
		{
			if (group.name == fullpath)
			{
				current = group.item;
				found   = true;
				break;
			}
		}

		if (!found)
		{
			current = AppendContainer(current, path[p]);
			groups_.emplace_back(current, fullpath);
		}
	}

	return current;
}


namespace slade
{
// -----------------------------------------------------------------------------
// ArgsControl Class
//
// Helper class that contains controls specific to a particular argument.
// Usually this is a text box, but some args take one of a list of choices,
// flags, etc.
// -----------------------------------------------------------------------------
class ArgsControl : public wxPanel
{
public:
	ArgsControl(wxWindow* parent, game::Arg arg) : wxPanel(parent, -1), arg_(std::move(arg))
	{
		SetSizer(new wxBoxSizer(wxVERTICAL));
	}
	~ArgsControl() = default;

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
		if (val == "")
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
	void updateCheckState(long val)
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
		// Add a tick for every predefined value
		for (const auto& custom_flag : arg.custom_flags)
			slider_control_->SetTick(custom_flag.value);
		slider_control_->Bind(wxEVT_SLIDER, &ArgsSpeedControl::onSlide, this);
		speed_label_ = new wxStaticText(this, -1, "");

		GetSizer()->Detach(choice_control_);
		row->Add(choice_control_, wxSizerFlags(0).Center());
		row->AddSpacer(ui::pad());
		row->Add(slider_control_, wxSizerFlags(1).Center());
		GetSizer()->Add(row, wxSizerFlags(1).Expand());
		GetSizer()->AddSpacer(ui::pad());
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

// -----------------------------------------------------------------------------
// ArgsDelayControl Class
//
// Arg control that shows a slider for selecting an amount of time, and also
// converts it to seconds.
// -----------------------------------------------------------------------------
class ArgsDelayControl : public ArgsChoiceControl
{
public:
	ArgsDelayControl(wxWindow* parent, const game::Arg& arg, int units_per_sec) :
		ArgsChoiceControl(parent, arg),
		units_per_sec_(units_per_sec)
	{
		auto row = new wxBoxSizer(wxHORIZONTAL);

		slider_control_ = new wxSlider(this, -1, 0, 0, 255);
		slider_control_->SetLineSize(2);
		slider_control_->SetPageSize(8);
		// Add a tick for every predefined value
		for (const auto& custom_flag : arg.custom_flags)
			slider_control_->SetTick(custom_flag.value);
		slider_control_->Bind(wxEVT_SLIDER, &ArgsDelayControl::onSlide, this);
		delay_label_ = new wxStaticText(this, -1, "");

		GetSizer()->Detach(choice_control_);
		row->Add(choice_control_, wxSizerFlags(0).Center());
		row->AddSpacer(ui::pad());
		row->Add(slider_control_, wxSizerFlags(1).Center());
		GetSizer()->Add(row, wxSizerFlags(1).Expand());
		GetSizer()->AddSpacer(ui::pad());
		GetSizer()->Add(delay_label_, wxSizerFlags(1).Expand());

		// The label has its longest value at 0, which makes for an appropriate
		// minimum size
		syncControls(0);

		wxWindowBase::Fit();
	}

	// Set the value in the textbox
	void setArgValue(long val) override { syncControls(val); }

protected:
	wxSlider*     slider_control_;
	wxStaticText* delay_label_;
	int           units_per_sec_;

	void onSlide(wxCommandEvent& event) { syncControls(slider_control_->GetValue()); }

	void syncControls(int value)
	{
		ArgsChoiceControl::setArgValue(value);

		if (value < 0)
		{
			slider_control_->SetValue(0);
			delay_label_->SetLabel("");
		}
		else
		{
			slider_control_->SetValue(value);
			delay_label_->SetLabel(fmt::format("{:.2f} seconds", (float)value / units_per_sec_));
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
	int row = 0;
	for (unsigned a = 0; a < 5; a++)
	{
		auto& arg      = args[a];
		bool  has_desc = false;

		if ((int)a < args.count)
		{
			has_desc = !arg.desc.empty();

			if (arg.type == Arg::Type::Choice)
				control_args_[a] = new ArgsChoiceControl(this, arg);
			else if (arg.type == Arg::Type::Flags)
				control_args_[a] = new ArgsFlagsControl(this, arg, !udmf);
			else if (arg.type == Arg::Type::Speed)
				control_args_[a] = new ArgsSpeedControl(this, arg);
			else if (arg.type == Arg::Type::Tics)
				control_args_[a] = new ArgsDelayControl(this, arg, 35);
			else if (arg.type == Arg::Type::Octics)
				control_args_[a] = new ArgsDelayControl(this, arg, 8);
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
void ArgsPanel::setValues(int args[5])
{
	for (unsigned a = 0; a < 5; a++)
	{
		control_args_[a]->setArgValue(args[a]);
	}
}

// -----------------------------------------------------------------------------
// Returns the current value for arg [index]
// -----------------------------------------------------------------------------
int ArgsPanel::argValue(int index)
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


// -----------------------------------------------------------------------------
//
// ActionSpecialPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ActionSpecialPanel class constructor
// -----------------------------------------------------------------------------
ActionSpecialPanel::ActionSpecialPanel(wxWindow* parent, bool trigger) : wxPanel(parent, -1)
{
	panel_args_     = nullptr;
	choice_trigger_ = nullptr;
	show_trigger_   = trigger;

	// Setup layout
	auto sizer = new wxBoxSizer(wxVERTICAL);

	if (game::configuration().featureSupported(game::Feature::Boom))
	{
		// Action Special radio button
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxBOTTOM, ui::pad());
		rb_special_ = new wxRadioButton(this, -1, "Action Special", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		hbox->Add(rb_special_, 0, wxEXPAND | wxRIGHT, ui::pad());

		// Generalised Special radio button
		rb_generalised_ = new wxRadioButton(this, -1, "Generalised Special");
		hbox->Add(rb_generalised_, 0, wxEXPAND);

		// Boom generalised line special panel
		panel_gen_specials_ = new GenLineSpecialPanel(this);
		panel_gen_specials_->Show(false);

		// Bind events
		rb_special_->Bind(wxEVT_RADIOBUTTON, &ActionSpecialPanel::onRadioButtonChanged, this);
		rb_generalised_->Bind(wxEVT_RADIOBUTTON, &ActionSpecialPanel::onRadioButtonChanged, this);
	}

	// Action specials tree
	setupSpecialPanel();
	sizer->Add(panel_action_special_, 1, wxEXPAND);

	SetSizerAndFit(sizer);

	// Bind events
	tree_specials_->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ActionSpecialPanel::onSpecialSelectionChanged, this);
	tree_specials_->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ActionSpecialPanel::onSpecialItemActivated, this);
}

// -----------------------------------------------------------------------------
// Creates and sets up the action special panel
// -----------------------------------------------------------------------------
void ActionSpecialPanel::setupSpecialPanel()
{
	// Create panel
	panel_action_special_ = new wxPanel(this, -1);
	auto sizer            = new wxBoxSizer(wxVERTICAL);

	// Special box
	text_special_ = new wxTextCtrl(panel_action_special_, wxID_ANY);
	sizer->Add(text_special_, 0, wxEXPAND | wxBOTTOM, ui::pad());
	// Typing in the box acts as a filter
	text_special_->Bind(
		wxEVT_TEXT,
		[&](wxCommandEvent& e)
		{
			// This method calls Select on itself, but we don't want to treat that as a real selection
			// change, or we'll alter the text and recurse into it.  Disable the event while calling it
			auto selection       = selectedSpecial();
			ignore_select_event_ = true;
			tree_specials_->filterSpecials(text_special_->GetValue().ToStdString());
			ignore_select_event_ = false;

			if (selection != selectedSpecial())
				updateArgsPanel();
		});
	// Focusing the text also select-alls; if you leave and return you probably want to start over,
	// not make small edits
	text_special_->Bind(wxEVT_SET_FOCUS, [&](wxFocusEvent& e) { text_special_->SetSelection(-1, -1); });

	// Action specials tree
	tree_specials_ = new ActionSpecialTreeView(panel_action_special_);
	sizer->Add(tree_specials_, 1, wxEXPAND);

	if (show_trigger_)
	{
		// UDMF Triggers
		if (mapeditor::editContext().mapDesc().format == MapFormat::UDMF)
		{
			// Get all UDMF properties
			auto& props = game::configuration().allUDMFProperties(MapObject::Type::Line);

			// Get all UDMF trigger properties
			std::map<wxString, wxFlexGridSizer*> named_flexgrids;
			for (auto& i : props)
			{
				if (!i.second.isTrigger())
					continue;

				wxString group       = i.second.group();
				auto     frame_sizer = named_flexgrids[group];
				if (!frame_sizer)
				{
					auto frame_triggers = new wxStaticBox(panel_action_special_, -1, group);
					auto sizer_triggers = new wxStaticBoxSizer(frame_triggers, wxVERTICAL);
					sizer->Add(sizer_triggers, 0, wxEXPAND | wxTOP, ui::pad());

					frame_sizer = new wxFlexGridSizer(3, ui::pad() / 2, ui::pad());
					frame_sizer->AddGrowableCol(0, 1);
					frame_sizer->AddGrowableCol(1, 1);
					frame_sizer->AddGrowableCol(2, 1);
					sizer_triggers->Add(frame_sizer, 1, wxEXPAND | wxALL, ui::pad());

					named_flexgrids.find(group)->second = frame_sizer;
				}

				auto cb_trigger = new wxCheckBox(
					panel_action_special_, -1, i.second.name(), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
				frame_sizer->Add(cb_trigger, 0, wxEXPAND);

				flags_.push_back({ cb_trigger, -1, i.second.propName() });
			}
		}

		// Hexen trigger
		else if (mapeditor::editContext().mapDesc().format == MapFormat::Hexen)
		{
			auto frame_trigger = new wxStaticBox(panel_action_special_, -1, "Special Trigger");
			auto sizer_trigger = new wxStaticBoxSizer(frame_trigger, wxVERTICAL);
			sizer->Add(sizer_trigger, 0, wxEXPAND | wxALL, ui::pad());

			// Add triggers dropdown
			auto spac_triggers = wxutil::arrayStringStd(game::configuration().allSpacTriggers());
			choice_trigger_ = new wxChoice(panel_action_special_, -1, wxDefaultPosition, wxDefaultSize, spac_triggers);
			sizer_trigger->Add(choice_trigger_, 0, wxEXPAND | wxALL, ui::pad());

			// Add activation-related flags
			auto fg_sizer = new wxFlexGridSizer(3, ui::pad() / 2, ui::pad());
			fg_sizer->AddGrowableCol(0, 1);
			fg_sizer->AddGrowableCol(1, 1);
			fg_sizer->AddGrowableCol(2, 1);
			sizer_trigger->Add(fg_sizer, 0, wxEXPAND | wxALL, ui::pad());
			for (unsigned a = 0; a < game::configuration().nLineFlags(); a++)
			{
				if (game::configuration().lineFlag(a).activation)
				{
					flags_.push_back(
						{ new wxCheckBox(panel_action_special_, -1, game::configuration().lineFlag(a).name),
						  (int)a,
						  game::configuration().lineFlag(a).udmf });
					fg_sizer->Add(flags_.back().check_box, 0, wxEXPAND);
				}
			}
		}

		// Preset button
		btn_preset_ = new wxButton(panel_action_special_, -1, "Preset...");
		sizer->Add(btn_preset_, 0, wxALIGN_RIGHT | wxTOP, ui::pad());
		btn_preset_->Bind(wxEVT_BUTTON, &ActionSpecialPanel::onSpecialPresetClicked, this);
	}

	panel_action_special_->SetSizerAndFit(sizer);
}

// -----------------------------------------------------------------------------
// Selects the item for special [special] in the specials tree
// -----------------------------------------------------------------------------
void ActionSpecialPanel::setSpecial(int special)
{
	// Check for boom generalised special
	if (game::configuration().featureSupported(game::Feature::Boom))
	{
		if (panel_gen_specials_->loadSpecial(special))
		{
			rb_generalised_->SetValue(true);
			showGeneralised(true);
			panel_gen_specials_->SetFocus();
			return;
		}
		else
			rb_special_->SetValue(true);
	}

	// Regular action special
	showGeneralised(false);
	tree_specials_->showSpecial(special, false);
	text_special_->SetValue(wxString::Format("%d", special));

	updateArgsPanel();
}

// -----------------------------------------------------------------------------
// Sets the action special trigger (hexen or udmf)
// -----------------------------------------------------------------------------
void ActionSpecialPanel::setTrigger(int index)
{
	if (!show_trigger_)
		return;

	// Hexen trigger
	if (choice_trigger_)
		choice_trigger_->SetSelection(index);

	// UDMF Trigger
	else if (index < (int)flags_.size())
		flags_[index].check_box->SetValue(true);
}

// -----------------------------------------------------------------------------
// Sets the action special trigger from a udmf trigger name (hexen or udmf)
// -----------------------------------------------------------------------------
void ActionSpecialPanel::setTrigger(const wxString& trigger)
{
	if (!show_trigger_)
		return;

	// Hexen trigger
	if (choice_trigger_)
	{
		for (unsigned a = 0; a < choice_trigger_->GetCount(); a++)
			if (game::configuration().spacTriggerUDMFName(a) == trigger)
			{
				choice_trigger_->SetSelection(a);
				break;
			}
	}

	// UDMF Trigger or Hexen Flag
	for (auto& flag : flags_)
		if (flag.udmf == trigger)
		{
			flag.check_box->SetValue(true);
			break;
		}
}

// -----------------------------------------------------------------------------
// Deselects all triggers (or resets to 'player cross' in hexen format)
// -----------------------------------------------------------------------------
void ActionSpecialPanel::clearTrigger()
{
	// UDMF Triggers and Flags
	for (auto& flag : flags_)
		flag.check_box->SetValue(false);

	// Hexen trigger
	if (choice_trigger_)
		choice_trigger_->SetSelection(0);
}

// -----------------------------------------------------------------------------
// Returns the currently selected action special
// -----------------------------------------------------------------------------
int ActionSpecialPanel::selectedSpecial() const
{
	if (game::configuration().featureSupported(game::Feature::Boom))
	{
		if (rb_special_->GetValue())
			return tree_specials_->selectedSpecial();
		else
			return panel_gen_specials_->special();
	}
	else
		return tree_specials_->selectedSpecial();
}

// -----------------------------------------------------------------------------
// If [show] is true, show the generalised special panel, otherwise show the
// action special tree
// -----------------------------------------------------------------------------
void ActionSpecialPanel::showGeneralised(bool show)
{
	if (!game::configuration().featureSupported(game::Feature::Boom))
		return;

	if (show)
	{
		auto sizer = GetSizer();
		sizer->Replace(panel_action_special_, panel_gen_specials_);
		panel_action_special_->Show(false);
		panel_gen_specials_->Show(true);
		Layout();
	}
	else
	{
		auto sizer = GetSizer();
		sizer->Replace(panel_gen_specials_, panel_action_special_);
		panel_action_special_->Show(true);
		panel_gen_specials_->Show(false);
		Layout();
	}
}

// -----------------------------------------------------------------------------
// Applies selected special (if [apply_special] is true), trigger(s) and args
// (if any) to [lines]
// -----------------------------------------------------------------------------
void ActionSpecialPanel::applyTo(vector<MapObject*>& lines, bool apply_special)
{
	// Special
	int special = selectedSpecial();
	if (apply_special && special >= 0)
	{
		for (auto& line : lines)
			line->setIntProperty("special", special);
	}

	// Args
	if (panel_args_)
	{
		// Get values
		int args[5];
		args[0] = panel_args_->argValue(0);
		args[1] = panel_args_->argValue(1);
		args[2] = panel_args_->argValue(2);
		args[3] = panel_args_->argValue(3);
		args[4] = panel_args_->argValue(4);

		for (auto& line : lines)
		{
			if (args[0] >= 0)
				line->setIntProperty("arg0", args[0]);
			if (args[1] >= 0)
				line->setIntProperty("arg1", args[1]);
			if (args[2] >= 0)
				line->setIntProperty("arg2", args[2]);
			if (args[3] >= 0)
				line->setIntProperty("arg3", args[3]);
			if (args[4] >= 0)
				line->setIntProperty("arg4", args[4]);
		}
	}

	// Trigger(s)
	if (show_trigger_)
	{
		for (auto& line : lines)
		{
			// Hexen
			if (choice_trigger_)
				game::configuration().setLineSpacTrigger(choice_trigger_->GetSelection(), (MapLine*)line);

			// UDMF / Flags
			for (auto& flag : flags_)
			{
				if (flag.check_box->Get3StateValue() == wxCHK_UNDETERMINED)
					continue;

				if (choice_trigger_)
					game::configuration().setLineFlag(flag.index, (MapLine*)line, flag.check_box->GetValue());
				else
					line->setBoolProperty(flag.udmf.ToStdString(), flag.check_box->GetValue());
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Loads special/trigger/arg values from [lines]
// -----------------------------------------------------------------------------
void ActionSpecialPanel::openLines(vector<MapObject*>& lines)
{
	if (lines.empty())
		return;

	// Special
	int special = lines[0]->intProperty("special");
	MapObject::multiIntProperty(lines, "special", special);
	setSpecial(special);

	// Args
	if (panel_args_)
	{
		int args[5] = { -1, -1, -1, -1, -1 };
		MapObject::multiIntProperty(lines, "arg0", args[0]);
		MapObject::multiIntProperty(lines, "arg1", args[1]);
		MapObject::multiIntProperty(lines, "arg2", args[2]);
		MapObject::multiIntProperty(lines, "arg3", args[3]);
		MapObject::multiIntProperty(lines, "arg4", args[4]);
		panel_args_->setValues(args);
	}

	// Trigger
	if (show_trigger_)
	{
		// Hexen
		if (choice_trigger_)
		{
			int trigger = game::configuration().spacTriggerIndexHexen((MapLine*)lines[0]);
			for (unsigned a = 1; a < lines.size(); a++)
			{
				if (trigger != game::configuration().spacTriggerIndexHexen((MapLine*)lines[a]))
				{
					trigger = -1;
					break;
				}
			}

			if (trigger >= 0)
				choice_trigger_->SetSelection(trigger);

			// Flags
			for (auto& flag : flags_)
			{
				// Set initial flag checked value
				flag.check_box->SetValue(game::configuration().lineFlagSet(flag.index, (MapLine*)lines[0]));

				// Go through subsequent lines
				for (unsigned b = 1; b < lines.size(); b++)
				{
					// Check for mismatch
					if (flag.check_box->GetValue() != game::configuration().lineFlagSet(flag.index, (MapLine*)lines[b]))
					{
						// Set undefined
						flag.check_box->Set3StateValue(wxCHK_UNDETERMINED);
						break;
					}
				}
			}
		}

		// UDMF
		else
		{
			for (auto& flag : flags_)
			{
				bool set;
				if (MapObject::multiBoolProperty(lines, flag.udmf.ToStdString(), set))
					flag.check_box->SetValue(set);
				else
					flag.check_box->Set3StateValue(wxCHK_UNDETERMINED);
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Update the arg names/types on the args panel
// -----------------------------------------------------------------------------
void ActionSpecialPanel::updateArgsPanel()
{
	if (panel_args_)
	{
		auto& args = game::configuration().actionSpecial(selectedSpecial()).argSpec();
		panel_args_->setup(args, (mapeditor::editContext().mapDesc().format == MapFormat::UDMF));
	}
}


// -----------------------------------------------------------------------------
//
// ActionSpecialPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the radio button selection is changed
// -----------------------------------------------------------------------------
void ActionSpecialPanel::onRadioButtonChanged(wxCommandEvent& e)
{
	// Swap panels
	showGeneralised(rb_generalised_->GetValue());
}

// -----------------------------------------------------------------------------
// Called when the action special selection is changed
// -----------------------------------------------------------------------------
void ActionSpecialPanel::onSpecialSelectionChanged(wxDataViewEvent& e)
{
	if (ignore_select_event_ || selectedSpecial() < 0
		|| (game::configuration().featureSupported(game::Feature::Boom) && rb_generalised_->GetValue()))
	{
		e.Skip();
		return;
	}

	// Set special # text box
	text_special_->SetValue(wxString::Format("%d", selectedSpecial()));

	updateArgsPanel();
}

// -----------------------------------------------------------------------------
// Called when the action special item is activated
// (double-clicked or enter pressed)
// -----------------------------------------------------------------------------
void ActionSpecialPanel::onSpecialItemActivated(wxDataViewEvent& e)
{
	if (tree_specials_->GetChildCount(e.GetItem()) > 0)
	{
		tree_specials_->Expand(e.GetItem());
		e.Skip();
		return;
	}

	// Jump to args tab, if there is one
	updateArgsPanel();
	if (panel_args_)
		panel_args_->SetFocus();
}

// -----------------------------------------------------------------------------
// Called when the special preset button is clicked
// -----------------------------------------------------------------------------
void ActionSpecialPanel::onSpecialPresetClicked(wxCommandEvent& e)
{
	// Open Special Preset Dialog
	SpecialPresetDialog dlg(this);
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		auto preset = dlg.selectedPreset();
		if (preset.special > 0)
		{
			// Set Special
			setSpecial(preset.special);

			// Set Args
			if (panel_args_)
				panel_args_->setValues(preset.args);

			// Set Flags
			clearTrigger();
			for (auto& flag : preset.flags)
				setTrigger(flag);
		}
	}
}


// -----------------------------------------------------------------------------
//
// ActionSpecialDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ActionSpecialDialog class constructor
// -----------------------------------------------------------------------------
ActionSpecialDialog::ActionSpecialDialog(wxWindow* parent, bool show_args) :
	SDialog{ parent, "Select Action Special", "actionspecial", 400, 500 }
{
	panel_args_ = nullptr;
	auto sizer  = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// No args
	if (mapeditor::editContext().mapDesc().format == MapFormat::Doom || !show_args)
	{
		panel_special_ = new ActionSpecialPanel(this, false);
		sizer->Add(panel_special_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, ui::padLarge());
	}

	// Args (use tabs)
	else
	{
		stc_tabs_ = STabCtrl::createControl(this);
		sizer->Add(stc_tabs_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, ui::padLarge());

		// Special panel
		panel_special_ = new ActionSpecialPanel(this);
		stc_tabs_->AddPage(wxutil::createPadPanel(stc_tabs_, panel_special_), "Special");

		// Args panel
		panel_args_ = new ArgsPanel(this);
		stc_tabs_->AddPage(wxutil::createPadPanel(stc_tabs_, panel_args_), "Args");
		panel_special_->setArgsPanel(panel_args_);
	}

	// Add buttons
	sizer->AddSpacer(ui::pad());
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::padLarge());

	// Init
	SetSizerAndFit(sizer);
	CenterOnParent();
}

// -----------------------------------------------------------------------------
// Selects the item for special [special] in the specials tree
// -----------------------------------------------------------------------------
void ActionSpecialDialog::setSpecial(int special) const
{
	panel_special_->setSpecial(special);
}

// -----------------------------------------------------------------------------
// Sets the arg values
// -----------------------------------------------------------------------------
void ActionSpecialDialog::setArgs(int args[5]) const
{
	if (panel_args_)
		panel_args_->setValues(args);
}

// -----------------------------------------------------------------------------
// Returns the currently selected action special
// -----------------------------------------------------------------------------
int ActionSpecialDialog::selectedSpecial() const
{
	return panel_special_->selectedSpecial();
}

// -----------------------------------------------------------------------------
// Returns the value of arg [index]
// -----------------------------------------------------------------------------
int ActionSpecialDialog::argValue(int index) const
{
	if (panel_args_)
		return panel_args_->argValue(index);
	else
		return 0;
}

// -----------------------------------------------------------------------------
// Applies selected trigger(s) (hexen or udmf) to [lines]
// -----------------------------------------------------------------------------
void ActionSpecialDialog::applyTo(vector<MapObject*>& lines, bool apply_special) const
{
	panel_special_->applyTo(lines, apply_special);
}

// -----------------------------------------------------------------------------
// Loads special/trigger/arg values from [lines]
// -----------------------------------------------------------------------------
void ActionSpecialDialog::openLines(vector<MapObject*>& lines) const
{
	panel_special_->openLines(lines);
}
