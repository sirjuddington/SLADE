
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "ActionSpecialDialog.h"
#include "Game/Configuration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/GenLineSpecialPanel.h"
#include "UI/NumberTextCtrl.h"
#include "SpecialPresetDialog.h"


// ----------------------------------------------------------------------------
//
// ActionSpecialtreeView Class Functions
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ActionSpecialTreeView::ActionSpecialTreeView
//
// ActionSpecialTreeView class constructor
// ----------------------------------------------------------------------------
ActionSpecialTreeView::ActionSpecialTreeView(wxWindow* parent) : wxDataViewTreeCtrl(parent, -1)
{
	parent_dialog = nullptr;

	// Create root item
	root = wxDataViewItem(nullptr);

	// Add 'None'
	item_none = AppendItem(root, "0: None");

	// Computing the minimum width of the tree is slightly complicated, since
	// wx doesn't expose it to us directly
	wxClientDC dc(this);
	dc.SetFont(GetFont());
	wxSize textsize;

	// Populate tree
	for (auto& i : Game::configuration().allActionSpecials())
	{
		if (!i.second.defined())
			continue;

		string label = S_FMT("%d: %s", i.second.number(), i.second.name());
		AppendItem(getGroup(i.second.group()), label);
		textsize.IncTo(dc.GetTextExtent(label));
	}
	Expand(root);

	// Bind events
	Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &ActionSpecialTreeView::onItemEdit, this);
	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ActionSpecialTreeView::onItemActivated, this);

	// 64 is an arbitrary fudge factor -- should be at least the width of a
	// scrollbar plus the expand icons plus any extra padding
	int min_width = textsize.GetWidth() + GetIndent() + 64;
	SetMinSize(wxSize(min_width, 200));
}

// ----------------------------------------------------------------------------
// ActionSpecialTreeView::~ActionSpecialTreeView
//
// ActionSpecialTreeView class destructor
// ----------------------------------------------------------------------------
ActionSpecialTreeView::~ActionSpecialTreeView()
{
}

// ----------------------------------------------------------------------------
// ActionSpecialTreeView::specialNumber
//
// Returns the action special value for [item]
// ----------------------------------------------------------------------------
int ActionSpecialTreeView::specialNumber(wxDataViewItem item)
{
	string num = GetItemText(item).BeforeFirst(':');
	long s;
	if (!num.ToLong(&s))
		s = -1;

	return s;
}

// ----------------------------------------------------------------------------
// ActionSpecialTreeView::showSpecial
//
// Finds the item for [special], selects it and ensures it is shown
// ----------------------------------------------------------------------------
void ActionSpecialTreeView::showSpecial(int special, bool focus)
{
	if (special == 0)
	{
		EnsureVisible(item_none);
		Select(item_none);
		if (focus)
			SetFocus();
		return;
	}

	// Go through item groups
	for (unsigned a = 0; a < groups.size(); a++)
	{
		// Go through group items
		for (int b = 0; b < GetChildCount(groups[a].item); b++)
		{
			wxDataViewItem item = GetNthChild(groups[a].item, b);

			// Select+show if match
			if (specialNumber(item) == special)
			{
				EnsureVisible(item);
				Select(item);
				if (focus)
					SetFocus();
				return;
			}
		}
	}
}

// ----------------------------------------------------------------------------
// ActionSpecialTreeView::selectedSpecial
//
// Returns the currently selected action special value
// ----------------------------------------------------------------------------
int ActionSpecialTreeView::selectedSpecial()
{
	wxDataViewItem item = GetSelection();
	if (item.IsOk())
		return specialNumber(item);
	else
		return -1;
}

// ----------------------------------------------------------------------------
// ActionSpecialTreeView::getGroup
//
// Returns the parent wxDataViewItem representing action special group [group]
// ----------------------------------------------------------------------------
wxDataViewItem ActionSpecialTreeView::getGroup(string group)
{
	// Check if group was already made
	for (unsigned a = 0; a < groups.size(); a++)
	{
		if (group == groups[a].name)
			return groups[a].item;
	}

	// Split group into subgroups
	wxArrayString path = wxSplit(group, '/');

	// Create group needed
	wxDataViewItem current = root;
	string fullpath = "";
	for (unsigned p = 0; p < path.size(); p++)
	{
		if (p > 0) fullpath += "/";
		fullpath += path[p];

		bool found = false;
		for (unsigned a = 0; a < groups.size(); a++)
		{
			if (groups[a].name == fullpath)
			{
				current = groups[a].item;
				found = true;
				break;
			}
		}

		if (!found)
		{
			current = AppendContainer(current, path[p], -1, 1);
			groups.push_back(astv_group_t(current, fullpath));
		}
	}

	return current;
}


// ----------------------------------------------------------------------------
//
// ActionSpecialTreeView Class Events
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ActionSpecialTreeView::onItemEdit
//
// Called when a tree item label is edited
// ----------------------------------------------------------------------------
void ActionSpecialTreeView::onItemEdit(wxDataViewEvent& e)
{
	e.Veto();
}

// ----------------------------------------------------------------------------
// ActionSpecialTreeView::onItemActivated
//
// Called when a tree item is activated
// ----------------------------------------------------------------------------
void ActionSpecialTreeView::onItemActivated(wxDataViewEvent& e)
{
	if (parent_dialog)
		parent_dialog->EndModal(wxID_OK);
}


// ----------------------------------------------------------------------------
// ArgsControl Class
//
// Helper class that contains controls specific to a particular argument.
// Usually this is a text box, but some args take one of a list of choices,
// flags, etc.
// ----------------------------------------------------------------------------
class ArgsControl : public wxPanel
{
protected:
	// Original arg configuration
	Game::Arg	arg;

public:
	ArgsControl(wxWindow* parent, const Game::Arg& arg) : wxPanel(parent, -1), arg(arg)
	{
		SetSizer(new wxBoxSizer(wxVERTICAL));
	}
	~ArgsControl() {}

	virtual long getArgValue() = 0;
	virtual void setArgValue(long val) = 0;
};

// ----------------------------------------------------------------------------
// ArgsTextControl Class
//
// Trivial case of an arg control: a text box that can hold a number
// from 0 to 255.
// ----------------------------------------------------------------------------
class ArgsTextControl : public ArgsControl
{
protected:
	// This is the control holding the "real" value
	wxTextCtrl* text_control;

public:
	ArgsTextControl(wxWindow* parent, const Game::Arg& arg, bool limit_byte) : ArgsControl(parent, arg)
	{
		text_control = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(40, -1));
		if (limit_byte)
			text_control->SetValidator(wxIntegerValidator<unsigned char>());
		else
			text_control->SetValidator(wxIntegerValidator<int>());
		GetSizer()->Add(text_control, wxSizerFlags().Expand());
	}

	// Get the value of the argument from the textbox
	long getArgValue()
	{
		wxString val = text_control->GetValue();

		// Empty string means ignore it
		if (val == "")
			return -1;

		long ret;
		val.ToLong(&ret);
		return ret;
	}

	// Set the value in the textbox
	void setArgValue(long val)
	{
		if (val < 0)
			text_control->ChangeValue("");
		else
			text_control->ChangeValue(S_FMT("%ld", val));
	}
};

// ----------------------------------------------------------------------------
// Helper for the combo box.  wxIntegerValidator, by default, will erase the
// entire combo box if one of the labeled numbers is selected, because the
// label isn't a valid number.
// ----------------------------------------------------------------------------
template<typename T>
class ComboBoxAwareIntegerValidator : public wxIntegerValidator<T>
{
public:
	ComboBoxAwareIntegerValidator() : wxIntegerValidator<T>() {}
	ComboBoxAwareIntegerValidator(const ComboBoxAwareIntegerValidator& validator)
		: wxIntegerValidator<T>(validator) {}

	virtual wxObject* Clone() const { return new ComboBoxAwareIntegerValidator(*this); }

	virtual wxString NormalizeString(const wxString& s) const
	{
		// If there's a valid selection in the combobox, don't "normalize".
		// This is a highly inappropriate place for this check, but everything
		// else is private and non-virtual.
		wxComboBox* control = static_cast<wxComboBox*>(wxNumValidatorBase::GetTextEntry());
		if (control->GetSelection() != wxNOT_FOUND)
			return s;

		return wxIntegerValidator<T>::NormalizeString(s);
	}
};

// ----------------------------------------------------------------------------
// ArgsChoiceControl Class
//
// Combo box for an argument that takes one of a set of predefined values.
// ----------------------------------------------------------------------------
class ArgsChoiceControl : public ArgsControl
{
protected:
	wxComboBox*			choice_control;

public:
	ArgsChoiceControl(wxWindow* parent, const Game::Arg& arg)
		: ArgsControl(parent, arg)
	{
		choice_control = new wxComboBox(this, -1, "", wxDefaultPosition, wxSize(100, -1));
		choice_control->SetValidator(ComboBoxAwareIntegerValidator<unsigned char>());

		for (unsigned i = 0; i < arg.custom_values.size(); i++)
		{
			choice_control->Append(
				S_FMT("%d: %s", arg.custom_values[i].value, arg.custom_values[i].name));
		}

		GetSizer()->Add(choice_control, wxSizerFlags().Expand());
		Fit();
	}

	long getArgValue()
	{
		int selected = choice_control->GetSelection();
		if (selected == wxNOT_FOUND)
		{
			// No match.  User must have entered a value themselves
			string val = choice_control->GetValue();

			// Empty string means ignore it
			if (val == "")
				return -1;

			long ret;
			val.ToLong(&ret);
			return ret;

		}
		else
		{
			return arg.custom_values[selected].value;
		}
	}

	void setArgValue(long val)
	{
		if (val < 0)
		{
			choice_control->ChangeValue("");
			return;
		}

		// Look for a name for this value
		for (unsigned i = 0; i < arg.custom_values.size(); i++)
		{
			if (val == arg.custom_values[i].value)
			{
				choice_control->SetSelection(i);
				return;
			}
		}
		choice_control->ChangeValue(S_FMT("%ld", val));
	}
};

// ----------------------------------------------------------------------------
// ArgsFlagsControl Class
//
// Set of checkboxes, for an argument that contains flags.
// ----------------------------------------------------------------------------
class ArgsFlagsControl : public ArgsTextControl
{
private:
	// Vector of bitmasks for the groups each flag belongs to, or 0 for an
	// independent flag, parallel to the arg's custom_flags
	vector<int>			flag_to_bit_group;
	// Parallel vector of the checkboxes and radio buttons we create
	vector<wxControl*>	controls;

	bool isPowerOfTwo(long n) { return (n & (n - 1)) == 0; }

	// ------------------------------------------------------------------------
	// ArgsFlagsControl::addControl
	//
	// Add a checkbox or radio button to the sizer, and perform some
	// bookkeeping.
	// ------------------------------------------------------------------------
	void addControl(wxControl* control, int index, int group)
	{
		GetSizer()->Add(control);
		controls[index] = control;
		flag_to_bit_group[index] = group;
		control->Bind(wxEVT_CHECKBOX, &ArgsFlagsControl::onCheck, this);
		control->Bind(wxEVT_RADIOBUTTON, &ArgsFlagsControl::onCheck, this);
	}

	// ------------------------------------------------------------------------
	// ArgsFlagsControl::onCheck
	//
	// Event handler called when a checkbox or radio button is toggled.
	// Update the value in the textbox.
	// ------------------------------------------------------------------------
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
		wxObject* control = event.GetEventObject();
		for (unsigned i = 0; i < arg.custom_flags.size(); i++)
		{
			if (controls[i] == control)
			{
				// Remove the entire group
				if (flag_to_bit_group[i])
					val &= ~flag_to_bit_group[i];
				else
					val &= ~arg.custom_flags[i].value;

				// Then re-add if appropriate
				if (event.IsChecked())
					val |= arg.custom_flags[i].value;
				ArgsTextControl::setArgValue(val);
				return;
			}
		}
	}

	// ------------------------------------------------------------------------
	// ArgsFlagsControl::onKeypress
	//
	// Event handler called when a key is pressed in the textbox.
	// Refresh all the flag states.
	// ------------------------------------------------------------------------
	void onKeypress(wxKeyEvent& event)
	{
		event.Skip();
		updateCheckState(getArgValue());
	}

	// ------------------------------------------------------------------------
	// ArgsFlagsControl::updateCheckState
	//
	// Do the actual work of updating the checkbox states.
	// ------------------------------------------------------------------------
	void updateCheckState(long val)
	{
		for (unsigned i = 0; i < arg.custom_flags.size(); i++)
		{
			if (flag_to_bit_group[i])
			{
				bool checked = (val >= 0 && (val & flag_to_bit_group[i]) == arg.custom_flags[i].value);
				static_cast<wxRadioButton*>(controls[i])->SetValue(checked);
			}
			else
			{
				bool checked = (val >= 0 && (val & arg.custom_flags[i].value) == arg.custom_flags[i].value);
				static_cast<wxCheckBox*>(controls[i])->SetValue(checked);
			}
		}
	}

public:
	ArgsFlagsControl(wxWindow* parent, const Game::Arg& arg, bool limit_byte)
		: ArgsTextControl(parent, arg, limit_byte),
		flag_to_bit_group(arg.custom_flags.size(), 0),
		controls(arg.custom_flags.size(), nullptr)
	{
		text_control->Bind(wxEVT_KEY_UP, &ArgsFlagsControl::onKeypress, this);

		wxControl* control;
		wxSizer* sizer = GetSizer();

		// Sometimes multiple bits are used for a set of more than two flags.
		// For example, if 3 is a flag, then it must be one of /four/ flags
		// along with values 0, 1, and 2.  In such cases, we need radio buttons
		// instead of a checkbox.
		// This is not as robust as it could be, but to my knowledge, the only
		// place this gets used is the "type" argument to ZDoom's
		// Sector_Set3DFloor, where the first two bits are an enum.
		vector<int> bit_groups;
		for (unsigned i = 0; i < arg.custom_flags.size(); i++)
		{
			int value = arg.custom_flags[i].value;
			if (isPowerOfTwo(value))
				continue;

			bool found_match = false;
			for (unsigned j = 0; j < bit_groups.size(); j++)
			{
				if (bit_groups[j] & value) {
					bit_groups[j] |= value;
					found_match = true;
					break;
				}
			}
			if (!found_match)
				bit_groups.push_back(value);
		}

		vector<bool> flag_done(arg.custom_flags.size(), false);
		for (unsigned i = 0; i < arg.custom_flags.size(); i++)
		{
			if (flag_done[i])
				continue;

			// Check if this flag is part of a group
			int group = 0;
			int check_against = arg.custom_flags[i].value;
			// Special case: if the value is 0, it has no bits, so assume it's
			// part of the next flag's group
			if (arg.custom_flags[i].value == 0 && i < arg.custom_flags.size() - 1)
				check_against = arg.custom_flags[i + 1].value;
			for (unsigned j = 0; j < bit_groups.size(); j++)
				if (bit_groups[j] & check_against) {
					group = bit_groups[j];
					break;
				}

			if (group)
			{
				addControl(
					new wxRadioButton(this, -1, S_FMT("%d: %s", arg.custom_flags[i].value, arg.custom_flags[i].name),
						wxDefaultPosition, wxDefaultSize, wxRB_GROUP),
					i, group);
				// Find all the other (later) flags that are part of this same bit group
				for (unsigned ii = i + 1; ii < arg.custom_flags.size(); ii++)
				{
					if (flag_done[ii])
						continue;
					if (arg.custom_flags[ii].value & group)
					{
						addControl(
							new wxRadioButton(this, -1, S_FMT("%d: %s", arg.custom_flags[ii].value, arg.custom_flags[ii].name)),
							ii, group);
						flag_done[ii] = true;
					}
				}
			}
			else  // not in a group
			{
				control = new wxCheckBox(this, -1, S_FMT("%d: %s", arg.custom_flags[i].value, arg.custom_flags[i].name));
				addControl(control, i, 0);
			}
		}

		Fit();
	}

	void setArgValue(long val)
	{
		ArgsTextControl::setArgValue(val);
		updateCheckState(val);
	}
};

// ----------------------------------------------------------------------------
// ArgsSpeedControl Class
//
// Arg control that shows a slider for selecting a flat movement speed.
// ----------------------------------------------------------------------------
class ArgsSpeedControl : public ArgsChoiceControl
{
protected:
	wxSlider* slider_control;
	wxStaticText* speed_label;

	void onSlide(wxCommandEvent& event)
	{
		syncControls(slider_control->GetValue());
	}

	void syncControls(int value)
	{
		ArgsChoiceControl::setArgValue(value);

		if (value < 0)
		{
			slider_control->SetValue(0);
			speed_label->SetLabel("");
		}
		else
		{
			slider_control->SetValue(value);
			speed_label->SetLabel(S_FMT(
				"%s (%.1f units per tic, %.1f units per sec)",
				arg.speedLabel(value), value / 8.0,
				// A tic is 28ms, slightly less than 1/35 of a second
				value / 8.0 * 1000.0 / 28.0
			));
		}
	}

public:
	ArgsSpeedControl(wxWindow* parent, const Game::Arg& arg)
		: ArgsChoiceControl(parent, arg)
	{
		wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);

		slider_control = new wxSlider(this, -1, 0, 0, 255);
		slider_control->SetLineSize(2);
		slider_control->SetPageSize(8);
		// Add a tic for every predefined value
		for (unsigned a = 0; a < arg.custom_flags.size(); a++)
			slider_control->SetTick(arg.custom_flags[a].value);
		slider_control->Bind(wxEVT_SLIDER, &ArgsSpeedControl::onSlide, this);
		speed_label = new wxStaticText(this, -1, "");

		GetSizer()->Detach(choice_control);
		row->Add(choice_control, wxSizerFlags(0).Expand());
		row->AddSpacer(4);
		row->Add(slider_control, wxSizerFlags(1).Align(wxALIGN_CENTER_VERTICAL));
		GetSizer()->Add(row, wxSizerFlags(1).Expand());
		GetSizer()->Add(speed_label, wxSizerFlags(1).Expand());

		// The label has its longest value at 0, which makes for an appropriate
		// minimum size
		syncControls(0);

		Fit();
	}

	// Set the value in the textbox
	void setArgValue(long val)
	{
		syncControls(val);
	}
};


// ----------------------------------------------------------------------------
//
// ArgsPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ArgsPanel::ArgsPanel
//
// ArgsPanel class constructor
// ----------------------------------------------------------------------------
ArgsPanel::ArgsPanel(wxWindow* parent)
: wxScrolled<wxPanel>(parent, -1, wxDefaultPosition, wxDefaultSize, wxVSCROLL)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add arg controls
	fg_sizer = new wxFlexGridSizer(2, 4, 4);
	fg_sizer->AddGrowableCol(1);
	sizer->Add(fg_sizer, 1, wxEXPAND|wxALL, 4);

	for (unsigned a = 0; a < 5; a++)
	{
		label_args[a] = new wxStaticText(this, -1, "");
		control_args[a] = nullptr;
		label_args_desc[a] = new wxStaticText(this, -1, "", wxDefaultPosition, wxSize(100, -1));
	}

	// Set up vertical scrollbar
	SetScrollRate(0, 10);

	Bind(wxEVT_SIZE, &ArgsPanel::onSize, this);
}

// ----------------------------------------------------------------------------
// ArgsPanel::setup
//
// Sets up the arg names and descriptions from specification in [args]
// ----------------------------------------------------------------------------
void ArgsPanel::setup(const Game::ArgSpec& args, bool udmf)
{
	using Game::Arg;

	// Reset stuff (but preserve the values)
	int old_values[5];
	fg_sizer->Clear();
	for (unsigned a = 0; a < 5; a++)
	{
		if (control_args[a])
		{
			old_values[a] = control_args[a]->getArgValue();
			control_args[a]->Destroy();
		}
		else
			old_values[a] = -1;

		control_args[a] = nullptr;
		label_args[a]->SetLabelText(S_FMT("Arg %d:", a + 1));
		label_args_desc[a]->Show(false);
	}

	// Setup layout
	int row = 0;
	for (unsigned a = 0; a < 5; a++)
	{
		auto& arg = args[a];
		bool has_desc = false;
		
		if ((int)a < args.count) {
			has_desc = !arg.desc.IsEmpty();

			if (arg.type == Arg::Type::Choice)
				control_args[a] = new ArgsChoiceControl(this, arg);
			else if (arg.type == Arg::Type::Flags)
				control_args[a] = new ArgsFlagsControl(this, arg, !udmf);
			else if (arg.type == Arg::Type::Speed)
				control_args[a] = new ArgsSpeedControl(this, arg);
			else
				control_args[a] = new ArgsTextControl(this, arg, !udmf);
		}
		else {
			control_args[a] = new ArgsTextControl(this, arg, !udmf);
		}

		// Arg name
		label_args[a]->SetLabelText(S_FMT("%s:", arg.name));
		fg_sizer->Add(label_args[a], wxSizerFlags().Align(wxALIGN_TOP|wxALIGN_RIGHT).Border(wxALL, 4));

		// Arg value
		control_args[a]->setArgValue(old_values[a]);
		fg_sizer->Add(control_args[a], wxSizerFlags().Expand());
		
		// Arg description
		if (has_desc)
		{
			// Add an empty spacer to the first column
			fg_sizer->Add(0, 0);
			fg_sizer->Add(label_args_desc[a], wxSizerFlags().Expand());
		}
	}

	// We may have changed the minimum size of the window by adding new big
	// controls, so we need to ask the top-level parent to recompute its
	// minimum size
	wxWindow* toplevel = this;
	while (!toplevel->IsTopLevel() && toplevel->GetParent())
		toplevel = toplevel->GetParent();
	wxSizer* toplevel_sizer = toplevel->GetSizer();
	if (toplevel_sizer)
	{
		// This is more or less what SetSizerAndFit does, but without resizing
		// the window if not necessary
		toplevel->SetMinClientSize(
			toplevel_sizer->ComputeFittingClientSize(toplevel));
		wxSize toplevel_size = toplevel->GetSize();
		wxSize toplevel_best = toplevel_size;
		toplevel_best.IncTo(toplevel->GetBestSize());
		if (toplevel_best != toplevel_size)
			toplevel->SetSize(toplevel_best);
	}

	// Set the label text last, so very long labels will wrap naturally and not
	// force the window to be ridiculously wide
	Layout();
	int available_width = fg_sizer->GetColWidths()[1];
	for (int a = 0; a < args.count; a++)
	{
		auto& arg = args[a];

		if (!arg.desc.IsEmpty())
		{
			label_args_desc[a]->Show(true);
			label_args_desc[a]->SetLabelText(arg.desc);
			label_args_desc[a]->Wrap(available_width);
		}
	}

	FitInside();  // for wxScrolled's benefit
}

// ----------------------------------------------------------------------------
// ArgsPanel::setValues
//
// Sets the arg values
// ----------------------------------------------------------------------------
void ArgsPanel::setValues(int args[5])
{
	for (unsigned a = 0; a < 5; a++)
	{
		control_args[a]->setArgValue(args[a]);
	}
}

// ----------------------------------------------------------------------------
// ArgsPanel::getArgValue
//
// Returns the current value for arg [index]
// ----------------------------------------------------------------------------
int ArgsPanel::getArgValue(int index)
{
	// Check index
	if (index < 0 || index > 4 || !control_args[index])
		return -1;

	return control_args[index]->getArgValue();
}

// ----------------------------------------------------------------------------
// ArgsPanel::onSize
//
// Rewrap the descriptions when the panel is resized
// ----------------------------------------------------------------------------
void ArgsPanel::onSize(wxSizeEvent& event)
{
	event.Skip();

	Layout();
	if (fg_sizer->GetColWidths().size() > 1)
	{
		int available_width = fg_sizer->GetColWidths()[1];
		for (int a = 0; a < 5; a++)
		{
			// Wrap() puts hard newlines in the label, so we need to remove them
			wxString label = label_args_desc[a]->GetLabelText();
			label.Replace("\n", " ");
			label_args_desc[a]->SetLabelText(label);
			label_args_desc[a]->Wrap(available_width);
		}
	}
}


// ----------------------------------------------------------------------------
//
// ActionSpecialPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ActionSpecialPanel::ActionSpecialPanel
//
// ActionSpecialPanel class constructor
// ----------------------------------------------------------------------------
ActionSpecialPanel::ActionSpecialPanel(wxWindow* parent, bool trigger) : wxPanel(parent, -1)
{
	panel_args_ = nullptr;
	choice_trigger_ = nullptr;
	show_trigger_ = trigger;

	// Setup layout
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	if (Game::configuration().featureSupported(Game::Feature::Boom))
	{
		// Action Special radio button
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 4);
		rb_special_ = new wxRadioButton(this, -1, "Action Special", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		hbox->Add(rb_special_, 0, wxEXPAND|wxRIGHT, 8);

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
	sizer->Add(panel_action_special_, 1, wxEXPAND|wxALL, 4);

	SetSizerAndFit(sizer);

	// Bind events
	tree_specials_->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ActionSpecialPanel::onSpecialSelectionChanged, this);
	tree_specials_->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ActionSpecialPanel::onSpecialItemActivated, this);
}

// ----------------------------------------------------------------------------
// ActionSpecialPanel::~ActionSpecialPanel
//
// ActionSpecialPanel class destructor
// ----------------------------------------------------------------------------
ActionSpecialPanel::~ActionSpecialPanel()
{
}

// ----------------------------------------------------------------------------
// ActionSpecialPanel::setupSpecialPanel
//
// Creates and sets up the action special panel
// ----------------------------------------------------------------------------
void ActionSpecialPanel::setupSpecialPanel()
{
	// Create panel
	panel_action_special_ = new wxPanel(this, -1);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// Special box
	text_special_ = new NumberTextCtrl(panel_action_special_);
	sizer->Add(text_special_, 0, wxEXPAND|wxALL, 4);
	text_special_->Bind(wxEVT_TEXT, [&](wxCommandEvent &e)
	{
		tree_specials_->showSpecial(text_special_->getNumber(), false);
	});

	// Action specials tree
	tree_specials_ = new ActionSpecialTreeView(panel_action_special_);
	sizer->Add(tree_specials_, 1, wxEXPAND|wxALL, 4);

	if (show_trigger_)
	{
		// UDMF Triggers
		if (MapEditor::editContext().mapDesc().format == MAP_UDMF)
		{
			// Get all UDMF properties
			auto& props = Game::configuration().allUDMFProperties(MOBJ_LINE);

			// Get all UDMF trigger properties
			std::map<string, wxFlexGridSizer*> named_flexgrids;
			for (auto& i : props)
			{
				if (!i.second.isTrigger())
					continue;

				string group = i.second.group();
				wxFlexGridSizer* frame_sizer = named_flexgrids[group];
				if (!frame_sizer)
				{
					auto frame_triggers = new wxStaticBox(panel_action_special_, -1, group);
					auto sizer_triggers = new wxStaticBoxSizer(frame_triggers, wxVERTICAL);
					sizer->Add(sizer_triggers, 0, wxEXPAND|wxTOP, 4);

					frame_sizer = new wxFlexGridSizer(3);
					frame_sizer->AddGrowableCol(0, 1);
					frame_sizer->AddGrowableCol(1, 1);
					frame_sizer->AddGrowableCol(2, 1);
					sizer_triggers->Add(frame_sizer, 1, wxEXPAND|wxALL, 4);

					named_flexgrids.find(group)->second = frame_sizer;
				}

				auto cb_trigger = new wxCheckBox(
					panel_action_special_,
					-1,
					i.second.name(),
					wxDefaultPosition,
					wxDefaultSize,
					wxCHK_3STATE
				);
				frame_sizer->Add(cb_trigger, 0, wxEXPAND);

				flags_.push_back({ cb_trigger, -1, i.second.propName() });
			}
		}

		// Hexen trigger
		else if (MapEditor::editContext().mapDesc().format == MAP_HEXEN)
		{
			auto frame_trigger = new wxStaticBox(panel_action_special_, -1, "Special Trigger");
			auto sizer_trigger = new wxStaticBoxSizer(frame_trigger, wxVERTICAL);
			sizer->Add(sizer_trigger, 0, wxEXPAND | wxALL, 4);

			// Add triggers dropdown
			choice_trigger_ = new wxChoice(
				panel_action_special_,
				-1,
				wxDefaultPosition,
				wxDefaultSize,
				Game::configuration().allSpacTriggers()
			);
			sizer_trigger->Add(choice_trigger_, 0, wxEXPAND | wxALL, 4);

			// Add activation-related flags
			auto fg_sizer = new wxFlexGridSizer(3);
			fg_sizer->AddGrowableCol(0, 1);
			fg_sizer->AddGrowableCol(1, 1);
			fg_sizer->AddGrowableCol(2, 1);
			sizer_trigger->Add(fg_sizer, 0, wxEXPAND | wxALL, 4);
			for (unsigned a = 0; a < Game::configuration().nLineFlags(); a++)
			{
				if (Game::configuration().lineFlag(a).activation)
				{
					flags_.push_back({
						new wxCheckBox(panel_action_special_, -1, Game::configuration().lineFlag(a).name),
						(int)a,
						Game::configuration().lineFlag(a).udmf
					});
					fg_sizer->Add(flags_.back().check_box, 0, wxEXPAND);
				}
			}
		}

		// Preset button
		btn_preset_ = new wxButton(panel_action_special_, -1, "Preset...");
		sizer->Add(btn_preset_, 0, wxALIGN_RIGHT | wxALL, 4);
		btn_preset_->Bind(wxEVT_BUTTON, &ActionSpecialPanel::onSpecialPresetClicked, this);
	}

	panel_action_special_->SetSizerAndFit(sizer);
}

// ----------------------------------------------------------------------------
// ActionSpecialPanel::setSpecial
//
// Selects the item for special [special] in the specials tree
// ----------------------------------------------------------------------------
void ActionSpecialPanel::setSpecial(int special)
{
	// Check for boom generalised special
	if (Game::configuration().featureSupported(Game::Feature::Boom))
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
	text_special_->SetValue(S_FMT("%d", special));

	// Setup args if any
	if (panel_args_)
	{
		auto& args = Game::configuration().actionSpecial(selectedSpecial()).argSpec();
		panel_args_->setup(args, (MapEditor::editContext().mapDesc().format == MAP_UDMF));
	}
}

// ----------------------------------------------------------------------------
// ActionSpecialPanel::setTrigger
//
// Sets the action special trigger (hexen or udmf)
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// ActionSpecialPanel::setTrigger
//
// Sets the action special trigger from a udmf trigger name (hexen or udmf)
// ----------------------------------------------------------------------------
void ActionSpecialPanel::setTrigger(string trigger)
{
	if (!show_trigger_)
		return;

	// Hexen trigger
	if (choice_trigger_)
	{
		for (unsigned a = 0; a < choice_trigger_->GetCount(); a++)
			if (Game::configuration().spacTriggerUDMFName(a) == trigger)
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

// ----------------------------------------------------------------------------
// ActionSpecialPanel::clearTrigger
//
// Deselects all triggers (or resets to 'player cross' in hexen format)
// ----------------------------------------------------------------------------
void ActionSpecialPanel::clearTrigger()
{
	// UDMF Triggers and Flags
	for (auto& flag : flags_)
		flag.check_box->SetValue(false);

	// Hexen trigger
	if (choice_trigger_)
		choice_trigger_->SetSelection(0);
}

// ----------------------------------------------------------------------------
// ActionSpecialPanel::selectedSpecial
//
// Returns the currently selected action special
// ----------------------------------------------------------------------------
int ActionSpecialPanel::selectedSpecial()
{
	if (Game::configuration().featureSupported(Game::Feature::Boom))
	{
		if (rb_special_->GetValue())
			return tree_specials_->selectedSpecial();
		else
			return panel_gen_specials_->getSpecial();
	}
	else
		return tree_specials_->selectedSpecial();
}

// ----------------------------------------------------------------------------
// ActionSpecialPanel::showGeneralised
//
// If [show] is true, show the generalised special panel, otherwise show the
// action special tree
// ----------------------------------------------------------------------------
void ActionSpecialPanel::showGeneralised(bool show)
{
	if (!Game::configuration().featureSupported(Game::Feature::Boom))
		return;

	if (show)
	{
		wxSizer* sizer = GetSizer();
		sizer->Replace(panel_action_special_, panel_gen_specials_);
		panel_action_special_->Show(false);
		panel_gen_specials_->Show(true);
		Layout();
	}
	else
	{
		wxSizer* sizer = GetSizer();
		sizer->Replace(panel_gen_specials_, panel_action_special_);
		panel_action_special_->Show(true);
		panel_gen_specials_->Show(false);
		Layout();
	}
}

// ----------------------------------------------------------------------------
// ActionSpecialPanel::applyTo
//
// Applies selected special (if [apply_special] is true), trigger(s) and args
// (if any) to [lines]
// ----------------------------------------------------------------------------
void ActionSpecialPanel::applyTo(vector<MapObject*>& lines, bool apply_special)
{
	// Special
	int special = selectedSpecial();
	if (apply_special && special >= 0)
	{
		for (unsigned a = 0; a < lines.size(); a++)
			lines[a]->setIntProperty("special", special);
	}

	// Args
	if (panel_args_)
	{
		// Get values
		int args[5];
		args[0] = panel_args_->getArgValue(0);
		args[1] = panel_args_->getArgValue(1);
		args[2] = panel_args_->getArgValue(2);
		args[3] = panel_args_->getArgValue(3);
		args[4] = panel_args_->getArgValue(4);

		for (unsigned a = 0; a < lines.size(); a++)
		{
			if (args[0] >= 0) lines[a]->setIntProperty("arg0", args[0]);
			if (args[1] >= 0) lines[a]->setIntProperty("arg1", args[1]);
			if (args[2] >= 0) lines[a]->setIntProperty("arg2", args[2]);
			if (args[3] >= 0) lines[a]->setIntProperty("arg3", args[3]);
			if (args[4] >= 0) lines[a]->setIntProperty("arg4", args[4]);
		}
	}

	// Trigger(s)
	if (show_trigger_)
	{
		for (unsigned a = 0; a < lines.size(); a++)
		{
			// Hexen
			if (choice_trigger_)
				Game::configuration().setLineSpacTrigger(choice_trigger_->GetSelection(), (MapLine*)lines[a]);

			// UDMF / Flags
			for (auto& flag : flags_)
			{
				if (flag.check_box->Get3StateValue() == wxCHK_UNDETERMINED)
					continue;

				if (choice_trigger_)
					Game::configuration().setLineFlag(flag.index, (MapLine*)lines[a], flag.check_box->GetValue());
				else
					lines[a]->setBoolProperty(flag.udmf, flag.check_box->GetValue());
			}
		}
	}
}

// ----------------------------------------------------------------------------
// ActionSpecialPanel::openLines
//
// Loads special/trigger/arg values from [lines]
// ----------------------------------------------------------------------------
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
			int trigger = Game::configuration().spacTriggerIndexHexen((MapLine*)lines[0]);
			for (unsigned a = 1; a < lines.size(); a++)
			{
				if (trigger != Game::configuration().spacTriggerIndexHexen((MapLine*)lines[a]))
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
				flag.check_box->SetValue(Game::configuration().lineFlagSet(flag.index, (MapLine*)lines[0]));

				// Go through subsequent lines
				for (unsigned b = 1; b < lines.size(); b++)
				{
					// Check for mismatch			
					if (flag.check_box->GetValue() !=
						Game::configuration().lineFlagSet(flag.index, (MapLine*)lines[b]))
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
				if (MapObject::multiBoolProperty(lines, flag.udmf, set))
					flag.check_box->SetValue(set);
				else
					flag.check_box->Set3StateValue(wxCHK_UNDETERMINED);
			}
		}
	}
}


// ----------------------------------------------------------------------------
//
// ActionSpecialPanel Class Events
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ActionSpecialPanel::onRadioButtonChanged
//
// Called when the radio button selection is changed
// ----------------------------------------------------------------------------
void ActionSpecialPanel::onRadioButtonChanged(wxCommandEvent& e)
{
	// Swap panels
	showGeneralised(rb_generalised_->GetValue());
}

// ----------------------------------------------------------------------------
// ActionSpecialPanel::onSpecialSelectionChanged
//
// Called when the action special selection is changed
// ----------------------------------------------------------------------------
void ActionSpecialPanel::onSpecialSelectionChanged(wxDataViewEvent &e)
{
	if ((Game::configuration().featureSupported(Game::Feature::Boom) && rb_generalised_->GetValue()) ||
		selectedSpecial() < 0)
	{
		e.Skip();
		return;
	}

	// Set special # text box
	text_special_->SetValue(S_FMT("%d", selectedSpecial()));

	if (panel_args_)
	{
		auto& args = Game::configuration().actionSpecial(selectedSpecial()).argSpec();
		panel_args_->setup(args, (MapEditor::editContext().mapDesc().format == MAP_UDMF));
	}
}

// ----------------------------------------------------------------------------
// ActionSpecialPanel::onSpecialItemActivated
//
// Called when the action special item is activated
// (double-clicked or enter pressed)
// ----------------------------------------------------------------------------
void ActionSpecialPanel::onSpecialItemActivated(wxDataViewEvent &e)
{
	if (tree_specials_->GetChildCount(e.GetItem()) > 0)
	{
		tree_specials_->Expand(e.GetItem());
		e.Skip();
		return;
	}

	// Jump to args tab, if there is one
	if (panel_args_)
	{
		auto& args = Game::configuration().actionSpecial(selectedSpecial()).argSpec();
		panel_args_->setup(args, (MapEditor::editContext().mapDesc().format == MAP_UDMF));
		panel_args_->SetFocus();
	}
}

// ----------------------------------------------------------------------------
// ActionSpecialPanel::onSpecialPresetClicked
//
// Called when the special preset button is clicked
// ----------------------------------------------------------------------------
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


// ----------------------------------------------------------------------------
//
// ActionSpecialDialog Class Functions
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ActionSpecialDialog::ActionSpecialDialog
//
// ActionSpecialDialog class constructor
// ----------------------------------------------------------------------------
ActionSpecialDialog::ActionSpecialDialog(wxWindow* parent, bool show_args)
: SDialog(parent, "Select Action Special", "actionspecial", 400, 500)
{
	panel_args = nullptr;
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// No args
	if (MapEditor::editContext().mapDesc().format == MAP_DOOM || !show_args)
	{
		panel_special = new ActionSpecialPanel(this, false);
		sizer->Add(panel_special, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);
	}

	// Args (use tabs)
	else
	{
		stc_tabs = STabCtrl::createControl(this);
		sizer->Add(stc_tabs, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);

		// Special panel
		panel_special = new ActionSpecialPanel(stc_tabs);
		stc_tabs->AddPage(panel_special, "Special");

		// Args panel
		panel_args = new ArgsPanel(stc_tabs);
		stc_tabs->AddPage(panel_args, "Args");
		panel_special->setArgsPanel(panel_args);
	}

	// Add buttons
	sizer->AddSpacer(4);
	sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);

	// Init
	SetSizerAndFit(sizer);
	CenterOnParent();
}

// ----------------------------------------------------------------------------
// ActionSpecialDialog::~ActionSpecialDialog
//
// ActionSpecialDialog class constructor
// ----------------------------------------------------------------------------
ActionSpecialDialog::~ActionSpecialDialog()
{
}

// ----------------------------------------------------------------------------
// ActionSpecialDialog::setSpecial
//
// Selects the item for special [special] in the specials tree
// ----------------------------------------------------------------------------
void ActionSpecialDialog::setSpecial(int special)
{
	panel_special->setSpecial(special);
	if (panel_args)
	{
		auto& args = Game::configuration().actionSpecial(special).argSpec();
		panel_args->setup(args, (MapEditor::editContext().mapDesc().format == MAP_UDMF));
	}
}

// ----------------------------------------------------------------------------
// ActionSpecialDialog::setArgs
//
// Sets the arg values
// ----------------------------------------------------------------------------
void ActionSpecialDialog::setArgs(int args[5])
{
	if (panel_args)
		panel_args->setValues(args);
}

// ----------------------------------------------------------------------------
// ActionSpecialDialog::selectedSpecial
//
// Returns the currently selected action special
// ----------------------------------------------------------------------------
int ActionSpecialDialog::selectedSpecial()
{
	return panel_special->selectedSpecial();
}

// ----------------------------------------------------------------------------
// ActionSpecialDialog::getArg
//
// Returns the value of arg [index]
// ----------------------------------------------------------------------------
int ActionSpecialDialog::getArg(int index)
{
	if (panel_args)
		return panel_args->getArgValue(index);
	else
		return 0;
}

// ----------------------------------------------------------------------------
// ActionSpecialDialog::applyTriggers
//
// Applies selected trigger(s) (hexen or udmf) to [lines]
// ----------------------------------------------------------------------------
void ActionSpecialDialog::applyTo(vector<MapObject*>& lines, bool apply_special)
{
	panel_special->applyTo(lines, apply_special);
}

// ----------------------------------------------------------------------------
// ActionSpecialDialog::openLines
//
// Loads special/trigger/arg values from [lines]
// ----------------------------------------------------------------------------
void ActionSpecialDialog::openLines(vector<MapObject*>& lines)
{
	panel_special->openLines(lines);
}
