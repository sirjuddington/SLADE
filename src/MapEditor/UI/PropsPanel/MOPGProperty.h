#pragma once

#include "Game/Args.h"

class MapObject;
class MapObjectPropsPanel;
class UDMFProperty;
class MOPGProperty
{
public:
	MOPGProperty()
	{
		noupdate_  = false;
		udmf_prop_ = nullptr;
	}
	~MOPGProperty() {}

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

	string       getPropName() { return propname_; }
	void         setParent(MapObjectPropsPanel* parent) { this->parent_ = parent; }
	virtual void setUDMFProp(UDMFProperty* prop) { this->udmf_prop_ = prop; }

	virtual Type getType()                                = 0;
	virtual void openObjects(vector<MapObject*>& objects) = 0;
	virtual void updateVisibility()                       = 0;
	virtual void applyValue() {}
	virtual void resetValue();

protected:
	int                  type_;
	MapObjectPropsPanel* parent_;
	bool                 noupdate_;
	UDMFProperty*        udmf_prop_;
	string               propname_;
};

class MOPGBoolProperty : public MOPGProperty, public wxBoolProperty
{
public:
	MOPGBoolProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type getType() { return Type::Boolean; }
	void openObjects(vector<MapObject*>& objects);
	void updateVisibility();
	void applyValue();
};

class MOPGIntProperty : public MOPGProperty, public wxIntProperty
{
public:
	MOPGIntProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type getType() { return Type::Integer; }
	void openObjects(vector<MapObject*>& objects);
	void updateVisibility();
	void applyValue();
};

class MOPGFloatProperty : public MOPGProperty, public wxFloatProperty
{
public:
	MOPGFloatProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type getType() { return Type::Float; }
	void openObjects(vector<MapObject*>& objects);
	void updateVisibility();
	void applyValue();
};

class MOPGStringProperty : public MOPGProperty, public wxStringProperty
{
public:
	MOPGStringProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	void setUDMFProp(UDMFProperty* prop);

	Type getType() { return Type::String; }
	void openObjects(vector<MapObject*>& objects);
	void updateVisibility();
	void applyValue();
};

class MOPGIntWithArgsProperty : public MOPGIntProperty
{
protected:
	virtual const Game::ArgSpec& getArgspec() = 0;

public:
	MOPGIntWithArgsProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	void applyValue();
	bool hasArgs();
	void updateArgs(wxPGProperty* args[5]);

	// wxPGProperty overrides
	void OnSetValue();
};

class MOPGActionSpecialProperty : public MOPGIntWithArgsProperty
{
protected:
	const Game::ArgSpec& getArgspec();

public:
	MOPGActionSpecialProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL) :
		MOPGIntWithArgsProperty(label, name)
	{
	}

	Type getType() { return Type::ActionSpecial; }

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int argFlags = 0) const;
	bool     OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e);
};

class MOPGThingTypeProperty : public MOPGIntWithArgsProperty
{
protected:
	const Game::ArgSpec& getArgspec();

public:
	MOPGThingTypeProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL) :
		MOPGIntWithArgsProperty(label, name)
	{
	}

	Type getType() { return Type::ThingType; }

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int argFlags = 0) const;
	bool     OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e);
};

class MOPGLineFlagProperty : public MOPGBoolProperty
{
public:
	MOPGLineFlagProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL, int index = -1);

	Type getType() { return Type::LineFlag; }
	void openObjects(vector<MapObject*>& objects);
	void applyValue();

private:
	int index_;
};

class MOPGThingFlagProperty : public MOPGBoolProperty
{
public:
	MOPGThingFlagProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL, int index = -1);

	Type getType() { return Type::ThingFlag; }
	void openObjects(vector<MapObject*>& objects);
	void applyValue();

private:
	int index_;
};

class MOPGAngleProperty : public MOPGProperty, public wxEditEnumProperty
{
public:
	MOPGAngleProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type getType() { return Type::Angle; }
	void openObjects(vector<MapObject*>& objects);
	void updateVisibility();
	void applyValue();

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int argFlags = 0) const;
};

class MOPGColourProperty : public MOPGProperty, public wxColourProperty
{
public:
	MOPGColourProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type getType() { return Type::Colour; }
	void openObjects(vector<MapObject*>& objects);
	void updateVisibility();
	void applyValue();
};

class MOPGTextureProperty : public MOPGStringProperty
{
public:
	MOPGTextureProperty(int textype = 0, const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type getType() { return Type::Texture; }
	void openObjects(vector<MapObject*>& objects);

	// wxPGProperty overrides
	bool OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e);

private:
	int textype_;
};

class MOPGSPACTriggerProperty : public MOPGProperty, public wxEnumProperty
{
public:
	MOPGSPACTriggerProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type getType() { return Type::SPACTrigger; }
	void openObjects(vector<MapObject*>& objects);
	void updateVisibility();
	void applyValue();
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

	Type getType() { return Type::Id; }
	void openObjects(vector<MapObject*>& objects);

	// wxPGProperty overrides
	bool OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e);

private:
	IdType id_type_;
};

class MOPGSectorSpecialProperty : public MOPGIntProperty
{
public:
	MOPGSectorSpecialProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type getType() { return Type::SectorSpecial; }
	void openObjects(vector<MapObject*>& objects);

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int argFlags = 0) const;
	bool     OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e);
};
