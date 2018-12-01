#pragma once

class Property
{
public:
	enum class Type
	{
		Boolean,
		Int,
		Float,
		String,
		Flag, // The 'flag' property type mimics a boolean property that is always true
		UInt
	};

	union Value
	{
		bool     Boolean;
		int      Integer;
		double   Floating;
		unsigned Unsigned;
	};

	Property(Type type = Type::Boolean); // Default property type is bool
	Property(const Property& copy);
	Property(bool value);
	Property(int value);
	Property(float value);
	Property(double value);
	Property(string value);
	Property(unsigned value);
	~Property();

	Type type() const { return type_; }
	bool isType(Type type) const { return this->type_ == type; }
	bool hasValue() const { return has_value_; }

	void changeType(Type newtype);
	void setHasValue(bool hv) { has_value_ = hv; }

	inline operator bool() const { return boolValue(); }
	inline operator int() const { return intValue(); }
	inline operator float() const { return (float)floatValue(); }
	inline operator double() const { return floatValue(); }
	inline operator string() const { return stringValue(); }
	inline operator unsigned() const { return unsignedValue(); }

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

	bool     boolValue(bool warn_wrong_type = false) const;
	int      intValue(bool warn_wrong_type = false) const;
	double   floatValue(bool warn_wrong_type = false) const;
	string   stringValue(bool warn_wrong_type = false) const;
	unsigned unsignedValue(bool warn_wrong_type = false) const;

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
