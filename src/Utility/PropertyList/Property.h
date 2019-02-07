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

	union Value {
		bool     Boolean;
		int      Integer;
		double   Floating;
		unsigned Unsigned;
	};

	Property(Type type = Type::Boolean); // Default property type is bool
	Property(const Property& copy) = default;
	Property(bool value);
	Property(int value);
	Property(float value);
	Property(double value);
	Property(const wxString& value);
	Property(unsigned value);
	~Property() = default;

	Type type() const { return type_; }
	bool isType(Type type) const { return this->type_ == type; }
	bool hasValue() const { return has_value_; }

	void changeType(Type newtype);
	void setHasValue(bool hv) { has_value_ = hv; }

	operator bool() const { return boolValue(); }
	operator int() const { return intValue(); }
	operator float() const { return (float)floatValue(); }
	operator double() const { return floatValue(); }
	operator wxString() const { return stringValue(); }
	operator unsigned() const { return unsignedValue(); }

	Property& operator=(bool val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(int val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(float val)
	{
		setValue(static_cast<double>(val));
		return *this;
	}

	Property& operator=(double val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(const wxString& val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(unsigned val)
	{
		setValue(val);
		return *this;
	}

	bool     boolValue(bool warn_wrong_type = false) const;
	int      intValue(bool warn_wrong_type = false) const;
	double   floatValue(bool warn_wrong_type = false) const;
	wxString stringValue(bool warn_wrong_type = false) const;
	unsigned unsignedValue(bool warn_wrong_type = false) const;

	void setValue(bool val);
	void setValue(int val);
	void setValue(double val);
	void setValue(const wxString& val);
	void setValue(unsigned val);

	wxString typeString() const;

private:
	Type     type_ = Type::Boolean;
	Value    value_;
	wxString val_string_; // I *would* put this in the union but i'm not sure about using const char* there
	bool     has_value_ = false;
};
