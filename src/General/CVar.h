
#pragma once

// CVar classes, a lot of ideas taken from the ZDoom source

class CVar
{
public:
	enum class Type
	{
		Boolean,
		Integer,
		Float,
		String,
	};

	union Value {
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

	uint16_t    flags = 0;
	Type        type  = Type::Integer;
	std::string name;
	CVar*       next = nullptr;

	CVar()          = default;
	virtual ~CVar() = default;

	virtual Value getValue()
	{
		Value val;
		val.Int = 0;
		return val;
	}

	// Static functions
	static std::string writeAll();
	static void        set(const std::string& cvar_name, const std::string& value);
	static CVar*       get(const std::string& cvar_name);
	static void        putList(vector<std::string>& list);
};

class CIntCVar : public CVar
{
public:
	int value;

	CIntCVar(const std::string& NAME, int defval, uint16_t FLAGS);
	~CIntCVar() = default;

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

class CBoolCVar : public CVar
{
public:
	bool value;

	CBoolCVar(const std::string& NAME, bool defval, uint16_t FLAGS);
	~CBoolCVar() = default;

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

class CFloatCVar : public CVar
{
public:
	double value;

	CFloatCVar(const std::string& NAME, double defval, uint16_t FLAGS);
	~CFloatCVar() = default;

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

class CStringCVar : public CVar
{
public:
	std::string value;

	CStringCVar(const std::string& NAME, const std::string& defval, uint16_t FLAGS);
	~CStringCVar() = default;

	bool empty() const { return value.empty(); }

				operator std::string() const { return value; }
	std::string operator*() const { return value; }
				operator std::string_view() const { return value; }
				operator wxString() const { return value; }

	CStringCVar& operator=(std::string_view val)
	{
		value = val;
		return *this;
	}
};

#define CVAR(type, name, val, flags) C##type##CVar name(#name, val, flags);

#define EXTERN_CVAR(type, name) extern C##type##CVar name;
