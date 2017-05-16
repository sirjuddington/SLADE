
#ifndef __MOPG_PROPERTY_H__
#define __MOPG_PROPERTY_H__

#include "common.h"

#include "Game/Args.h"

class MapObject;
class MapObjectPropsPanel;
class UDMFProperty;
class MOPGProperty
{
protected:
	int						type;
	MapObjectPropsPanel*	parent;
	bool					noupdate;
	UDMFProperty*			udmf_prop;
	string					propname;

public:
	MOPGProperty() { noupdate = false; udmf_prop = NULL; }
	~MOPGProperty() {}

	enum
	{
		TYPE_BOOL = 0,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_STRING,
		TYPE_ASPECIAL,
		TYPE_SSPECIAL,
		TYPE_TTYPE,
		TYPE_LFLAG,
		TYPE_TFLAG,
		TYPE_ANGLE,
		TYPE_COLOUR,
		TYPE_TEXTURE,
		TYPE_SPAC,
		TYPE_ID,
	};

	string			getPropName() { return propname; }
	void			setParent(MapObjectPropsPanel* parent) { this->parent = parent; }
	virtual void	setUDMFProp(UDMFProperty* prop) { this->udmf_prop = prop; }

	virtual int		getType() = 0;
	virtual void	openObjects(vector<MapObject*>& objects) = 0;
	virtual void	updateVisibility() = 0;
	virtual void	applyValue() {}
	virtual void	resetValue();
};

class MOPGBoolProperty : public MOPGProperty, public wxBoolProperty
{
public:
	MOPGBoolProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	int		getType() { return TYPE_BOOL; }
	void	openObjects(vector<MapObject*>& objects);
	void	updateVisibility();
	void	applyValue();
};

class MOPGIntProperty : public MOPGProperty, public wxIntProperty
{
public:
	MOPGIntProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	int		getType() { return TYPE_INT; }
	void	openObjects(vector<MapObject*>& objects);
	void	updateVisibility();
	void	applyValue();
};

class MOPGFloatProperty : public MOPGProperty, public wxFloatProperty
{
public:
	MOPGFloatProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	int		getType() { return TYPE_FLOAT; }
	void	openObjects(vector<MapObject*>& objects);
	void	updateVisibility();
	void	applyValue();
};

class MOPGStringProperty : public MOPGProperty, public wxStringProperty
{
public:
	MOPGStringProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	void	setUDMFProp(UDMFProperty* prop);

	int		getType() { return TYPE_STRING; }
	void	openObjects(vector<MapObject*>& objects);
	void	updateVisibility();
	void	applyValue();
};

class MOPGIntWithArgsProperty : public MOPGIntProperty
{
protected:
	virtual const Game::ArgSpec& getArgspec() = 0;

public:
	MOPGIntWithArgsProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	void	applyValue();
	bool	hasArgs();
	void	updateArgs(wxPGProperty* args[5]);

	// wxPGProperty overrides
	void	OnSetValue();
};

class MOPGActionSpecialProperty : public MOPGIntWithArgsProperty
{
protected:
	const Game::ArgSpec& getArgspec();

public:
	MOPGActionSpecialProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL)
		: MOPGIntWithArgsProperty(label, name) {}

	int		getType() { return TYPE_ASPECIAL; }

	// wxPGProperty overrides
	wxString	ValueToString(wxVariant& value, int argFlags = 0) const;
	bool 		OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e);
};

class MOPGThingTypeProperty : public MOPGIntWithArgsProperty
{
protected:
	const Game::ArgSpec& getArgspec();

public:
	MOPGThingTypeProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL)
		: MOPGIntWithArgsProperty(label, name) {}

	int		getType() { return TYPE_TTYPE; }

	// wxPGProperty overrides
	wxString	ValueToString(wxVariant& value, int argFlags = 0) const;
	bool 		OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e);
};

class MOPGLineFlagProperty : public MOPGBoolProperty
{
private:
	int	index;

public:
	MOPGLineFlagProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL, int index = -1);

	int		getType() { return TYPE_LFLAG; }
	void	openObjects(vector<MapObject*>& objects);
	void	applyValue();
};

class MOPGThingFlagProperty : public MOPGBoolProperty
{
private:
	int	index;

public:
	MOPGThingFlagProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL, int index = -1);

	int		getType() { return TYPE_TFLAG; }
	void	openObjects(vector<MapObject*>& objects);
	void	applyValue();
};

class MOPGAngleProperty : public MOPGProperty, public wxEditEnumProperty
{
public:
	MOPGAngleProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	int		getType() { return TYPE_ANGLE; }
	void	openObjects(vector<MapObject*>& objects);
	void	updateVisibility();
	void	applyValue();

	// wxPGProperty overrides
	wxString	ValueToString(wxVariant& value, int argFlags = 0) const;
};

class MOPGColourProperty : public MOPGProperty, public wxColourProperty
{
public:
	MOPGColourProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	int		getType() { return TYPE_COLOUR; }
	void	openObjects(vector<MapObject*>& objects);
	void	updateVisibility();
	void	applyValue();
};

class MOPGTextureProperty : public MOPGStringProperty
{
private:
	int	textype;

public:
	MOPGTextureProperty(int textype = 0, const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	int		getType() { return TYPE_TEXTURE; }
	void	openObjects(vector<MapObject*>& objects);

	// wxPGProperty overrides
	bool 	OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e);
};

class MOPGSPACTriggerProperty : public MOPGProperty, public wxEnumProperty
{
public:
	MOPGSPACTriggerProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	int		getType() { return TYPE_SPAC; }
	void	openObjects(vector<MapObject*>& objects);
	void	updateVisibility();
	void	applyValue();
};

class MOPGTagProperty : public MOPGIntProperty
{
private:
	int	tagtype;

public:
	enum
	{
		TT_SECTORTAG,
		TT_LINEID,
		TT_THINGID
	};

	MOPGTagProperty(int tagtype, const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	int		getType() { return TYPE_ID; }
	void	openObjects(vector<MapObject*>& objects);

	// wxPGProperty overrides
	bool 	OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e);
};

class MOPGSectorSpecialProperty : public MOPGIntProperty
{
public:
	MOPGSectorSpecialProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	int		getType() { return TYPE_SSPECIAL; }
	void	openObjects(vector<MapObject*>& objects);

	// wxPGProperty overrides
	wxString	ValueToString(wxVariant& value, int argFlags = 0) const;
	bool		OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e);
};

#endif//__MOPG_PROPERTY_H__
