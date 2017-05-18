
#ifndef __PROPERTY_H__
#define __PROPERTY_H__

// Define property types
#define PROP_BOOL	0
#define PROP_INT	1
#define PROP_FLOAT	2
#define PROP_STRING	3
#define PROP_FLAG	4	// The 'flag' property type mimics a boolean property that is always true
#define PROP_UINT	5

// Union for (most) property values
union prop_value { bool Boolean; int Integer; double Floating; unsigned Unsigned; };

class Property
{
private:
	uint8_t		type;
	prop_value	value;
	string		val_string;	// I *would* put this in the union but i'm not sure about using const char* there
	bool		has_value;

public:
	Property(uint8_t type = PROP_BOOL);	// Default property type is bool
	Property(const Property& copy);
	Property(bool value);
	Property(int value);
	Property(float value);
	Property(double value);
	Property(string value);
	Property(unsigned value);
	~Property();

	uint8_t		getType() const { return type; }
	bool		isType(uint8_t type) const { return this->type == type; }
	bool		hasValue() const { return has_value; }

	void	changeType(uint8_t newtype);
	void	setHasValue(bool hv) { has_value = hv; }

	inline operator bool () const { return getBoolValue(); }
	inline operator int () const { return getIntValue(); }
	inline operator float () const { return (float)getFloatValue(); }
	inline operator double () const { return getFloatValue(); }
	inline operator string () const { return getStringValue(); }
	inline operator unsigned () const { return getUnsignedValue(); }

	inline bool operator= (bool val) { setValue(val); return val; }
	inline int operator= (int val) { setValue(val); return val; }
	inline float operator= (float val) { setValue((double)val); return val; }
	inline double operator= (double val) { setValue(val); return val; }
	inline string operator= (string val) { setValue(val); return val; }
	inline unsigned operator= (unsigned val) { setValue(val); return val; }

	bool		getBoolValue(bool warn_wrong_type = false) const;
	int			getIntValue(bool warn_wrong_type = false) const;
	double		getFloatValue(bool warn_wrong_type = false) const;
	string		getStringValue(bool warn_wrong_type = false) const;
	unsigned	getUnsignedValue(bool warn_wrong_type = false) const;

	void	setValue(bool val);
	void	setValue(int val);
	void	setValue(double val);
	void	setValue(string val);
	void	setValue(unsigned val);

	string	typeString() const;
};

#endif//__PROPERTY_H__
