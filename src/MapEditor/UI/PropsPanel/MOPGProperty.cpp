
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MOPGProperty.cpp
 * Description: MOPGProperty and child classes - specialisations of
 *              wxProperty to handle various map object property
 *              types, including display and modification of values,
 *              for use with the MapObjectPropsPanel grid
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
#include "MOPGProperty.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "MapEditor/UI/Dialogs/ActionSpecialDialog.h"
#include "MapEditor/UI/Dialogs/MapTextureBrowser.h"
#include "MapEditor/UI/Dialogs/SectorSpecialDialog.h"
#include "MapEditor/UI/Dialogs/ThingTypeBrowser.h"
#include "MapObjectPropsPanel.h"
#include "MapEditor/MapEditContext.h"


/*******************************************************************
 * MOPGPROPERTY CLASS FUNCTIONS
 *******************************************************************/

/* MOPGProperty::resetValue
 * Reloads the property value from the object(s) currently open in
 * the parent MapObjectPropsPanel, if any
 *******************************************************************/
void MOPGProperty::resetValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Read value from selection
	openObjects(parent->getObjects());
}


/*******************************************************************
 * MOPGBOOLPROPERTY CLASS FUNCTIONS
 *******************************************************************/

/* MOPGBoolProperty::MOPGBoolProperty
 * MOPGBoolProperty class constructor
 *******************************************************************/
MOPGBoolProperty::MOPGBoolProperty(const wxString& label, const wxString& name)
	: wxBoolProperty(label, name, false)
{
	propname = name;
}

/* MOPGBoolProperty::openObjects
 * Reads the value of this boolean property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGBoolProperty::openObjects(vector<MapObject*>& objects)
{
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	bool first = objects[0]->boolProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (objects[a]->boolProperty(GetName()) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	updateVisibility();
	noupdate = false;
}

/* MOPGBoolProperty::updateVisibility
 * Default to hiding this property if set to its default value.
 *******************************************************************/
void MOPGBoolProperty::updateVisibility()
{
	if (
		!parent->showAll()
		&& !IsValueUnspecified()
		&& udmf_prop
		&& !udmf_prop->showAlways()
		&& udmf_prop->getDefaultValue().getBoolValue()
			== GetValue().GetBool()
	)
		Hide(true);
	else
		Hide(false);
}

/* MOPGBoolProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGBoolProperty::applyValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Do nothing if the value is unspecified
	if (IsValueUnspecified())
		return;

	// Go through objects and set this value
	vector<MapObject*>& objects = parent->getObjects();
	for (unsigned a = 0; a < objects.size(); a++)
		objects[a]->setBoolProperty(GetName(), m_value.GetBool());
}


/*******************************************************************
 * MOPGINTPROPERTY CLASS FUNCTIONS
 *******************************************************************/

/* MOPGIntProperty::MOPGIntProperty
 * MOPGIntProperty class constructor
 *******************************************************************/
MOPGIntProperty::MOPGIntProperty(const wxString& label, const wxString& name)
	: wxIntProperty(label, name, 0)
{
	propname = name;
}

/* MOPGIntProperty::openObjects
 * Reads the value of this integer property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGIntProperty::openObjects(vector<MapObject*>& objects)
{
	// Assume unspecified until we see otherwise
	SetValueToUnspecified();
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		return;
	}

	// Get property of first object
	int first = objects[0]->intProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (objects[a]->intProperty(GetName()) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	updateVisibility();
	noupdate = false;
}

/* MOPGIntProperty::updateVisibility
 * Default to hiding this property if set to its default value.
 *******************************************************************/
void MOPGIntProperty::updateVisibility()
{
	if (
		!parent->showAll()
		&& !IsValueUnspecified()
		&& udmf_prop
		&& !udmf_prop->showAlways()
		&& udmf_prop->getDefaultValue().getIntValue()
			== GetValue().GetInteger()
	)
		Hide(true);
	else
		Hide(false);
}

/* MOPGIntProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGIntProperty::applyValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Do nothing if the value is unspecified
	if (IsValueUnspecified())
		return;

	// Go through objects and set this value
	vector<MapObject*>& objects = parent->getObjects();
	for (unsigned a = 0; a < objects.size(); a++)
		objects[a]->setIntProperty(GetName(), m_value.GetInteger());
}


/*******************************************************************
 * MOPGFLOATPROPERTY CLASS FUNCTIONS
 *******************************************************************/

/* MOPGFloatProperty::MOPGFloatProperty
 * MOPGFloatProperty class constructor
 *******************************************************************/
MOPGFloatProperty::MOPGFloatProperty(const wxString& label, const wxString& name)
	: wxFloatProperty(label, name, 0)
{
	propname = name;
}

/* MOPGFloatProperty::openObjects
 * Reads the value of this float property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGFloatProperty::openObjects(vector<MapObject*>& objects)
{
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	double first = objects[0]->floatProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (objects[a]->floatProperty(GetName()) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	updateVisibility();
	noupdate = false;
}

/* MOPGFloatProperty::updateVisibility
 * Default to hiding this property if set to its default value.
 *******************************************************************/
void MOPGFloatProperty::updateVisibility()
{
	if (
		!parent->showAll()
		&& !IsValueUnspecified()
		&& udmf_prop
		&& !udmf_prop->showAlways()
		&& udmf_prop->getDefaultValue().getFloatValue()
			== GetValue().GetDouble()
	)
		Hide(true);
	else
		Hide(false);
}

/* MOPGFloatProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGFloatProperty::applyValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Do nothing if the value is unspecified
	if (IsValueUnspecified())
		return;

	// Go through objects and set this value
	vector<MapObject*>& objects = parent->getObjects();
	for (unsigned a = 0; a < objects.size(); a++)
		objects[a]->setFloatProperty(GetName(), m_value.GetDouble());
}


/*******************************************************************
 * MOPGSTRINGPROPERTY CLASS FUNCTIONS
 *******************************************************************/

/* MOPGStringProperty::MOPGStringProperty
 * MOPGStringProperty class constructor
 *******************************************************************/
MOPGStringProperty::MOPGStringProperty(const wxString& label, const wxString& name)
	: wxStringProperty(label, name, "")
{
	propname = name;
}

/* MOPGStringProperty::setUDMFProp
 * Load a list of possible choices from the given UDMF prop, if any
 *******************************************************************/
void MOPGStringProperty::setUDMFProp(UDMFProperty* prop)
{
	MOPGProperty::setUDMFProp(prop);

	// If this is a soft enum (e.g. renderstyle can be "translucent" or "add",
	// but we don't want to enforce that strictly), use a combobox populated
	// with the possible values
	if (prop && prop->hasPossibleValues())
	{
		const vector<Property> values = prop->getPossibleValues();
		wxPGChoices choices = wxPGChoices();

		for (unsigned n = 0; n < values.size(); n++)
			choices.Add(values[n].getStringValue());

		SetChoices(choices);
		SetEditor(wxPGEditor_ComboBox);
	}
	//else
	//	SetEditor(wxPGEditor_TextCtrl);
}

/* MOPGStringProperty::openObjects
 * Reads the value of this string property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGStringProperty::openObjects(vector<MapObject*>& objects)
{
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	string first = objects[0]->stringProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (objects[a]->stringProperty(GetName()) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	updateVisibility();
	noupdate = false;
}

/* MOPGStringProperty::updateVisibility
 * Default to hiding this property if set to its default value.
 *******************************************************************/
void MOPGStringProperty::updateVisibility()
{
	if (
		!parent->showAll()
		&& !IsValueUnspecified()
		&& udmf_prop
		&& !udmf_prop->showAlways()
		&& udmf_prop->getDefaultValue().getStringValue()
			== GetValue().GetString()
	)
		Hide(true);
	else
		Hide(false);
}

/* MOPGStringProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGStringProperty::applyValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Do nothing if the value is unspecified
	if (IsValueUnspecified())
		return;

	// Go through objects and set this value
	vector<MapObject*>& objects = parent->getObjects();
	for (unsigned a = 0; a < objects.size(); a++)
		objects[a]->setStringProperty(GetName(), m_value.GetString());
}


/*******************************************************************
 * MOPGINTWITHARGSPROPERTY CLASS FUNCTIONS
 *******************************************************************
 * Superclass for shared functionality between action specials and things,
 * which both have arguments.  Arguments that are used by the engine (i.e.
 * those with names) should always be shown even if zero.
 */

/* MOPGIntWithArgsProperty::MOPGIntWithArgsProperty
 * MOPGIntWithArgsProperty class constructor
 *******************************************************************/
MOPGIntWithArgsProperty::MOPGIntWithArgsProperty(const wxString& label, const wxString& name)
	: MOPGIntProperty(label, name)
{
	// Set to text+button editor
	SetEditor(wxPGEditor_TextCtrlAndButton);
}

/* MOPGIntWithArgsProperty::hasArgs()
 * Return whether the selected special or thing type takes any arguments.
 *******************************************************************/
bool MOPGIntWithArgsProperty::hasArgs()
{
	return getArgspec().count > 0;
}

/* MOPGIntWithArgsProperty::updateArgs()
 * Update the UI to show the names of the arguments for the current special or
 * thing type, and hide those that don't have names.
 *******************************************************************/
void MOPGIntWithArgsProperty::updateArgs(wxPGProperty* args[5])
{
	argspec_t argspec = getArgspec();
	int default_value = 0;
	unsigned argcount;

	if (udmf_prop)
		default_value = udmf_prop->getDefaultValue().getIntValue();

	if (parent->showAll())
		argcount = 5;
	else if (IsValueUnspecified())
		argcount = 0;
	else
		argcount = argspec.count;

	for (unsigned a = 0; a < 5; a++)
	{
		if (! args[a])
			continue;

		if (IsValueUnspecified())
		{
			args[a]->SetLabel(S_FMT("Arg%d", a+1));
			args[a]->SetHelpString("");
		}
		else
		{
			args[a]->SetLabel(argspec.getArg(a).name);
			args[a]->SetHelpString(argspec.getArg(a).desc);
		}

		// Show any args that this special uses, hide the others, but never
		// hide an arg with a value
		args[a]->Hide(
			a >= argcount
			&& ! args[a]->IsValueUnspecified()
			&& args[a]->GetValue().GetInteger() == default_value
		);
	}
}

/* MOPGIntWithArgsProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGIntWithArgsProperty::applyValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Do nothing if the value is unspecified
	if (IsValueUnspecified())
		return;

	// Initialize any unset and meaningful args to 0
	const argspec_t argspec = getArgspec();

	// Go through objects and set this value
	vector<MapObject*>& objects = parent->getObjects();
	for (unsigned a = 0; a < objects.size(); a++)
	{
		objects[a]->setIntProperty(GetName(), m_value.GetInteger());

		for (int argn = 0; argn < argspec.count; argn++)
		{
			string key = S_FMT("arg%d", argn);
			if (! objects[a]->hasProp(key))
				objects[a]->setIntProperty(key, 0);
		}
	}
}

/* MOPGIntWithArgsProperty::OnSetValue
 * Ask the parent to refresh the args display after the value changes
 *******************************************************************/
void MOPGIntWithArgsProperty::OnSetValue()
{
	if (parent)
		parent->updateArgs(this);

	MOPGIntProperty::OnSetValue();
}


/*******************************************************************
 * MOPGACTIONSPECIALPROPERTY CLASS FUNCTIONS
 *******************************************************************
 * Property grid cell for action special properties, links to
 * 5 other cells for the special args (which will update when the
 * special value is changed)
 */

/* MOPGActionSpecialProperty::getArgspec
 * Returns a little object describing the args for this thing type
 *******************************************************************/
const argspec_t MOPGActionSpecialProperty::getArgspec()
{
	int special = m_value.GetInteger();
	ActionSpecial* as = theGameConfiguration->actionSpecial(special);
	return as->getArgspec();
}

/* MOPGActionSpecialProperty::ValueToString
 * Returns the action special value as a string (contains the special
 * name)
 *******************************************************************/
wxString MOPGActionSpecialProperty::ValueToString(wxVariant& value, int argFlags) const
{
	// Get value as integer
	int special = value.GetInteger();

	if (special == 0)
		return "0: None";
	else
	{
		ActionSpecial* as = theGameConfiguration->actionSpecial(special);
		return S_FMT("%d: %s", special, as->getName());
	}
}

/* MOPGActionSpecialProperty::OnEvent
 * Called when an event is raised for the control
 *******************************************************************/
bool MOPGActionSpecialProperty::OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e)
{
	// '...' button clicked
	if (e.GetEventType() == wxEVT_BUTTON)
	{
		int special = -1;
		ActionSpecialDialog dlg(MapEditor::windowWx());
		dlg.setSpecial(GetValue().GetInteger());
		if (dlg.ShowModal() == wxID_OK)
			special = dlg.selectedSpecial();

		if (special >= 0)
			GetGrid()->ChangePropertyValue(this, special);
	}

	return wxIntProperty::OnEvent(propgrid, window, e);
}


/*******************************************************************
 * MOPGTHINGTYPEPROPERTY CLASS FUNCTIONS
 *******************************************************************
 * Behaves similarly to MOPGActionSpecialProperty, except for
 * thing types
 */

/* MOPGThingTypeProperty::getArgspec
 * Returns a little object describing the args for this thing type
 *******************************************************************/
const argspec_t MOPGThingTypeProperty::getArgspec()
{
	int type = m_value.GetInteger();
	ThingType* tt = theGameConfiguration->thingType(type);
	return tt->getArgspec();
}

/* MOPGThingTypeProperty::ValueToString
 * Returns the thing type value as a string (contains the thing
 * type name)
 *******************************************************************/
wxString MOPGThingTypeProperty::ValueToString(wxVariant& value, int argFlags) const
{
	// Get value as integer
	int type = value.GetInteger();

	if (type == 0)
		return "0: None";


	ThingType* tt = theGameConfiguration->thingType(type);
	return S_FMT("%d: %s", type, tt->getName());
}

/* MOPGThingTypeProperty::OnEvent
 * Called when an event is raised for the control
 *******************************************************************/
bool MOPGThingTypeProperty::OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e)
{
	if (e.GetEventType() == wxEVT_BUTTON)
	{
		// Get type to select initially (if any)
		int init_type = -1;
		if (!IsValueUnspecified())
			init_type = GetValue().GetInteger();

		// Open thing browser
		ThingTypeBrowser browser(MapEditor::windowWx(), init_type);
		if (browser.ShowModal() == wxID_OK)
		{
			// Set the value if a type was selected
			int type = browser.getSelectedType();
			if (type >= 0)
				GetGrid()->ChangePropertyValue(this, type);
		}
	}

	return wxIntProperty::OnEvent(propgrid, window, e);
}


/*******************************************************************
 * MOPGLINEFLAGPROPERTY CLASS FUNCTIONS
 *******************************************************************/

/* MOPGLineFlagProperty::MOPGLineFlagProperty
 * MOPGLineFlagProperty class constructor
 *******************************************************************/
MOPGLineFlagProperty::MOPGLineFlagProperty(const wxString& label, const wxString& name, int index)
	: MOPGBoolProperty(label, name)
{
	// Init variables
	this->index = index;
}

/* MOPGLineFlagProperty::openObjects
 * Reads the value of this line flag property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGLineFlagProperty::openObjects(vector<MapObject*>& objects)
{
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		SetValueToUnspecified();
		return;
	}

	// Check flag against first object
	bool first = theGameConfiguration->lineFlagSet(index, (MapLine*)objects[0]);

	// Check whether all objects share the same flag setting
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (theGameConfiguration->lineFlagSet(index, (MapLine*)objects[a]) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	updateVisibility();
	noupdate = false;
}

/* MOPGLineFlagProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGLineFlagProperty::applyValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Do nothing if the value is unspecified
	if (IsValueUnspecified())
		return;

	// Go through objects and set this value
	vector<MapObject*>& objects = parent->getObjects();
	for (unsigned a = 0; a < objects.size(); a++)
		theGameConfiguration->setLineFlag(index, (MapLine*)objects[a], GetValue());
}


/*******************************************************************
 * MOPGTHINGFLAGPROPERTY CLASS FUNCTIONS
 *******************************************************************/

/* MOPGThingFlagProperty::MOPGThingFlagProperty
 * MOPGThingFlagProperty class constructor
 *******************************************************************/
MOPGThingFlagProperty::MOPGThingFlagProperty(const wxString& label, const wxString& name, int index)
	: MOPGBoolProperty(label, name)
{
	// Init variables
	this->index = index;
}

/* MOPGThingFlagProperty::openObjects
 * Reads the value of this thing flag property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGThingFlagProperty::openObjects(vector<MapObject*>& objects)
{
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		SetValueToUnspecified();
		return;
	}

	// Check flag against first object
	bool first = theGameConfiguration->thingFlagSet(index, (MapThing*)objects[0]);

	// Check whether all objects share the same flag setting
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (theGameConfiguration->thingFlagSet(index, (MapThing*)objects[a]) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	updateVisibility();
	noupdate = false;
}

/* MOPGThingFlagProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGThingFlagProperty::applyValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Do nothing if the value is unspecified
	if (IsValueUnspecified())
		return;

	// Go through objects and set this value
	vector<MapObject*>& objects = parent->getObjects();
	for (unsigned a = 0; a < objects.size(); a++)
		theGameConfiguration->setThingFlag(index, (MapThing*)objects[a], GetValue());
}


/*******************************************************************
 * MOPGANGLEPROPERTY CLASS FUNCTIONS
 *******************************************************************/

/* MOPGAngleProperty::MOPGAngleProperty
 * MOPGAngleProperty class constructor
 *******************************************************************/
MOPGAngleProperty::MOPGAngleProperty(const wxString& label, const wxString& name)
	: wxEditEnumProperty(label, name)
{
	propname = name;
	// Set to combo box editor
	//SetEditor(wxPGEditor_ComboBox);

	// Setup combo box choices
	wxArrayString labels;
	wxArrayInt values;
	labels.Add("0: East");
	values.Add(0);
	labels.Add("45: Northeast");
	values.Add(45);
	labels.Add("90: North");
	values.Add(90);
	labels.Add("135: Northwest");
	values.Add(135);
	labels.Add("180: West");
	values.Add(180);
	labels.Add("225: Southwest");
	values.Add(225);
	labels.Add("270: South");
	values.Add(270);
	labels.Add("315: Southeast");
	values.Add(315);

	SetChoices(wxPGChoices(labels, values));
}

/* MOPGAngleProperty::openObjects
 * Reads the value of this angle property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGAngleProperty::openObjects(vector<MapObject*>& objects)
{
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	int first = objects[0]->intProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (objects[a]->intProperty(GetName()) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	updateVisibility();
	noupdate = false;
}

/* MOPGAngleProperty::updateVisibility
 * Default to hiding this property if set to its default value.
 *******************************************************************/
void MOPGAngleProperty::updateVisibility()
{
	if (
		!parent->showAll()
		&& !IsValueUnspecified()
		&& udmf_prop
		&& !udmf_prop->showAlways()
		&& udmf_prop->getDefaultValue().getIntValue()
			== GetValue().GetInteger()
	)
		Hide(true);
	else
		Hide(false);
}

/* MOPGAngleProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGAngleProperty::applyValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Do nothing if the value is unspecified
	if (IsValueUnspecified())
		return;

	// Go through objects and set this value
	vector<MapObject*>& objects = parent->getObjects();
	for (unsigned a = 0; a < objects.size(); a++)
		objects[a]->setIntProperty(GetName(), m_value.GetInteger());
}

/* MOPGAngleProperty::ValueToString
 * Returns the angle value as a string
 *******************************************************************/
wxString MOPGAngleProperty::ValueToString(wxVariant& value, int argFlags) const
{
	int angle = value.GetInteger();

	switch (angle)
	{
	case 0:		return "0: East"; break;
	case 45:	return "45: Northeast"; break;
	case 90:	return "90: North"; break;
	case 135:	return "135: Northwest"; break;
	case 180:	return "180: West"; break;
	case 225:	return "225: Southwest"; break;
	case 270:	return "270: South"; break;
	case 315:	return "315: Southeast"; break;
	default:	return S_FMT("%d", angle); break;
	}
}


/*******************************************************************
 * MOPGCOLOURPROPERTY CLASS FUNCTIONS
 *******************************************************************/

/* MOPGColourProperty::MOPGColourProperty
 * MOPGColourProperty class constructor
 *******************************************************************/
MOPGColourProperty::MOPGColourProperty(const wxString& label, const wxString& name)
	: wxColourProperty(label, name)
{
	propname = name;
}

/* MOPGColourProperty::openObjects
 * Reads the value of this colour property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGColourProperty::openObjects(vector<MapObject*>& objects)
{
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	int first = objects[0]->intProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (objects[a]->intProperty(GetName()) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	wxColor col(first);
	col.Set(col.Blue(), col.Green(), col.Red());
	wxVariant var_value;
	var_value << col;
	SetValue(var_value);
	updateVisibility();
	noupdate = false;
}

/* MOPGColourProperty::updateVisibility
 * Colours have no default and are always visible.
 *******************************************************************/
void MOPGColourProperty::updateVisibility()
{
	Hide(false);
}

/* MOPGColourProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGColourProperty::applyValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Do nothing if the value is unspecified
	if (IsValueUnspecified())
		return;

	// Go through objects and set this value
	vector<MapObject*>& objects = parent->getObjects();
	wxColor col;
	col << m_value;
	col.Set(col.Blue(), col.Green(), col.Red());
	for (unsigned a = 0; a < objects.size(); a++)
		objects[a]->setIntProperty(GetName(), col.GetRGB());
}


/*******************************************************************
 * MOPGTEXTUREPROPERTY CLASS FUNCTIONS
 *******************************************************************/

/* MOPGTextureProperty::MOPGTextureProperty
 * MOPGTextureProperty class constructor
 *******************************************************************/
MOPGTextureProperty::MOPGTextureProperty(int textype, const wxString& label, const wxString& name)
	: MOPGStringProperty(label, name)
{
	// Init variables
	this->textype = textype;

	// Set to text+button editor
	SetEditor(wxPGEditor_TextCtrlAndButton);
}

/* MOPGTextureProperty::openObjects
 * Reads the value of this texture property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGTextureProperty::openObjects(vector<MapObject*>& objects)
{
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	string first = objects[0]->stringProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (objects[a]->stringProperty(GetName()) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	updateVisibility();
	noupdate = false;
}

/* MOPGTextureProperty::OnEvent
 * Called when an event is raised for the control
 *******************************************************************/
bool MOPGTextureProperty::OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e)
{
	// '...' button clicked
	if (e.GetEventType() == wxEVT_BUTTON)
	{
		// Get current texture (if any)
		string tex_current = "";
		if (!IsValueUnspecified())
			tex_current = GetValueAsString();

		// Open map texture browser
		MapTextureBrowser browser(MapEditor::windowWx(), textype, tex_current, &(MapEditor::editContext().map()));
		if (browser.ShowModal() == wxID_OK && browser.getSelectedItem())
			GetGrid()->ChangePropertyValue(this, browser.getSelectedItem()->getName());

		// Refresh text
		RefreshEditor();
	}
	
	return wxStringProperty::OnEvent(propgrid, window, e);
}


/*******************************************************************
 * MOPGSPACTRIGGERPROPERTY CLASS FUNCTIONS
 *******************************************************************/

/* MOPGSPACTriggerProperty::MOPGSPACTriggerProperty
 * MOPGSPACTriggerProperty class constructor
 *******************************************************************/
MOPGSPACTriggerProperty::MOPGSPACTriggerProperty(const wxString& label, const wxString& name)
	: wxEnumProperty(label, name)
{
	propname = name;

	// Set to combo box editor
	SetEditor(wxPGEditor_ComboBox);

	// Setup combo box choices
	wxArrayString labels = theGameConfiguration->allSpacTriggers();
	SetChoices(wxPGChoices(labels));
}

/* MOPGSPACTriggerProperty::openObjects
 * Reads the value of this SPAC trigger property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGSPACTriggerProperty::openObjects(vector<MapObject*>& objects)
{
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	int map_format = MapEditor::editContext().mapDesc().format;
	string first = theGameConfiguration->spacTriggerString((MapLine*)objects[0], map_format);

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (theGameConfiguration->spacTriggerString((MapLine*)objects[a], map_format) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	updateVisibility();
	noupdate = false;
}

/* MOPGSPACTriggerProperty::updateVisibility
 * Default to hiding this property if set to its default value.
 *******************************************************************/
void MOPGSPACTriggerProperty::updateVisibility()
{
	if (
		!parent->showAll()
		&& !IsValueUnspecified()
		&& udmf_prop
		&& !udmf_prop->showAlways()
		&& udmf_prop->getDefaultValue().getIntValue()
			== GetValue().GetInteger()
	)
		Hide(true);
	else
		Hide(false);
}

/* MOPGSPACTriggerProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGSPACTriggerProperty::applyValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Do nothing if the value is unspecified
	if (IsValueUnspecified())
		return;

	// Go through objects and set this value
	vector<MapObject*>& objects = parent->getObjects();
	for (unsigned a = 0; a < objects.size(); a++)
		theGameConfiguration->setLineSpacTrigger(GetChoiceSelection(), (MapLine*)objects[a]);
	//objects[a]->setIntProperty(GetName(), m_value.GetInteger());
}


/*******************************************************************
 * MOPGTAGPROPERTY CLASS FUNCTIONS
 *******************************************************************
 * Property grid cell to handle tag properties, the '...' button will
 * set it to the next free tag or id depending on the object type
 */

/* MOPGTagProperty::MOPGTagProperty
 * MOPGTagProperty class constructor
 *******************************************************************/
MOPGTagProperty::MOPGTagProperty(int tagtype, const wxString& label, const wxString& name)
	: MOPGIntProperty(label, name)
{
	this->tagtype = tagtype;

	// Set to text+button editor
	SetEditor(wxPGEditor_TextCtrlAndButton);
}

/* MOPGTagProperty::openObjects
 * Reads the value of this tag/id property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGTagProperty::openObjects(vector<MapObject*>& objects)
{
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	int first = objects[0]->intProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (objects[a]->intProperty(GetName()) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	updateVisibility();
	noupdate = false;
}

/* MOPGTagProperty::OnEvent
 * Called when an event is raised for the control
 *******************************************************************/
bool MOPGTagProperty::OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e)
{
	if (e.GetEventType() == wxEVT_BUTTON)
	{
		vector<MapObject*>& objects = parent->getObjects();
		if (objects.size() == 0)
			return false;
		if (!objects[0]->getParentMap())
			return false;

		// Get unused tag/id depending on object type
		int tag = GetValue().GetInteger();
		if (tagtype == TT_SECTORTAG)//objects[0]->getObjType() == MOBJ_SECTOR)
			tag = objects[0]->getParentMap()->findUnusedSectorTag();
		else if (tagtype == TT_LINEID)//objects[0]->getObjType() == MOBJ_LINE)
			tag = objects[0]->getParentMap()->findUnusedLineId();
		else if (tagtype == TT_THINGID)//objects[0]->getObjType() == MOBJ_THING)
			tag = objects[0]->getParentMap()->findUnusedThingId();

		GetGrid()->ChangePropertyValue(this, tag);
		return true;
	}

	return wxIntProperty::OnEvent(propgrid, window, e);
}


/*******************************************************************
 * MOPGSECTORSPECIAL CLASS FUNCTIONS
 *******************************************************************/

/* MOPGSectorSpecialProperty::MOPGSectorSpecialProperty
 * MOPGSectorSpecialProperty class constructor
 *******************************************************************/
MOPGSectorSpecialProperty::MOPGSectorSpecialProperty(const wxString& label, const wxString& name)
	: MOPGIntProperty(label, name)
{
	// Set to text+button editor
	SetEditor(wxPGEditor_TextCtrlAndButton);
}

/* MOPGSectorSpecialProperty::openObjects
 * Reads the value of this sector special property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGSectorSpecialProperty::openObjects(vector<MapObject*>& objects)
{
	// Set unspecified if no objects given
	if (objects.size() == 0)
	{
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	int first = objects[0]->intProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++)
	{
		if (objects[a]->intProperty(GetName()) != first)
		{
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	updateVisibility();
	noupdate = false;
}

/* MOPGSectorSpecialProperty::ValueToString
 * Returns the sector special value as a string
 *******************************************************************/
wxString MOPGSectorSpecialProperty::ValueToString(wxVariant& value, int argFlags) const
{
	int type = value.GetInteger();

	return S_FMT("%d: %s", type, theGameConfiguration->sectorTypeName(type));
}

/* MOPGSectorSpecialProperty::OnEvent
 * Called when an event is raised for the control
 *******************************************************************/
bool MOPGSectorSpecialProperty::OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e)
{
	// '...' button clicked
	if (e.GetEventType() == wxEVT_BUTTON)
	{
		SectorSpecialDialog dlg(MapEditor::windowWx());
		dlg.setup(m_value.GetInteger());
		if (dlg.ShowModal() == wxID_OK)
			GetGrid()->ChangePropertyValue(this, dlg.getSelectedSpecial());

		return true;
	}

	return wxIntProperty::OnEvent(propgrid, window, e);
}
