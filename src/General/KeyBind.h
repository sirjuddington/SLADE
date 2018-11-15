#pragma once

#define KPM_CTRL 0x01
#define KPM_ALT 0x02
#define KPM_SHIFT 0x04

class Tokenizer;

struct Keypress
{
	string key;
	bool   alt;
	bool   ctrl;
	bool   shift;

	Keypress(string key, bool alt, bool ctrl, bool shift)
	{
		this->key   = key;
		this->alt   = alt;
		this->ctrl  = ctrl;
		this->shift = shift;
	}

	Keypress(string key = "", int modifiers = 0)
	{
		this->key = key;
		ctrl = alt = shift = false;
		if (modifiers & KPM_CTRL)
			ctrl = true;
		if (modifiers & KPM_ALT)
			alt = true;
		if (modifiers & KPM_SHIFT)
			shift = true;
	}

	string asString()
	{
		if (key.IsEmpty())
			return "";

		string ret = "";
		if (ctrl)
			ret += "Ctrl+";
		if (alt)
			ret += "Alt+";
		if (shift)
			ret += "Shift+";

		string keyname = key;
		keyname.Replace("_", " ");
		keyname = keyname.Capitalize();
		ret += keyname;

		return ret;
	}
};

class KeyBind
{
public:
	KeyBind(string name);
	~KeyBind();

	// Operators
	inline bool operator>(const KeyBind r) const
	{
		if (priority == r.priority)
			return name < r.name;
		else
			return priority < r.priority;
	}
	inline bool operator<(const KeyBind r) const
	{
		if (priority == r.priority)
			return name > r.name;
		else
			return priority > r.priority;
	}

	void   clear() { keys.clear(); }
	void   addKey(string key, bool alt = false, bool ctrl = false, bool shift = false);
	string getName() { return name; }
	string getGroup() { return group; }
	string getDescription() { return description; }
	string keysAsString();

	int      nKeys() { return keys.size(); }
	Keypress getKey(unsigned index)
	{
		if (index >= keys.size())
			return Keypress();
		else
			return keys[index];
	}
	int      nDefaults() { return defaults.size(); }
	Keypress getDefault(unsigned index) { return defaults[index]; }

	// Static functions
	static KeyBind&      getBind(string name);
	static wxArrayString getBinds(Keypress key);
	static bool          isPressed(string name);
	static bool          addBind(
				 string   name,
				 Keypress key,
				 string   desc         = "",
				 string   group        = "",
				 bool     ignore_shift = false,
				 int      priority     = -1);
	static string   keyName(int key);
	static string   mbName(int button);
	static bool     keyPressed(Keypress key);
	static bool     keyReleased(string key);
	static Keypress asKeyPress(int keycode, int modifiers);
	static void     allKeyBinds(vector<KeyBind*>& list);
	static void     releaseAll();
	static void     pressBind(string name);

	static void   initBinds();
	static string writeBinds();
	static bool   readBinds(Tokenizer& tz);
	static void   updateSortedBindsList();

private:
	string           name;
	vector<Keypress> keys;
	vector<Keypress> defaults;
	bool             pressed;
	string           description;
	string           group;
	bool             ignore_shift;
	int              priority;
};


class KeyBindHandler
{
public:
	KeyBindHandler();
	virtual ~KeyBindHandler();

	virtual void onKeyBindPress(string name) {}
	virtual void onKeyBindRelease(string name) {}
};
