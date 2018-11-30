#pragma once

class Property
{
public:
	enum class Type
	{
		Bool,
		Int,
		Float,
		String,
		Flag, // The 'flag' property type mimics a boolean property that is always true
		UInt
	};

	union Value {
		bool     Boolean;
		int      Integer;
		double   Floating;
		unsigned Unsigned;
	};

	Property(Type type = Type::Bool); // Default property type is bool
	Property(const Property& copy);
	Property(bool value);
	Property(int value);
	Property(float value);
	Property(double value);
	Property(string value);
	Property(unsigned value);
	~Property();

	Type getType() const { return type_; }
	bool isType(Type type) const { return this->type_ == type; }
	bool hasValue() const { return has_value_; }

	void changeType(Type newtype);
	void setHasValue(bool hv) { has_value_ = hv; }

	inline operator bool() const { return getBoolValue(); }
	inline operator int() const { return getIntValue(); }
	inline operator float() const { return (float)getFloatValue(); }
	inline operator double() const { return getFloatValue(); }
	inline operator string() const { return getStringValue(); }
	inline operator unsigned() const { return getUnsignedValue(); }

	inline bool operator=(bool val)
	{
		setValue(val);
		return val;
	}
	inline int operator=(int val)
	{
		setValue(val);
		return val;
	}
	inline float operator=(float val)
	{
		setValue((double)val);
		return val;
	}
	inline double operator=(double val)
	{
		setValue(val);
		return val;
	}
	inline string operator=(string val)
	{
		setValue(val);
		return val;
	}
	inline unsigned operator=(unsigned val)
	{
		setValue(val);
		return val;
	}

	bool     getBoolValue(bool warn_wrong_type = false) const;
	int      getIntValue(bool warn_wrong_type = false) const;
	double   getFloatValue(bool warn_wrong_type = false) const;
	string   getStringValue(bool warn_wrong_type = false) const;
	unsigned getUnsignedValue(bool warn_wrong_type = false) const;

	void setValue(bool val);
	void setValue(int val);
	void setValue(double val);
	void setValue(string val);
	void setValue(unsigned val);

	string typeString() const;

private:
	Type   type_;
	Value  value_;
	string val_string_; // I *would* put this in the union but i'm not sure about using const char* there
	bool   has_value_;
};
