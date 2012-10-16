
#ifndef __UDMF_PROPERTY_H__
#define __UDMF_PROPERTY_H__

#include "Property.h"

class ParseTreeNode;
class UDMFProperty {
private:
	string				property;
	string				name;
	string				group;
	int					type;
	bool				flag;
	bool				trigger;
	bool				has_default;
	Property			default_value;
	vector<Property>	values;
	bool				show_always;

public:
	UDMFProperty();
	~UDMFProperty();

	string		getProperty() { return property; }
	string		getName() { return name; }
	string		getGroup() { return group; }
	int			getType() { return type; }
	Property	getDefaultValue() { return default_value; }
	bool		hasDefaultValue() { return has_default; }
	bool		isFlag() { return flag; }
	bool		isTrigger() { return trigger; }
	bool		showAlways() { return show_always; }

	void	parse(ParseTreeNode* node, string group);

	string	getStringRep();

	enum {
		TYPE_BOOL = 0,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_STRING,
		TYPE_COLOUR,
		TYPE_ASPECIAL,
		TYPE_SSPECIAL,
		TYPE_TTYPE,
		TYPE_ANGLE,
		TYPE_TEX_WALL,
		TYPE_TEX_FLAT,
	};
};

#endif//__UDMF_PROPERTY_H__
