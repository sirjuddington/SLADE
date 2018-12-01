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
		if (priority_ == r.priority_)
			return name_ < r.name_;
		else
			return priority_ < r.priority_;
	}
	inline bool operator<(const KeyBind r) const
	{
		if (priority_ == r.priority_)
			return name_ > r.name_;
		else
			return priority_ > r.priority_;
	}

	void   clear() { keys_.clear(); }
	void   addKey(string key, bool alt = false, bool ctrl = false, bool shift = false);
	string name() { return name_; }
	string group() { return group_; }
	string description() { return description_; }
	string keysAsString();

	int      nKeys() { return keys_.size(); }
	Keypress key(unsigned index)
	{
		if (index >= keys_.size())
			return Keypress();
		else
			return keys_[index];
	}
	int      nDefaults() { return defaults_.size(); }
	Keypress defaultKey(unsigned index) { return defaults_[index]; }

	// Static functions
	static KeyBind&      bind(string name);
	static wxArrayString bindsForKey(Keypress key);
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
	string           name_;
	vector<Keypress> keys_;
	vector<Keypress> defaults_;
	bool             pressed_;
	string           description_;
	string           group_;
	bool             ignore_shift_;
	int              priority_;
};


class KeyBindHandler
{
public:
	KeyBindHandler();
	virtual ~KeyBindHandler();

	virtual void onKeyBindPress(string name) {}
	virtual void onKeyBindRelease(string name) {}
};
