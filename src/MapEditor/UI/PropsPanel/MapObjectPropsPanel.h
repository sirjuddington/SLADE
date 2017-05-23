
#ifndef __MAP_OBJECT_PROPS_PANEL_H__
#define __MAP_OBJECT_PROPS_PANEL_H__

#include "PropsPanelBase.h"
#include "MOPGProperty.h"
#include "UI/STabCtrl.h"

class wxPropertyGrid;
class wxPGProperty;
class MapObject;
class UDMFProperty;
class MOPGProperty;
class MapObjectPropsPanel : public PropsPanelBase
{
private:
	TabControl*				stc_sections;
	wxPropertyGrid*			pg_properties;
	wxPropertyGrid*			pg_props_side1;
	wxPropertyGrid*			pg_props_side2;
	int						last_type;
	string					last_config;
	wxStaticText*			label_item;
	vector<MOPGProperty*>	properties;
	wxPGProperty*			args[5];
	wxButton*				btn_reset;
	wxButton*				btn_apply;
	wxCheckBox*				cb_show_all;
	wxButton*				btn_add;
	wxPGProperty*			group_custom;
	bool					no_apply;
	bool					udmf;

	// Hide properties
	bool			hide_flags;
	bool			hide_triggers;
	vector<string>	hide_props;

	MOPGProperty*	addBoolProperty(wxPGProperty* group, string label, string propname, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addIntProperty(wxPGProperty* group, string label, string propname, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addFloatProperty(wxPGProperty* group, string label, string propname, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addStringProperty(wxPGProperty* group, string label, string propname, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addLineFlagProperty(wxPGProperty* group, string label, string propname, int index, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addThingFlagProperty(wxPGProperty* group, string label, string propname, int index, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addTextureProperty(wxPGProperty* group, string label, string propname, int textype, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	void			addUDMFProperty(UDMFProperty& prop, int objtype, string basegroup = "", wxPropertyGrid* grid = NULL);

	bool	setBoolProperty(wxPGProperty* prop, bool value, bool force_set = false);

	void	setupType(int objtype);
	void	setupTypeUDMF(int objtype);

public:
	MapObjectPropsPanel(wxWindow* parent, bool no_apply = false);
	~MapObjectPropsPanel();

	vector<MapObject*>&	getObjects() { return objects; }
	bool showAll();

	void	openObject(MapObject* object);
	void	openObjects(vector<MapObject*>& objects);
	void	updateArgs(MOPGIntWithArgsProperty* source);
	void	applyChanges();
	void	clearGrid();
	void	hideFlags(bool hide) { hide_flags = hide; }
	void	hideTriggers(bool hide) { hide_triggers = hide; }
	void	hideProperty(string property) { hide_props.push_back(property); }
	void	clearHiddenProperties() { hide_props.clear(); }
	bool	propHidden(string property) { return VECTOR_EXISTS(hide_props, property); }

	// Events
	void	onBtnApply(wxCommandEvent& e);
	void	onBtnReset(wxCommandEvent& e);
	void	onShowAllToggled(wxCommandEvent& e);
	void	onBtnAdd(wxCommandEvent& e);
	void	onPropertyChanged(wxPropertyGridEvent& e);
};

#endif//__MAP_OBJECT_PROPS_PANEL_H__
