
#ifndef __MAP_OBJECT_H__
#define __MAP_OBJECT_H__

#include "PropertyList.h"

class SLADEMap;

enum {
	MOBJ_UNKNOWN = 0,
	MOBJ_VERTEX,
	MOBJ_LINE,
	MOBJ_SIDE,
	MOBJ_SECTOR,
	MOBJ_THING,
};

class MapObject {
friend class SLADEMap;
private:
	int			type;

protected:
	unsigned		index;
	SLADEMap*		parent_map;
	PropertyList	properties;
	bool			filtered;
	long			modified_time;

public:
	MapObject(int type = MOBJ_UNKNOWN, SLADEMap* parent = NULL);
	~MapObject();

	int			getObjType() { return type; }
	unsigned	getIndex();
	SLADEMap*	getParentMap() { return parent_map; }
	bool		isFiltered() { return filtered; }
	long		modifiedTime() { return modified_time; }

	PropertyList&	props()				{ return properties; }
	//Property&		prop(string key)	{ return properties[key]; }
	bool			hasProp(string key)	{ return properties.propertyExists(key); }

	// Generic property modification
	virtual bool	boolProperty(string key);
	virtual int		intProperty(string key);
	virtual double	floatProperty(string key);
	virtual string	stringProperty(string key);
	virtual void	setBoolProperty(string key, bool value);
	virtual void	setIntProperty(string key, int value);
	virtual void	setFloatProperty(string key, double value);
	virtual void	setStringProperty(string key, string value);

	virtual fpoint2_t	midPoint() { return fpoint2_t(0,0); }
	virtual fpoint2_t	textPoint() { return fpoint2_t(0,0); }

	void	filter(bool f = true) { filtered = f; }

	virtual void	copy(MapObject* c);
};

#endif//__MAP_OBJECT_H__
