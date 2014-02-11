
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
#include "WxStuff.h"
#include "MOPGProperty.h"
#include "SLADEMap.h"
#include "ActionSpecialDialog.h"
#include "ThingTypeTreeView.h"
#include "GameConfiguration.h"
#include "MapObjectPropsPanel.h"
#include "ThingTypeBrowser.h"
#include "MapEditorWindow.h"
#include "MapTextureBrowser.h"
#include "SectorSpecialDialog.h"


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
	: wxBoolProperty(label, name, false), MOPGProperty(MOPGProperty::TYPE_BOOL)
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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getBoolValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
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
	: wxIntProperty(label, name, 0), MOPGProperty(MOPGProperty::TYPE_INT)
{
	propname = name;
}

/* MOPGIntProperty::openObjects
 * Reads the value of this integer property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGIntProperty::openObjects(vector<MapObject*>& objects)
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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getIntValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
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
	: wxFloatProperty(label, name, 0), MOPGProperty(MOPGProperty::TYPE_FLOAT)
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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getFloatValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
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
	: wxStringProperty(label, name, ""), MOPGProperty(MOPGProperty::TYPE_STRING)
{
	propname = name;
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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getStringValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
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
 * MOPGACTIONSPECIALPROPERTY CLASS FUNCTIONS
 *******************************************************************
 * Property grid cell for action special properties, links to
 * 5 other cells for the special args (which will update when the
 * special value is changed)
 */

/* MOPGActionSpecialProperty::MOPGActionSpecialProperty
 * MOPGActionSpecialProperty class constructor
 *******************************************************************/
MOPGActionSpecialProperty::MOPGActionSpecialProperty(const wxString& label, const wxString& name)
	: wxIntProperty(label, name, 0), MOPGProperty(MOPGProperty::TYPE_ASPECIAL)
{
	// Init variables
	args[0] = NULL;
	args[1] = NULL;
	args[2] = NULL;
	args[3] = NULL;
	args[4] = NULL;
	propname = name;

	// Set to text+button editor
	SetEditor(wxPGEditor_TextCtrlAndButton);
}

/* MOPGActionSpecialProperty::openObjects
 * Reads the value of this action special property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGActionSpecialProperty::openObjects(vector<MapObject*>& objects)
{
	// Reset arg property names
	for (unsigned a = 0; a < 5; a++)
	{
		if (args[a])
		{
			args[a]->SetLabel(S_FMT("Arg%d", a+1));
			args[a]->SetHelpString("");
		}
	}

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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getIntValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;

	updateArgNames();
}

/* MOPGActionSpecialProperty::addArgProperty
 * Adds a linked arg property [prop] at [index]
 *******************************************************************/
void MOPGActionSpecialProperty::addArgProperty(wxPGProperty* prop, int index)
{
	if (index < 5)
		args[index] = prop;
}

/* MOPGActionSpecialProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGActionSpecialProperty::applyValue()
{
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Do nothing if the value is unspecified
	if (IsValueUnspecified())
		return;

	// Initialize any unset and meaningful args to 0
	int special = m_value.GetInteger();
	ActionSpecial* as = theGameConfiguration->actionSpecial(special);

	// Go through objects and set this value
	vector<MapObject*>& objects = parent->getObjects();
	for (unsigned a = 0; a < objects.size(); a++)
	{
		objects[a]->setIntProperty(GetName(), m_value.GetInteger());

		for (int argn = 0; argn < as->getArgCount(); argn++)
		{
			string key = S_FMT("arg%d", argn);
			if (! objects[a]->hasProp(key))
				objects[a]->setIntProperty(key, 0);
		}
	}
}

void MOPGActionSpecialProperty::updateArgNames()
{
	// Reset names if the value is unspecified
	if (IsValueUnspecified())
	{
		for (unsigned a = 0; a < 5; a++)
		{
			if (!args[a])
				continue;

			args[a]->SetLabel(S_FMT("Arg%d", a+1));
			args[a]->SetHelpString("");
		}
		return;
	}

	int special = m_value.GetInteger();
	ActionSpecial* as = theGameConfiguration->actionSpecial(special);
	for (unsigned a = 0; a < 5; a++)
	{
		if (!args[a])
			continue;

		args[a]->SetLabel(as->getArg(a).name);
		args[a]->SetHelpString(as->getArg(a).desc);
	}
}

void MOPGActionSpecialProperty::updateArgVisibility()
{
	int argcount = 0;

	if (parent->showAll())
		return;

	if (! IsValueUnspecified())
	{
		// Find out how many args the special uses.  (An unknown
		// special effectively has zero.)
		int special = m_value.GetInteger();
		ActionSpecial* as = theGameConfiguration->actionSpecial(special);
		argcount = as->getArgCount();
	}

	// Show any args that this special uses, hide the others, but never hide an
	// arg with a value
	for (unsigned a = 0; a < 5; a++)
	{
		if (! args[a])
			continue;

		args[a]->Hide(
			a >= argcount
			&& ! args[a]->IsValueUnspecified()
			&& args[a]->GetValue().GetInteger() == 0
		);
	}
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
		ActionSpecialDialog dlg(window);
		dlg.setSpecial(GetValue().GetInteger());
		if (dlg.ShowModal() == wxID_OK)
			special = dlg.selectedSpecial();

		if (special >= 0)
		{
			SetValue(special);
			updateArgNames();
			updateArgVisibility();
		}
	}

	return wxIntProperty::OnEvent(propgrid, window, e);
}


/*******************************************************************
 * MOPGTHINGTYPEPROPERTY CLASS FUNCTIONS
 *******************************************************************
 * Behaves similarly to MOPGActionSpecialProperty, except for
 * thing types
 */

/* MOPGThingTypeProperty::MOPGThingTypeProperty
 * MOPGThingTypeProperty class constructor
 *******************************************************************/
MOPGThingTypeProperty::MOPGThingTypeProperty(const wxString& label, const wxString& name)
	: wxIntProperty(label, name, 0), MOPGProperty(MOPGProperty::TYPE_TTYPE)
{
	// Init variables
	args[0] = NULL;
	args[1] = NULL;
	args[2] = NULL;
	args[3] = NULL;
	args[4] = NULL;
	propname = name;

	// Set to text+button editor
	SetEditor(wxPGEditor_TextCtrlAndButton);
}

/* MOPGThingTypeProperty::openObjects
 * Reads the value of this thing type property from [objects]
 * (if the value differs between objects, it is set to unspecified)
 *******************************************************************/
void MOPGThingTypeProperty::openObjects(vector<MapObject*>& objects)
{
	// Reset arg property names
	for (unsigned a = 0; a < 5; a++)
	{
		if (args[a])
		{
			args[a]->SetLabel(S_FMT("Arg%d", a+1));
			args[a]->SetHelpString("");
		}
	}

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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getIntValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;

	// Set arg property names
	ThingType* tt = theGameConfiguration->thingType(first);
	for (unsigned a = 0; a < 5; a++)
	{
		if (!args[a])
			continue;

		args[a]->SetLabel(tt->getArg(a).name);
		args[a]->SetHelpString(tt->getArg(a).desc);
	}
}

/* MOPGThingTypeProperty::addArgProperty
 * Adds a linked arg property [prop] at [index]
 *******************************************************************/
void MOPGThingTypeProperty::addArgProperty(wxPGProperty* prop, int index)
{
	if (index < 5)
		args[index] = prop;
}

/* MOPGThingTypeProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGThingTypeProperty::applyValue()
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

/* MOPGThingTypeProperty::ValueToString
 * Returns the thing type value as a string (contains the thing
 * type name)
 *******************************************************************/
wxString MOPGThingTypeProperty::ValueToString(wxVariant& value, int argFlags) const
{
	// Get value as integer
	int type = value.GetInteger();

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
		ThingTypeBrowser browser(theMapEditor, init_type);
		if (browser.ShowModal() == wxID_OK)
		{
			// Set the value if a type was selected
			int type = browser.getSelectedType();
			if (type >= 0) SetValue(type);
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
	: wxBoolProperty(label, name, false), MOPGProperty(MOPGProperty::TYPE_LFLAG)
{
	// Init variables
	this->index = index;
	propname = name;
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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getBoolValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
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
	: wxBoolProperty(label, name, false), MOPGProperty(MOPGProperty::TYPE_LFLAG)
{
	// Init variables
	this->index = index;
	propname = name;
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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getBoolValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
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
	: wxEditEnumProperty(label, name), MOPGProperty(MOPGProperty::TYPE_ANGLE)
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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getIntValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
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
	: wxColourProperty(label, name), MOPGProperty(MOPGProperty::TYPE_COLOUR)
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
	noupdate = false;
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
	: wxStringProperty(label, name), MOPGProperty(MOPGProperty::TYPE_TEXTURE)
{
	// Init variables
	this->textype = textype;
	propname = name;

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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getIntValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
}

/* MOPGTextureProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGTextureProperty::applyValue()
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
		MapTextureBrowser browser(theMapEditor, textype, tex_current, &(theMapEditor->mapEditor().getMap()));
		if (browser.ShowModal() == wxID_OK && browser.getSelectedItem())
			SetValue(browser.getSelectedItem()->getName());

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
	: wxEnumProperty(label, name), MOPGProperty(MOPGProperty::TYPE_SPAC)
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
	int map_format = theMapEditor->currentMapDesc().format;
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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getIntValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
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
MOPGTagProperty::MOPGTagProperty(const wxString& label, const wxString& name)
	: wxIntProperty(label, name, 0), MOPGProperty(MOPGProperty::TYPE_ID)
{
	propname = name;

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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getIntValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
}

/* MOPGTagProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGTagProperty::applyValue()
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
		if (objects[0]->getObjType() == MOBJ_SECTOR)
			tag = objects[0]->getParentMap()->findUnusedSectorTag();
		else if (objects[0]->getObjType() == MOBJ_THING)
			tag = objects[0]->getParentMap()->findUnusedThingId();
		else if (objects[0]->getObjType() == MOBJ_LINE)
			tag = objects[0]->getParentMap()->findUnusedLineId();

		SetValue(tag);
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
	: wxIntProperty(label, name, 0), MOPGProperty(MOPGProperty::TYPE_SSPECIAL)
{
	propname = name;

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
	if (!parent->showAll() && udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getIntValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
}

/* MOPGSectorSpecialProperty::applyValue
 * Applies the current property value to all objects currently open
 * in the parent MapObjectPropsPanel, if a value is specified
 *******************************************************************/
void MOPGSectorSpecialProperty::applyValue()
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

/* MOPGSectorSpecialProperty::OnEvent
 * Called when an event is raised for the control
 *******************************************************************/
bool MOPGSectorSpecialProperty::OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e)
{
	// '...' button clicked
	if (e.GetEventType() == wxEVT_BUTTON)
	{
		SectorSpecialDialog dlg(theMapEditor);
		int map_format = theMapEditor->currentMapDesc().format;
		dlg.setup(m_value.GetInteger(), map_format);
		if (dlg.ShowModal() == wxID_OK)
			SetValue(dlg.getSelectedSpecial(map_format));

		return true;
	}

	return wxIntProperty::OnEvent(propgrid, window, e);
}
