#pragma once

#ifdef __clang__
#pragma clang diagnostic ignored "-Wundefined-bool-conversion"
#endif

#include "Utility/PropertyList.h"
#include <array>

namespace slade
{
class MapObject
{
	friend class SLADEMap;
	friend class MapObjectCollection;

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
		PropertyList properties;
		PropertyList props_internal;
		unsigned     id   = 0;
		Type         type = Type::Object;
	};

	typedef std::array<int, 5> ArgSet;

	MapObject(Type type = Type::Object, SLADEMap* parent = nullptr);
	virtual ~MapObject() = default;

	virtual void readUDMF(ParseTreeNode* def) {}

	bool operator<(const MapObject& right) const { return (index_ < right.index_); }
	bool operator>(const MapObject& right) const { return (index_ > right.index_); }

	Type      objType() const { return type_; }
	unsigned  index() const;
	SLADEMap* parentMap() const { return parent_map_; }
	bool      isFiltered() const { return filtered_; }
	long      modifiedTime() const { return modified_time_; }
	unsigned  objId() const { return obj_id_; }
	string    typeName() const;
	void      setModified();
	void      setIndex(unsigned index) { index_ = index; }

	PropertyList& props() { return properties_; }
	bool          hasProp(string_view key) const { return properties_.contains(key); }

	// Generic property modification
	virtual bool   boolProperty(string_view key);
	virtual int    intProperty(string_view key);
	virtual double floatProperty(string_view key);
	virtual string stringProperty(string_view key);
	virtual void   setBoolProperty(string_view key, bool value);
	virtual void   setIntProperty(string_view key, int value);
	virtual void   setFloatProperty(string_view key, double value);
	virtual void   setStringProperty(string_view key, string_view value);
	virtual bool   scriptCanModifyProp(string_view key) { return true; }

	virtual Vec2d getPoint(Point point) { return { 0, 0 }; }

	void filter(bool f = true) { filtered_ = f; }

	virtual void copy(MapObject* c);

	void    backupTo(Backup* backup);
	void    loadFromBackup(Backup* backup);
	Backup* backup(bool remove = false);

	virtual void writeBackup(Backup* backup) = 0;
	virtual void readBackup(Backup* backup)  = 0;

	virtual void writeUDMF(string& def) {}

	static long propBackupTime();
	static void beginPropBackup(long current_time);
	static void endPropBackup();

	static bool multiBoolProperty(const vector<MapObject*>& objects, string_view prop, bool& value);
	static bool multiIntProperty(const vector<MapObject*>& objects, string_view prop, int& value);
	static bool multiFloatProperty(const vector<MapObject*>& objects, string_view prop, double& value);
	static bool multiStringProperty(const vector<MapObject*>& objects, string_view prop, string& value);

protected:
	unsigned           index_      = 0;
	SLADEMap*          parent_map_ = nullptr;
	PropertyList       properties_;
	bool               filtered_      = false;
	long               modified_time_ = 0;
	unsigned           obj_id_        = 0;
	unique_ptr<Backup> obj_backup_;

private:
	Type type_ = Type::Object;
};
} // namespace slade
