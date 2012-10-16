
#include "Main.h"
#include "WxStuff.h"
#include "MOPGProperty.h"
#include "SLADEMap.h"
#include "ActionSpecialTreeView.h"
#include "ThingTypeTreeView.h"
#include "GameConfiguration.h"
#include "MapObjectPropsPanel.h"
#include "ThingTypeBrowser.h"
#include "MapEditorWindow.h"
#include "MapTextureBrowser.h"


void MOPGProperty::resetValue() {
	// Do nothing if no parent (and thus no object list)
	if (!parent || noupdate)
		return;

	// Read value from selection
	openObjects(parent->getObjects());
}


MOPGBoolProperty::MOPGBoolProperty(const wxString& label, const wxString& name)
: wxBoolProperty(label, name, false), MOPGProperty(MOPGProperty::TYPE_BOOL) {
}

void MOPGBoolProperty::openObjects(vector<MapObject*>& objects) {
	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	bool first = objects[0]->boolProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++) {
		if (objects[a]->boolProperty(GetName()) != first) {
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	if (udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getBoolValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
}

void MOPGBoolProperty::applyValue() {
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


MOPGIntProperty::MOPGIntProperty(const wxString& label, const wxString& name)
: wxIntProperty(label, name, 0), MOPGProperty(MOPGProperty::TYPE_INT) {
}

void MOPGIntProperty::openObjects(vector<MapObject*>& objects) {
	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	int first = objects[0]->intProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++) {
		if (objects[a]->intProperty(GetName()) != first) {
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	if (udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getIntValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
}

void MOPGIntProperty::applyValue() {
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


MOPGFloatProperty::MOPGFloatProperty(const wxString& label, const wxString& name)
: wxFloatProperty(label, name, 0), MOPGProperty(MOPGProperty::TYPE_FLOAT) {
}

void MOPGFloatProperty::openObjects(vector<MapObject*>& objects) {
	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	double first = objects[0]->floatProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++) {
		if (objects[a]->floatProperty(GetName()) != first) {
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	if (udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getFloatValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
}

void MOPGFloatProperty::applyValue() {
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


MOPGStringProperty::MOPGStringProperty(const wxString& label, const wxString& name)
: wxStringProperty(label, name, ""), MOPGProperty(MOPGProperty::TYPE_STRING) {
}

void MOPGStringProperty::openObjects(vector<MapObject*>& objects) {
	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	string first = objects[0]->stringProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++) {
		if (objects[a]->stringProperty(GetName()) != first) {
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	if (udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getStringValue() == first)
		Hide(true);
	else
		Hide(false);
	SetValue(first);
	noupdate = false;
}

void MOPGStringProperty::applyValue() {
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


MOPGActionSpecialProperty::MOPGActionSpecialProperty(const wxString& label, const wxString& name)
: wxIntProperty(label, name, 0), MOPGProperty(MOPGProperty::TYPE_ASPECIAL) {
	// Init variables
	args[0] = NULL;
	args[1] = NULL;
	args[2] = NULL;
	args[3] = NULL;
	args[4] = NULL;

	// Set to text+button editor
	SetEditor(wxPGEditor_TextCtrlAndButton);
}

void MOPGActionSpecialProperty::openObjects(vector<MapObject*>& objects) {
	// Reset arg property names
	for (unsigned a = 0; a < 5; a++) {
		if (args[a]) {
			args[a]->SetLabel(S_FMT("Arg%d", a+1));
			args[a]->SetHelpString("");
		}
	}

	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	int first = objects[0]->intProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++) {
		if (objects[a]->intProperty(GetName()) != first) {
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	if (udmf_prop && !udmf_prop->showAlways() && udmf_prop->getDefaultValue().getIntValue() == first)
		Hide(true);
	else
		Hide(false);
	noupdate = false;

	// Set arg property names
	ActionSpecial* as = theGameConfiguration->actionSpecial(first);
	for (unsigned a = 0; a < 5; a++) {
		if (!args[a])
			continue;

		args[a]->SetLabel(as->getArg(a).name);
		args[a]->SetHelpString(as->getArg(a).desc);
	}
}

void MOPGActionSpecialProperty::addArgProperty(wxPGProperty* prop, int index) {
	if (index < 5)
		args[index] = prop;
}

void MOPGActionSpecialProperty::applyValue() {
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

wxString MOPGActionSpecialProperty::ValueToString(wxVariant &value, int argFlags) const {
	// Get value as integer
	int special = value.GetInteger();

	if (special == 0)
		return "0: None";
	else {
		ActionSpecial* as = theGameConfiguration->actionSpecial(special);
		return S_FMT("%d: %s", special, CHR(as->getName()));
	}
}

bool MOPGActionSpecialProperty::OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) {
	if (e.GetEventType() == wxEVT_COMMAND_BUTTON_CLICKED) {
		int special = ActionSpecialTreeView::showDialog(window, GetValue().GetInteger());
		if (special >= 0) SetValue(special);
	}

	return wxIntProperty::OnEvent(propgrid, window, e);
}


MOPGThingTypeProperty::MOPGThingTypeProperty(const wxString& label, const wxString& name)
: wxIntProperty(label, name, 0), MOPGProperty(MOPGProperty::TYPE_TTYPE) {
	// Init variables
	args[0] = NULL;
	args[1] = NULL;
	args[2] = NULL;
	args[3] = NULL;
	args[4] = NULL;

	// Set to text+button editor
	SetEditor(wxPGEditor_TextCtrlAndButton);
}

void MOPGThingTypeProperty::openObjects(vector<MapObject*>& objects) {
	// Reset arg property names
	for (unsigned a = 0; a < 5; a++) {
		if (args[a]) {
			args[a]->SetLabel(S_FMT("Arg%d", a+1));
			args[a]->SetHelpString("");
		}
	}

	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	int first = objects[0]->intProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++) {
		if (objects[a]->intProperty(GetName()) != first) {
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	noupdate = false;

	// Set arg property names
	ThingType* tt = theGameConfiguration->thingType(first);
	for (unsigned a = 0; a < 5; a++) {
		if (!args[a])
			continue;

		args[a]->SetLabel(tt->getArg(a).name);
		args[a]->SetHelpString(tt->getArg(a).desc);
	}
}

void MOPGThingTypeProperty::addArgProperty(wxPGProperty* prop, int index) {
	if (index < 5)
		args[index] = prop;
}

void MOPGThingTypeProperty::applyValue() {
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

wxString MOPGThingTypeProperty::ValueToString(wxVariant &value, int argFlags) const {
	// Get value as integer
	int type = value.GetInteger();

	ThingType* tt = theGameConfiguration->thingType(type);
	return S_FMT("%d: %s", type, CHR(tt->getName()));
}

bool MOPGThingTypeProperty::OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) {
	if (e.GetEventType() == wxEVT_COMMAND_BUTTON_CLICKED) {
		// Get type to select initially (if any)
		int init_type = -1;
		if (!IsValueUnspecified())
			init_type = GetValue().GetInteger();

		// Open thing browser
		ThingTypeBrowser browser(theMapEditor, init_type);
		if (browser.ShowModal() == wxID_OK) {
			// Set the value if a type was selected
			int type = browser.getSelectedType();
			if (type >= 0) SetValue(type);
		}
	}

	return wxIntProperty::OnEvent(propgrid, window, e);
}


MOPGLineFlagProperty::MOPGLineFlagProperty(const wxString& label, const wxString& name, int index)
: wxBoolProperty(label, name, false), MOPGProperty(MOPGProperty::TYPE_LFLAG) {
	// Init variables
	this->index = index;
}

void MOPGLineFlagProperty::openObjects(vector<MapObject*>& objects) {
	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Check flag against first object
	bool first = theGameConfiguration->lineFlagSet(index, (MapLine*)objects[0]);

	// Check whether all objects share the same flag setting
	for (unsigned a = 1; a < objects.size(); a++) {
		if (theGameConfiguration->lineFlagSet(index, (MapLine*)objects[a]) != first) {
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	noupdate = false;
}

void MOPGLineFlagProperty::applyValue() {
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


MOPGThingFlagProperty::MOPGThingFlagProperty(const wxString& label, const wxString& name, int index)
: wxBoolProperty(label, name, false), MOPGProperty(MOPGProperty::TYPE_LFLAG) {
	// Init variables
	this->index = index;
}

void MOPGThingFlagProperty::openObjects(vector<MapObject*>& objects) {
	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Check flag against first object
	bool first = theGameConfiguration->thingFlagSet(index, (MapThing*)objects[0]);

	// Check whether all objects share the same flag setting
	for (unsigned a = 1; a < objects.size(); a++) {
		if (theGameConfiguration->thingFlagSet(index, (MapThing*)objects[a]) != first) {
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	noupdate = false;
}

void MOPGThingFlagProperty::applyValue() {
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


MOPGAngleProperty::MOPGAngleProperty(const wxString& label, const wxString& name)
: wxEditEnumProperty(label, name), MOPGProperty(MOPGProperty::TYPE_ANGLE) {
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

void MOPGAngleProperty::openObjects(vector<MapObject*>& objects) {
	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	int first = objects[0]->intProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++) {
		if (objects[a]->intProperty(GetName()) != first) {
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	noupdate = false;
}

void MOPGAngleProperty::applyValue() {
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

wxString MOPGAngleProperty::ValueToString(wxVariant &value, int argFlags) const {
	int angle = value.GetInteger();

	switch (angle) {
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



MOPGColourProperty::MOPGColourProperty(const wxString& label, const wxString& name)
: wxColourProperty(label, name), MOPGProperty(MOPGProperty::TYPE_COLOUR) {
}

void MOPGColourProperty::openObjects(vector<MapObject*>& objects) {
	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	int first = objects[0]->intProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++) {
		if (objects[a]->intProperty(GetName()) != first) {
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

void MOPGColourProperty::applyValue() {
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


MOPGTextureProperty::MOPGTextureProperty(int textype, const wxString& label, const wxString& name)
: wxStringProperty(label, name), MOPGProperty(MOPGProperty::TYPE_TEXTURE) {
	// Init variables
	this->textype = textype;

	// Set to text+button editor
	SetEditor(wxPGEditor_TextCtrlAndButton);
}

void MOPGTextureProperty::openObjects(vector<MapObject*>& objects) {
	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	string first = objects[0]->stringProperty(GetName());

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++) {
		if (objects[a]->stringProperty(GetName()) != first) {
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	noupdate = false;
}

void MOPGTextureProperty::applyValue() {
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

bool MOPGTextureProperty::OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) {
	if (e.GetEventType() == wxEVT_COMMAND_BUTTON_CLICKED) {
		// Get current texture (if any)
		string tex_current = "";
		if (!IsValueUnspecified())
			tex_current = GetValueAsString();

		// Open map texture browser
		MapTextureBrowser browser(theMapEditor, textype, tex_current);
		if (browser.ShowModal() == wxID_OK && browser.getSelectedItem())
			SetValue(browser.getSelectedItem()->getName());

		// Refresh text
		RefreshEditor();
	}

	return wxStringProperty::OnEvent(propgrid, window, e);
}


MOPGSPACTriggerProperty::MOPGSPACTriggerProperty(const wxString& label, const wxString& name)
: wxEnumProperty(label, name), MOPGProperty(MOPGProperty::TYPE_SPAC) {
	// Set to combo box editor
	SetEditor(wxPGEditor_ComboBox);

	// Setup combo box choices
	wxArrayString labels = theGameConfiguration->allSpacTriggers();
	SetChoices(wxPGChoices(labels));
}

void MOPGSPACTriggerProperty::openObjects(vector<MapObject*>& objects) {
	// Set unspecified if no objects given
	if (objects.size() == 0) {
		SetValueToUnspecified();
		return;
	}

	// Get property of first object
	int map_format = theMapEditor->currentMapDesc().format;
	string first = theGameConfiguration->spacTriggerString((MapLine*)objects[0], map_format);

	// Check whether all objects share the same value
	for (unsigned a = 1; a < objects.size(); a++) {
		if (theGameConfiguration->spacTriggerString((MapLine*)objects[a], map_format) != first) {
			// Different value found, set unspecified
			SetValueToUnspecified();
			return;
		}
	}

	// Set to common value
	noupdate = true;
	SetValue(first);
	noupdate = false;
}

void MOPGSPACTriggerProperty::applyValue() {
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
