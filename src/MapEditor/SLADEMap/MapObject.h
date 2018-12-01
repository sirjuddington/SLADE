#pragma once

#ifdef __clang__
#pragma clang diagnostic ignored "-Wundefined-bool-conversion"
#endif

#include "MobjPropertyList.h"

class SLADEMap;

class MapObject
{
	friend class SLADEMap;

public:
	enum class Type
	{
		Object = 0,
		Vertex,
		Line,
		Side,
		Sector,
		Thing
	};

	enum class Point
	{
		Mid = 0,
		Within,
		Text
	};

	struct Backup
	{
		MobjPropertyList properties;
		MobjPropertyList props_internal;
		unsigned         id   = 0;
		Type             type = Type::Object;
	};

	MapObject(Type type = Type::Object, SLADEMap* parent = nullptr);
	virtual ~MapObject();
	bool operator<(const MapObject& right) const { return (index_ < right.index_); }
	bool operator>(const MapObject& right) const { return (index_ > right.index_); }

	Type      objType() const { return type_; }
	unsigned  index();
	SLADEMap* parentMap() const { return parent_map_; }
	bool      isFiltered() const { return filtered_; }
	long      modifiedTime() const { return modified_time_; }
	unsigned  objId() const { return obj_id_; }
	string    typeName();
	void      setModified();

	MobjPropertyList& props() { return properties_; }
	bool              hasProp(const string& key) { return properties_[key].hasValue(); }

	// Generic property modification
	virtual bool   boolProperty(const string& key);
	virtual int    intProperty(const string& key);
	virtual double floatProperty(const string& key);
	virtual string stringProperty(const string& key);
	virtual void   setBoolProperty(const string& key, bool value);
	virtual void   setIntProperty(const string& key, int value);
	virtual void   setFloatProperty(const string& key, double value);
	virtual void   setStringProperty(const string& key, const string& value);
	virtual bool   scriptCanModifyProp(const string& key) { return true; }

	virtual fpoint2_t getPoint(Point point) { return fpoint2_t(0, 0); }

	void filter(bool f = true) { filtered_ = f; }

	virtual void copy(MapObject* c);

	void    backupTo(Backup* backup);
	void    loadFromBackup(Backup* backup);
	Backup* backup(bool remove = false);

	virtual void writeBackup(Backup* backup) = 0;
	virtual void readBackup(Backup* backup)  = 0;

	static void resetIdCounter();
	static long propBackupTime();
	static void beginPropBackup(long current_time);
	static void endPropBackup();

	static bool multiBoolProperty(vector<MapObject*>& objects, string prop, bool& value);
	static bool multiIntProperty(vector<MapObject*>& objects, string prop, int& value);
	static bool multiFloatProperty(vector<MapObject*>& objects, string prop, double& value);
	static bool multiStringProperty(vector<MapObject*>& objects, string prop, string& value);

protected:
	unsigned         index_;
	SLADEMap*        parent_map_;
	MobjPropertyList properties_;
	bool             filtered_;
	long             modified_time_;
	unsigned         obj_id_;
	Backup*          obj_backup_;

private:
	Type type_;
};
