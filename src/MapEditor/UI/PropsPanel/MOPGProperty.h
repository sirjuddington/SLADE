#pragma once

#include "MapEditor/MapEditor.h"

namespace slade
{
namespace game
{
	struct ArgSpec;
	class UDMFProperty;
} // namespace game
class MapObjectPropsPanel;

class MOPGProperty
{
public:
	MOPGProperty(const wxString& prop_name) : propname_{ prop_name } {}
	virtual ~MOPGProperty() = default;

	enum class Type
	{
		Boolean = 0,
		Integer,
		Float,
		String,
		ActionSpecial,
		SectorSpecial,
		ThingType,
		LineFlag,
		ThingFlag,
		Angle,
		Colour,
		Texture,
		SPACTrigger,
		Id,
	};

	wxString     propName() const { return propname_; }
	void         setParent(MapObjectPropsPanel* parent) { parent_ = parent; }
	virtual void setUDMFProp(game::UDMFProperty* prop) { udmf_prop_ = prop; }

	virtual Type type()                                   = 0;
	virtual void openObjects(vector<MapObject*>& objects) = 0;
	virtual void updateVisibility()                       = 0;
	virtual void applyValue() {}
	virtual void resetValue();

protected:
	MapObjectPropsPanel* parent_    = nullptr;
	bool                 noupdate_  = false;
	game::UDMFProperty*  udmf_prop_ = nullptr;
	wxString             propname_;
};

class MOPGBoolProperty : public MOPGProperty, public wxBoolProperty
{
public:
	MOPGBoolProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Boolean; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGIntProperty : public MOPGProperty, public wxIntProperty
{
public:
	MOPGIntProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Integer; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGFloatProperty : public MOPGProperty, public wxFloatProperty
{
public:
	MOPGFloatProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Float; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGStringProperty : public MOPGProperty, public wxStringProperty
{
public:
	MOPGStringProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	void setUDMFProp(game::UDMFProperty* prop) override;

	Type type() override { return Type::String; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGIntWithArgsProperty : public MOPGIntProperty
{
protected:
	virtual const game::ArgSpec& argSpec() = 0;

public:
	MOPGIntWithArgsProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	void applyValue() override;
	bool hasArgs();
	void updateArgs(wxPGProperty* args[5]);

	// wxPGProperty overrides
	void OnSetValue() override;
};

class MOPGActionSpecialProperty : public MOPGIntWithArgsProperty
{
protected:
	const game::ArgSpec& argSpec() override;

public:
	MOPGActionSpecialProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL) :
		MOPGIntWithArgsProperty(label, name)
	{
	}

	Type type() override { return Type::ActionSpecial; }

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	bool     OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) override;
};

class MOPGThingTypeProperty : public MOPGIntWithArgsProperty
{
protected:
	const game::ArgSpec& argSpec() override;

public:
	MOPGThingTypeProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL) :
		MOPGIntWithArgsProperty(label, name)
	{
	}

	Type type() override { return Type::ThingType; }

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	bool     OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) override;
};

class MOPGLineFlagProperty : public MOPGBoolProperty
{
public:
	MOPGLineFlagProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL, int index = -1);

	Type type() override { return Type::LineFlag; }
	void openObjects(vector<MapObject*>& objects) override;
	void applyValue() override;

private:
	int index_;
};

class MOPGThingFlagProperty : public MOPGBoolProperty
{
public:
	MOPGThingFlagProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL, int index = -1);

	Type type() override { return Type::ThingFlag; }
	void openObjects(vector<MapObject*>& objects) override;
	void applyValue() override;

private:
	int index_;
};

class MOPGAngleProperty : public MOPGProperty, public wxEditEnumProperty
{
public:
	MOPGAngleProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Angle; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int arg_flags = 0) const override;
};

class MOPGColourProperty : public MOPGProperty, public wxColourProperty
{
public:
	MOPGColourProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Colour; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGTextureProperty : public MOPGStringProperty
{
public:
	MOPGTextureProperty(
		mapeditor::TextureType textype = mapeditor::TextureType::Texture,
		const wxString&        label   = wxPG_LABEL,
		const wxString&        name    = wxPG_LABEL);

	Type type() override { return Type::Texture; }
	void openObjects(vector<MapObject*>& objects) override;

	// wxPGProperty overrides
	bool OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) override;

private:
	mapeditor::TextureType textype_;
};

class MOPGSPACTriggerProperty : public MOPGProperty, public wxEnumProperty
{
public:
	MOPGSPACTriggerProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::SPACTrigger; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGTagProperty : public MOPGIntProperty
{
public:
	enum class IdType
	{
		Sector,
		Line,
		Thing
	};

	MOPGTagProperty(IdType id_type, const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Id; }
	void openObjects(vector<MapObject*>& objects) override;

	// wxPGProperty overrides
	bool OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) override;

private:
	IdType id_type_;
};

class MOPGSectorSpecialProperty : public MOPGIntProperty
{
public:
	MOPGSectorSpecialProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::SectorSpecial; }
	void openObjects(vector<MapObject*>& objects) override;

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	bool     OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) override;
};
} // namespace slade
