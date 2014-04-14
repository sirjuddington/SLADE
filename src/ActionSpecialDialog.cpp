
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ActionSpecialDialog.cpp
 * Description: A dialog that allows selection of an action special
 *              (and other related classes)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "WxStuff.h"
#include "ActionSpecialDialog.h"
#include "GameConfiguration.h"
#include "GenLineSpecialPanel.h"
#include "MapEditorWindow.h"
#include <wx/gbsizer.h>
#undef min
#undef max
#include <wx/valnum.h>


/*******************************************************************
 * ACTIONSPECIALTREEVIEW CLASS FUNCTIONS
 *******************************************************************/

/* ActionSpecialTreeView::ActionSpecialTreeView
 * ActionSpecialTreeView class constructor
 *******************************************************************/
ActionSpecialTreeView::ActionSpecialTreeView(wxWindow* parent) : wxDataViewTreeCtrl(parent, -1)
{
	parent_dialog = NULL;

	// Create root item
	root = wxDataViewItem(0);

	// Add 'None'
	AppendItem(root, "0: None");

	// Populate tree
	vector<as_t> specials = theGameConfiguration->allActionSpecials();
	std::sort(specials.begin(), specials.end());
	for (unsigned a = 0; a < specials.size(); a++)
	{
		AppendItem(getGroup(specials[a].special->getGroup()),
		           S_FMT("%d: %s", specials[a].number, specials[a].special->getName()), -1);
	}

	// Bind events
	Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &ActionSpecialTreeView::onItemEdit, this);
	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ActionSpecialTreeView::onItemActivated, this);

	Expand(root);
}

/* ActionSpecialTreeView::~ActionSpecialTreeView
 * ActionSpecialTreeView class destructor
 *******************************************************************/
ActionSpecialTreeView::~ActionSpecialTreeView()
{
}

/* ActionSpecialTreeView::specialNumber
 * Returns the action special value for [item]
 *******************************************************************/
int ActionSpecialTreeView::specialNumber(wxDataViewItem item)
{
	string num = GetItemText(item).BeforeFirst(':');
	long s;
	num.ToLong(&s);

	return s;
}

/* ActionSpecialTreeView::showSpecial
 * Finds the item for [special], selects it and ensures it is shown
 *******************************************************************/
void ActionSpecialTreeView::showSpecial(int special)
{
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
				SetFocus();
				return;
			}
		}
	}
}

/* ActionSpecialTreeView::selectedSpecial
 * Returns the currently selected action special value
 *******************************************************************/
int ActionSpecialTreeView::selectedSpecial()
{
	wxDataViewItem item = GetSelection();
	if (item.IsOk())
		return specialNumber(item);
	else
		return -1;
}

/* ActionSpecialTreeView::getGroup
 * Returns the parent wxDataViewItem representing action special
 * group [group]
 *******************************************************************/
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


/*******************************************************************
 * ACTIONSPECIALTREEVIEW CLASS EVENTS
 *******************************************************************/

/* ActionSpecialTreeView::onItemEdit
 * Called when a tree item label is edited
 *******************************************************************/
void ActionSpecialTreeView::onItemEdit(wxDataViewEvent& e)
{
	e.Veto();
}

/* ActionSpecialTreeView::onItemActivated
 * Called when a tree item is activated
 *******************************************************************/
void ActionSpecialTreeView::onItemActivated(wxDataViewEvent& e)
{
	if (parent_dialog)
		parent_dialog->EndModal(wxID_OK);
}


/*******************************************************************
 * ARGSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ArgsPanel::ArgsPanel
 * ArgsPanel class constructor
 *******************************************************************/
ArgsPanel::ArgsPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add arg controls
	gb_sizer = new wxGridBagSizer(4, 4);
	sizer->Add(gb_sizer, 1, wxEXPAND|wxALL, 4);

	for (unsigned a = 0; a < 5; a++)
	{
		label_args[a] = new wxStaticText(this, -1, "");
		text_args[a] = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(300, -1));
		text_args[a]->SetValidator(wxIntegerValidator<unsigned char>());
		label_args_desc[a] = new wxStaticText(this, -1, "", wxDefaultPosition, wxSize(300, -1));
	}
}

/* ArgsPanel::setup
 * Sets up the arg names and descriptions from specification in [args]
 *******************************************************************/
void ArgsPanel::setup(argspec_t* args)
{
	// Reset stuff
	gb_sizer->Clear();
	for (unsigned a = 0; a < 5; a++)
	{
		label_args[a]->SetLabel(S_FMT("Arg %d:", a + 1));
		label_args_desc[a]->Show(false);
	}

	// Setup layout
	int row = 0;
	for (unsigned a = 0; a < 5; a++)
	{
		bool has_desc = ((int)a < args->count && !args->getArg(a).desc.IsEmpty());

		// Arg name
		if (has_desc)
			gb_sizer->Add(label_args[a], wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		else
			gb_sizer->Add(label_args[a], wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 4);

		// Arg value
		if (has_desc)
			gb_sizer->Add(text_args[a], wxGBPosition(row++, 1), wxDefaultSpan, wxEXPAND);
		else
			gb_sizer->Add(text_args[a], wxGBPosition(row++, 1), wxDefaultSpan, wxEXPAND|wxBOTTOM, 4);
		
		// Arg description
		if (has_desc)
			gb_sizer->Add(label_args_desc[a], wxGBPosition(row++, 1), wxDefaultSpan, wxEXPAND|wxBOTTOM, 4);
	}

	// Setup controls
	for (int a = 0; a < args->count; a++)
	{
		arg_t& arg = args->getArg(a);

		label_args[a]->SetLabel(S_FMT("%s:", arg.name));
		if (!arg.desc.IsEmpty())
		{
			label_args_desc[a]->Show(true);
			label_args_desc[a]->SetLabel(arg.desc);
		}
	}

	gb_sizer->AddGrowableCol(1, 1);

	Layout();

	for (unsigned a = 0; a < 5; a++)
	{
		label_args_desc[a]->SetSize(text_args[a]->GetSize().GetWidth(), -1);
		label_args_desc[a]->Wrap(text_args[a]->GetSize().GetWidth());
	}

	Layout();
}

/* ArgsPanel::setValues
 * Sets the arg values
 *******************************************************************/
void ArgsPanel::setValues(int args[5])
{
	for (unsigned a = 0; a < 5; a++)
	{
		if (args[a] >= 0)
			text_args[a]->SetValue(S_FMT("%d", args[a]));
		else
			text_args[a]->SetValue("");
	}
}

/* ArgsPanel::getArgValue
 * Returns the current value for arg [index]
 *******************************************************************/
int ArgsPanel::getArgValue(int index)
{
	// Check index
	if (index < 0 || index > 4)
		return -1;

	// Check ignored
	if (text_args[index]->GetValue() == "")
		return -1;

	// Get value
	long val;
	text_args[index]->GetValue().ToLong(&val);

	return val;
}


/*******************************************************************
 * ACTIONSPECIALPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ActionSpecialPanel::ActionSpecialPanel
 * ActionSpecialPanel class constructor
 *******************************************************************/
ActionSpecialPanel::ActionSpecialPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	panel_args = NULL;
	choice_trigger = NULL;

	// Setup layout
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	if (theGameConfiguration->isBoom())
	{
		// Action Special radio button
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 4);
		rb_special = new wxRadioButton(this, -1, "Action Special", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		hbox->Add(rb_special, 0, wxEXPAND|wxRIGHT, 8);

		// Generalised Special radio button
		rb_generalised = new wxRadioButton(this, -1, "Generalised Special");
		hbox->Add(rb_generalised, 0, wxEXPAND);

		// Boom generalised line special panel
		panel_gen_specials = new GenLineSpecialPanel(this);
		panel_gen_specials->Show(false);

		// Bind events
		rb_special->Bind(wxEVT_RADIOBUTTON, &ActionSpecialPanel::onRadioButtonChanged, this);
		rb_generalised->Bind(wxEVT_RADIOBUTTON, &ActionSpecialPanel::onRadioButtonChanged, this);
	}

	// Action specials tree
	setupSpecialPanel();
	sizer->Add(panel_action_special, 1, wxEXPAND|wxALL, 4);

	// Bind events
	tree_specials->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ActionSpecialPanel::onSpecialSelectionChanged, this);
}

/* ActionSpecialPanel::~ActionSpecialPanel
 * ActionSpecialPanel class destructor
 *******************************************************************/
ActionSpecialPanel::~ActionSpecialPanel()
{
}

/* ActionSpecialPanel::setupSpecialPanel
 * Creates and sets up the action special panel
 *******************************************************************/
void ActionSpecialPanel::setupSpecialPanel()
{
	// Create panel
	panel_action_special = new wxPanel(this, -1);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel_action_special->SetSizer(sizer);

	// Action specials tree
	tree_specials = new ActionSpecialTreeView(panel_action_special);
	sizer->Add(tree_specials, 1, wxEXPAND|wxBOTTOM, 4);

	// UDMF Triggers
	if (theMapEditor->currentMapDesc().format == MAP_UDMF)
	{
		// Get all UDMF properties
		vector<udmfp_t> props = theGameConfiguration->allUDMFProperties(MOBJ_LINE);
		sort(props.begin(), props.end());

		// Get all UDMF trigger properties
		vector<string> triggers;
		for (unsigned a = 0; a < props.size(); a++)
		{
			if (props[a].property->isTrigger())
			{
				triggers.push_back(props[a].property->getName());
				triggers_udmf.push_back(props[a].property->getProperty());
			}
		}

		// Check if there are any triggers defined
		if (triggers.size() > 0)
		{
			// Add frame
			wxStaticBox* frame_triggers = new wxStaticBox(panel_action_special, -1, "Special Triggers");
			wxStaticBoxSizer* sizer_triggers = new wxStaticBoxSizer(frame_triggers, wxVERTICAL);
			sizer->Add(sizer_triggers, 0, wxEXPAND);

			// Add trigger checkboxes
			wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
			sizer_triggers->Add(gb_sizer, 1, wxEXPAND|wxALL, 4);
			int row = 0;
			int col = 0;
			int trigger_mid = triggers.size() / 3;
			for (unsigned a = 0; a < triggers.size(); a++)
			{
				wxCheckBox* cb_trigger = new wxCheckBox(panel_action_special, -1, triggers[a], wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
				gb_sizer->Add(cb_trigger, wxGBPosition(row++, col), wxDefaultSpan, wxEXPAND);
				cb_triggers.push_back(cb_trigger);

				if (row >= trigger_mid && col <= 1)
				{
					trigger_mid;
					row = 0;
					col++;
				}
			}

			gb_sizer->AddGrowableCol(0, 1);
			gb_sizer->AddGrowableCol(1, 1);
			gb_sizer->AddGrowableCol(2, 1);
		}
	}

	// Hexen trigger
	else if (theMapEditor->currentMapDesc().format == MAP_HEXEN)
	{
		// Add triggers dropdown
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND);

		hbox->Add(new wxStaticText(panel_action_special, -1, "Special Trigger:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		choice_trigger = new wxChoice(panel_action_special, -1, wxDefaultPosition, wxDefaultSize, theGameConfiguration->allSpacTriggers());
		hbox->Add(choice_trigger, 1, wxEXPAND);
	}
}

/* ActionSpecialPanel::setSpecial
 * Selects the item for special [special] in the specials tree
 *******************************************************************/
void ActionSpecialPanel::setSpecial(int special)
{
	// Check for boom generalised special
	if (theGameConfiguration->isBoom())
	{
		if (panel_gen_specials->loadSpecial(special))
		{
			rb_generalised->SetValue(true);
			showGeneralised(true);
			panel_gen_specials->SetFocus();
			return;
		}
		else
			rb_special->SetValue(true);
	}

	// Regular action special
	showGeneralised(false);
	tree_specials->showSpecial(special);
	tree_specials->SetFocus();
	tree_specials->SetFocusFromKbd();

	// Setup args if any
	if (panel_args)
	{
		argspec_t args = theGameConfiguration->actionSpecial(selectedSpecial())->getArgspec();
		panel_args->setup(&args);
	}
}

/* ActionSpecialPanel::setTrigger
 * Sets the action special trigger (hexen or udmf)
 *******************************************************************/
void ActionSpecialPanel::setTrigger(int index)
{
	// UDMF Trigger
	if (cb_triggers.size() > 0)
	{
		if (index < (int)cb_triggers.size())
			cb_triggers[index]->SetValue(true);
	}

	// Hexen trigger
	else
		choice_trigger->SetSelection(index);
}

/* ActionSpecialPanel::selectedSpecial
 * Returns the currently selected action special
 *******************************************************************/
int ActionSpecialPanel::selectedSpecial()
{
	if (theGameConfiguration->isBoom())
	{
		if (rb_special->GetValue())
			return tree_specials->selectedSpecial();
		else
			return panel_gen_specials->getSpecial();
	}
	else
		return tree_specials->selectedSpecial();
}

/* ActionSpecialPanel::showGeneralised
 * If [show] is true, show the generalised special panel, otherwise
 * show the action special tree
 *******************************************************************/
void ActionSpecialPanel::showGeneralised(bool show)
{
	if (!theGameConfiguration->isBoom())
		return;

	if (show)
	{
		wxSizer* sizer = GetSizer();
		sizer->Replace(panel_action_special, panel_gen_specials);
		panel_action_special->Show(false);
		panel_gen_specials->Show(true);
		Layout();
	}
	else
	{
		wxSizer* sizer = GetSizer();
		sizer->Replace(panel_gen_specials, panel_action_special);
		panel_action_special->Show(true);
		panel_gen_specials->Show(false);
		Layout();
	}
}

/* ActionSpecialPanel::applyTo
 * Applies selected special (if [apply_special] is true), trigger(s)
 * and args (if any) to [lines]
 *******************************************************************/
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
	if (panel_args)
	{
		// Get values
		int args[5];
		args[0] = panel_args->getArgValue(0);
		args[1] = panel_args->getArgValue(1);
		args[2] = panel_args->getArgValue(2);
		args[3] = panel_args->getArgValue(3);
		args[4] = panel_args->getArgValue(4);

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
	for (unsigned a = 0; a < lines.size(); a++)
	{
		// UDMF
		if (!cb_triggers.empty())
		{
			for (unsigned b = 0; b < cb_triggers.size(); b++)
			{
				if (cb_triggers[b]->Get3StateValue() == wxCHK_UNDETERMINED)
					continue;

				lines[a]->setBoolProperty(triggers_udmf[b], cb_triggers[b]->GetValue());
			}
		}

		// Hexen
		else if (choice_trigger && choice_trigger->GetSelection() >= 0)
			theGameConfiguration->setLineSpacTrigger(choice_trigger->GetSelection(), (MapLine*)lines[a]);
	}
}

/* ActionSpecialPanel::openLines
 * Loads special/trigger/arg values from [lines]
 *******************************************************************/
void ActionSpecialPanel::openLines(vector<MapObject*>& lines)
{
	if (lines.empty())
		return;

	// Special
	int special = lines[0]->intProperty("special");
	MapObject::multiIntProperty(lines, "special", special);
	setSpecial(special);

	// Args
	if (panel_args)
	{
		int args[5] = { -1, -1, -1, -1, -1 };
		MapObject::multiIntProperty(lines, "arg0", args[0]);
		MapObject::multiIntProperty(lines, "arg1", args[1]);
		MapObject::multiIntProperty(lines, "arg2", args[2]);
		MapObject::multiIntProperty(lines, "arg3", args[3]);
		MapObject::multiIntProperty(lines, "arg4", args[4]);
		panel_args->setValues(args);
	}

	// Trigger (UDMF)
	if (!cb_triggers.empty())
	{
		for (unsigned a = 0; a < triggers_udmf.size(); a++)
		{
			bool set;
			if (MapObject::multiBoolProperty(lines, triggers_udmf[a], set))
				cb_triggers[a]->SetValue(set);
			else
				cb_triggers[a]->Set3StateValue(wxCHK_UNDETERMINED);
		}
	}

	// Trigger (Hexen)
	else if (choice_trigger)
	{
		int trigger = theGameConfiguration->spacTriggerIndexHexen((MapLine*)lines[0]);
		for (unsigned a = 1; a < lines.size(); a++)
		{
			if (trigger != theGameConfiguration->spacTriggerIndexHexen((MapLine*)lines[a]))
			{
				trigger = -1;
				break;
			}
		}

		if (trigger >= 0)
			choice_trigger->SetSelection(trigger);
	}
}


/*******************************************************************
 * ACTIONSPECIALPANEL CLASS EVENTS
 *******************************************************************/

/* ActionSpecialPanel::onRadioButtonChanged
 * Called when the radio button selection is changed
 *******************************************************************/
void ActionSpecialPanel::onRadioButtonChanged(wxCommandEvent& e)
{
	// Swap panels
	showGeneralised(rb_generalised->GetValue());
}

/* ActionSpecialPanel::onSpecialSelectionChanged
 * Called when the action special selection is changed
 *******************************************************************/
void ActionSpecialPanel::onSpecialSelectionChanged(wxDataViewEvent& e)
{
	if ((theGameConfiguration->isBoom() && rb_generalised->GetValue()) || !panel_args)
	{
		e.Skip();
		return;
	}

	argspec_t args = theGameConfiguration->actionSpecial(selectedSpecial())->getArgspec();
	panel_args->setup(&args);
}


/*******************************************************************
 * ACTIONSPECIALDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* ActionSpecialDialog::ActionSpecialDialog
 * ActionSpecialDialog class constructor
 *******************************************************************/
ActionSpecialDialog::ActionSpecialDialog(wxWindow* parent, bool show_args)
: SDialog(parent, "Select Action Special", "actionspecial", 400, 500)
{
	panel_args = NULL;
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// No args
	if (theMapEditor->currentMapDesc().format == MAP_DOOM || !show_args)
	{
		panel_special = new ActionSpecialPanel(this);
		sizer->Add(panel_special, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);
	}

	// Args (use tabs)
	else
	{
		nb_tabs = new wxNotebook(this, -1);
		sizer->Add(nb_tabs, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);

		// Special panel
		panel_special = new ActionSpecialPanel(nb_tabs);
		nb_tabs->AddPage(panel_special, "Special");

		// Args panel
		panel_args = new ArgsPanel(nb_tabs);
		nb_tabs->AddPage(panel_args, "Args");
		panel_special->setArgsPanel(panel_args);
	}

	// Add buttons
	sizer->AddSpacer(4);
	sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);

	// Init
	SetMinClientSize(sizer->GetMinSize());
	CenterOnParent();
}

/* ActionSpecialDialog::~ActionSpecialDialog
 * ActionSpecialDialog class constructor
 *******************************************************************/
ActionSpecialDialog::~ActionSpecialDialog()
{
}

/* ActionSpecialDialog::setSpecial
 * Selects the item for special [special] in the specials tree
 *******************************************************************/
void ActionSpecialDialog::setSpecial(int special)
{
	panel_special->setSpecial(special);
	if (panel_args)
	{
		argspec_t args = theGameConfiguration->actionSpecial(special)->getArgspec();
		panel_args->setup(&args);
	}
}

/* ActionSpecialDialog::setArgs
 * Sets the arg values
 *******************************************************************/
void ActionSpecialDialog::setArgs(int args[5])
{
	if (panel_args)
		panel_args->setValues(args);
}

/* ActionSpecialDialog::selectedSpecial
 * Returns the currently selected action special
 *******************************************************************/
int ActionSpecialDialog::selectedSpecial()
{
	return panel_special->selectedSpecial();
}

/* ActionSpecialDialog::getArg
 * Returns the value of arg [index]
 *******************************************************************/
int ActionSpecialDialog::getArg(int index)
{
	if (panel_args)
		return panel_args->getArgValue(index);
	else
		return 0;
}

/* ActionSpecialDialog::applyTriggers
 * Applies selected trigger(s) (hexen or udmf) to [lines]
 *******************************************************************/
void ActionSpecialDialog::applyTo(vector<MapObject*>& lines, bool apply_special)
{
	panel_special->applyTo(lines, apply_special);
}

/* ActionSpecialDialog::openLines
 * Loads special/trigger/arg values from [lines]
 *******************************************************************/
void ActionSpecialDialog::openLines(vector<MapObject*>& lines)
{
	panel_special->openLines(lines);
}
