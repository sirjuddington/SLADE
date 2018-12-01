
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

	uint16_t flags;
	Type     type;
	string   name;
	CVar*    next;

	CVar() { next = nullptr; }
	virtual ~CVar() {}

	virtual Value GetValue()
	{
		Value val;
		val.Int = 0;
		return val;
	}

	// Static functions
	static void  saveToFile(wxFile& file);
	static void  set(string cvar_name, string value);
	static CVar* get(string cvar_name);
	static void  putList(vector<string>& list);
};

class CIntCVar : public CVar
{
public:
	int value;

	CIntCVar(string NAME, int defval, uint16_t FLAGS);
	~CIntCVar() {}

	// Operators so the cvar name can be used like a normal variable
	inline     operator int() const { return value; }
	inline int operator*() const { return value; }
	inline int operator=(int val)
	{
		value = val;
		return val;
	}

	Value GetValue()
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

	CBoolCVar(string NAME, bool defval, uint16_t FLAGS);
	~CBoolCVar() {}

	inline      operator bool() const { return value; }
	inline bool operator*() const { return value; }
	inline bool operator=(bool val)
	{
		value = val;
		return val;
	}

	Value GetValue()
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

	CFloatCVar(string NAME, double defval, uint16_t FLAGS);
	~CFloatCVar() {}

	inline        operator double() const { return value; }
	inline double operator*() const { return value; }
	inline double operator=(double val)
	{
		value = val;
		return val;
	}

	Value GetValue()
	{
		Value val;
		val.Float = value;
		return val;
	}
};

class CStringCVar : public CVar
{
public:
	string value;

	CStringCVar(string NAME, string defval, uint16_t FLAGS);
	~CStringCVar() {}

	inline        operator string() const { return value; }
	inline string operator*() const { return value; }
	inline string operator=(string val)
	{
		value = val;
		return val;
	}
};

#define CVAR(type, name, val, flags) C##type##CVar name(#name, val, flags);

#define EXTERN_CVAR(type, name) extern C##type##CVar name;
