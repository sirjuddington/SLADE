
#pragma once

// CVar classes, a lot of ideas taken from the ZDoom source

namespace slade
{
struct CVar
{
	enum class Type
	{
		Boolean,
		Integer,
		Float,
		String,
	};

	union Value
	{
		int         Int;
		bool        Bool;
		double      Float;
		const char* String;
	};

	enum Flag
	{
		Save   = 1, // set if cvar is saved to config file
		Secret = 2, // set if cvar is not listed when cvarlist command called
		Locked = 4, // set if cvar cannot be changed by the user during runtime
	};

	uint16_t flags = 0;
	Type     type  = Type::Integer;
	string   name;
	CVar*    next = nullptr;

	CVar() = default;
	CVar(Type type, uint16_t flags, string_view name) : flags{ flags }, type{ type }, name{ name } {}
	virtual ~CVar() = default;

	virtual Value getValue()
	{
		Value val;
		val.Int = 0;
		return val;
	}

	// Static functions
	static void  writeAll(Json& json);
	static void  set(const string& cvar_name, const string& value);
	static CVar* get(const string& cvar_name);
	static void  putList(vector<string>& list);
};

struct CIntCVar : CVar
{
	int value;

	CIntCVar(string_view name, int defval, uint16_t flags);
	~CIntCVar() override = default;

	// Operators so the cvar name can be used like a normal variable
	operator int() const { return value; }
	int operator*() const { return value; }

	int operator=(int val)
	{
		value = val;
		return val;
	}

	Value getValue() override
	{
		Value val;
		val.Int = value;
		return val;
	}
};

struct CBoolCVar : CVar
{
	bool value;

	CBoolCVar(string_view name, bool defval, uint16_t flags);
	~CBoolCVar() override = default;

	operator bool() const { return value; }
	bool operator*() const { return value; }

	bool operator=(bool val)
	{
		value = val;
		return val;
	}

	Value getValue() override
	{
		Value val;
		val.Bool = value;
		return val;
	}
};

struct CFloatCVar : CVar
{
	double value;

	CFloatCVar(string_view name, double defval, uint16_t flags);
	~CFloatCVar() override = default;

	operator double() const { return value; }
	double operator*() const { return value; }

	double operator=(double val)
	{
		value = val;
		return val;
	}

	Value getValue() override
	{
		Value val;
		val.Float = value;
		return val;
	}
};

struct CStringCVar : CVar
{
	string value;

	CStringCVar(string_view name, string_view defval, uint16_t flags);
	~CStringCVar() override = default;

	bool empty() const { return value.empty(); }

	operator string() const { return value; }
	string operator*() const { return value; }
	operator string_view() const { return value; }
	operator wxString() const { return wxString::FromUTF8(value); }

	CStringCVar& operator=(string_view val)
	{
		value = val;
		return *this;
	}
};
} // namespace slade

#define CVAR(type, name, val, flags) C##type##CVar name(#name, val, flags);

#define EXTERN_CVAR(type, name) extern C##type##CVar name;
