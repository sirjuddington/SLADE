#pragma once

#include "MOPGProperty.h"
#include "PropsPanelBase.h"
#include "SLADEMap/MapObject/MapObject.h"
#include "UI/Controls/STabCtrl.h"

class wxPropertyGrid;
class wxPGProperty;

namespace slade
{
namespace game
{
	class UDMFProperty;
}
class MOPGProperty;

class MapObjectPropsPanel : public PropsPanelBase
{
public:
	MapObjectPropsPanel(wxWindow* parent, bool no_apply = false);
	~MapObjectPropsPanel() {}

	vector<MapObject*>& objects() { return objects_; }
	bool                showAll() const;

	void openObject(MapObject* object);
	void openObjects(vector<MapObject*>& objects) override;
	void updateArgs(MOPGIntWithArgsProperty* source);
	void applyChanges() override;
	void clearGrid();
	void hideFlags(bool hide) { hide_flags_ = hide; }
	void hideTriggers(bool hide) { hide_triggers_ = hide; }
	void hideProperty(wxString property) { hide_props_.push_back(property); }
	void clearHiddenProperties() { hide_props_.clear(); }
	bool propHidden(wxString property) { return VECTOR_EXISTS(hide_props_, property); }

private:
	TabControl*           stc_sections_   = nullptr;
	wxPropertyGrid*       pg_properties_  = nullptr;
	wxPropertyGrid*       pg_props_side1_ = nullptr;
	wxPropertyGrid*       pg_props_side2_ = nullptr;
	MapObject::Type       last_type_      = MapObject::Type::Object;
	wxString              last_config_;
	wxStaticText*         label_item_ = nullptr;
	vector<MOPGProperty*> properties_;
	wxPGProperty*         args_[5]      = {};
	wxButton*             btn_reset_    = nullptr;
	wxButton*             btn_apply_    = nullptr;
	wxCheckBox*           cb_show_all_  = nullptr;
	wxButton*             btn_add_      = nullptr;
	wxPGProperty*         group_custom_ = nullptr;
	bool                  no_apply_     = false;
	bool                  udmf_         = false;

	// Hide properties
	bool             hide_flags_    = false;
	bool             hide_triggers_ = false;
	vector<wxString> hide_props_;

	wxPropertyGrid* createPropGrid();

	template<typename T>
	T* addProperty(
		wxPGProperty*       group,
		T*                  prop,
		bool                readonly  = false,
		wxPropertyGrid*     grid      = nullptr,
		game::UDMFProperty* udmf_prop = nullptr);
	MOPGProperty* addBoolProperty(
		wxPGProperty*       group,
		const wxString&     label,
		const wxString&     propname,
		bool                readonly  = false,
		wxPropertyGrid*     grid      = nullptr,
		game::UDMFProperty* udmf_prop = nullptr);
	MOPGProperty* addIntProperty(
		wxPGProperty*       group,
		const wxString&     label,
		const wxString&     propname,
		bool                readonly  = false,
		wxPropertyGrid*     grid      = nullptr,
		game::UDMFProperty* udmf_prop = nullptr);
	MOPGProperty* addFloatProperty(
		wxPGProperty*       group,
		const wxString&     label,
		const wxString&     propname,
		bool                readonly  = false,
		wxPropertyGrid*     grid      = nullptr,
		game::UDMFProperty* udmf_prop = nullptr);
	MOPGProperty* addStringProperty(
		wxPGProperty*       group,
		const wxString&     label,
		const wxString&     propname,
		bool                readonly  = false,
		wxPropertyGrid*     grid      = nullptr,
		game::UDMFProperty* udmf_prop = nullptr);
	MOPGProperty* addLineFlagProperty(
		wxPGProperty*       group,
		const wxString&     label,
		const wxString&     propname,
		int                 index,
		bool                readonly  = false,
		wxPropertyGrid*     grid      = nullptr,
		game::UDMFProperty* udmf_prop = nullptr);
	MOPGProperty* addThingFlagProperty(
		wxPGProperty*       group,
		const wxString&     label,
		const wxString&     propname,
		int                 index,
		bool                readonly  = false,
		wxPropertyGrid*     grid      = nullptr,
		game::UDMFProperty* udmf_prop = nullptr);
	MOPGProperty* addTextureProperty(
		wxPGProperty*          group,
		const wxString&        label,
		const wxString&        propname,
		mapeditor::TextureType textype,
		bool                   readonly  = false,
		wxPropertyGrid*        grid      = nullptr,
		game::UDMFProperty*    udmf_prop = nullptr);
	void addUDMFProperty(
		game::UDMFProperty& prop,
		MapObject::Type     objtype,
		const wxString&     basegroup = "",
		wxPropertyGrid*     grid      = nullptr);

	bool setBoolProperty(wxPGProperty* prop, bool value, bool force_set = false) const;

	void setupType(MapObject::Type objtype);
	void setupTypeUDMF(MapObject::Type objtype);

	// Events
	void onBtnApply(wxCommandEvent& e);
	void onBtnReset(wxCommandEvent& e);
	void onShowAllToggled(wxCommandEvent& e);
	void onBtnAdd(wxCommandEvent& e);
	void onPropertyChanged(wxPropertyGridEvent& e);
};
} // namespace slade
