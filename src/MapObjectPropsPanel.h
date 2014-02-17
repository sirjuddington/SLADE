
#ifndef __MAP_OBJECT_PROPS_PANEL_H__
#define __MAP_OBJECT_PROPS_PANEL_H__

#include "MOPGProperty.h"
#include <wx/notebook.h>

class wxPropertyGrid;
class wxPGProperty;
class MapObject;
class UDMFProperty;
class MOPGProperty;
class MapObjectPropsPanel : public wxPanel
{
private:
	wxNotebook*				tabs_sections;
	wxPropertyGrid*			pg_properties;
	wxPropertyGrid*			pg_props_side1;
	wxPropertyGrid*			pg_props_side2;
	int						last_type;
	string					last_config;
	wxStaticText*			label_item;
	vector<MapObject*>		objects;
	vector<MOPGProperty*>	properties;
	wxPGProperty*			args[5];
	wxButton*				btn_reset;
	wxButton*				btn_apply;
	wxCheckBox*				cb_show_all;
	wxButton*				btn_add;
	wxPGProperty*			group_custom;

	MOPGProperty*	addBoolProperty(wxPGProperty* group, string label, string propname, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addIntProperty(wxPGProperty* group, string label, string propname, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addFloatProperty(wxPGProperty* group, string label, string propname, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addStringProperty(wxPGProperty* group, string label, string propname, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addLineFlagProperty(wxPGProperty* group, string label, string propname, int index, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addThingFlagProperty(wxPGProperty* group, string label, string propname, int index, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	MOPGProperty*	addTextureProperty(wxPGProperty* group, string label, string propname, int textype, bool readonly = false, wxPropertyGrid* grid = NULL, UDMFProperty* udmf_prop = NULL);
	void			addUDMFProperty(UDMFProperty* prop, int objtype, string basegroup = "", wxPropertyGrid* grid = NULL);

	bool	setBoolProperty(wxPGProperty* prop, bool value, bool force_set = false);

	void	setupType(int objtype);
	void	setupTypeUDMF(int objtype);

public:
	MapObjectPropsPanel(wxWindow* parent);
	~MapObjectPropsPanel();

	vector<MapObject*>&	getObjects() { return objects; }
	bool showAll() { return cb_show_all->IsChecked(); }

	void	openObject(MapObject* object);
	void	openObjects(vector<MapObject*>& objects);
	void	showApplyButton(bool show = true);
	void	updateArgs(MOPGIntWithArgsProperty* source);
	void	applyChanges();

	// Events
	void	onBtnApply(wxCommandEvent& e);
	void	onBtnReset(wxCommandEvent& e);
	void	onShowAllToggled(wxCommandEvent& e);
	void	onBtnAdd(wxCommandEvent& e);
};

#endif//__MAP_OBJECT_PROPS_PANEL_H__
