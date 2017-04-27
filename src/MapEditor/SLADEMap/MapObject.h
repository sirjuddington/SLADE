
#ifndef __MAP_OBJECT_H__
#define __MAP_OBJECT_H__

#ifdef __clang__
#pragma clang diagnostic ignored "-Wundefined-bool-conversion"
#endif

#include "MobjPropertyList.h"

class SLADEMap;

enum
{
	MOBJ_UNKNOWN = 0,
	MOBJ_VERTEX,
	MOBJ_LINE,
	MOBJ_SIDE,
	MOBJ_SECTOR,
	MOBJ_THING,

	MOBJ_POINT_MID = 0,
	MOBJ_POINT_WITHIN,
	MOBJ_POINT_TEXT
};

struct mobj_backup_t
{
	MobjPropertyList	properties;
	MobjPropertyList	props_internal;
	unsigned			id;
	uint8_t				type;

	mobj_backup_t() { id = 0; type = 0; }
};

class MapObject
{
	friend class SLADEMap;
private:
	uint8_t			type;

protected:
	unsigned			index;
	SLADEMap*			parent_map;
	MobjPropertyList	properties;
	bool				filtered;
	long				modified_time;
	unsigned			id;
	mobj_backup_t*		obj_backup;

public:
	MapObject(int type = MOBJ_UNKNOWN, SLADEMap* parent = NULL);
	virtual ~MapObject();
	bool operator< (const MapObject& right) const { return (index < right.index); }
	bool operator> (const MapObject& right) const { return (index > right.index); }

	uint8_t		getObjType() { return type; }
	unsigned	getIndex();
	SLADEMap*	getParentMap() { return parent_map; }
	bool		isFiltered() { return filtered; }
	long		modifiedTime() { return modified_time; }
	unsigned	getId() { return id; }
	string		getTypeName();
	void		setModified();

	MobjPropertyList&	props()				{ return properties; }
	bool				hasProp(string key)	{ return properties[key].hasValue(); }

	// Generic property modification
	virtual bool	boolProperty(string key);
	virtual int		intProperty(string key);
	virtual double	floatProperty(string key);
	virtual string	stringProperty(string key);
	virtual void	setBoolProperty(string key, bool value);
	virtual void	setIntProperty(string key, int value);
	virtual void	setFloatProperty(string key, double value);
	virtual void	setStringProperty(string key, string value);

	virtual fpoint2_t	getPoint(uint8_t point) { return fpoint2_t(0,0); }

	void	filter(bool f = true) { filtered = f; }

	virtual void	copy(MapObject* c);

	void			backup(mobj_backup_t* backup);
	void			loadFromBackup(mobj_backup_t* backup);
	mobj_backup_t*	getBackup(bool remove = false);

	virtual void writeBackup(mobj_backup_t* backup) = 0;
	virtual void readBackup(mobj_backup_t* backup) = 0;

	static void resetIdCounter();
	static long propBackupTime();
	static void beginPropBackup(long current_time);
	static void endPropBackup();
	
	static bool	multiBoolProperty(vector<MapObject*>& objects, string prop, bool& value);
	static bool multiIntProperty(vector<MapObject*>& objects, string prop, int& value);
	static bool multiFloatProperty(vector<MapObject*>& objects, string prop, double& value);
	static bool multiStringProperty(vector<MapObject*>& objects, string prop, string& value);
};

#endif//__MAP_OBJECT_H__
