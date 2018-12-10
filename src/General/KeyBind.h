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

	Keypress(const string& key, bool alt, bool ctrl, bool shift) : key{ key }, alt{ alt }, ctrl{ ctrl }, shift{ shift }
	{
	}

	Keypress(const string& key = "", int modifiers = 0)
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

	string asString() const
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
	KeyBind(const string& name) : name_{ name } {}
	~KeyBind() = default;

	// Operators
	bool operator>(const KeyBind r) const
	{
		if (priority_ == r.priority_)
			return name_ < r.name_;
		else
			return priority_ < r.priority_;
	}

	bool operator<(const KeyBind r) const
	{
		if (priority_ == r.priority_)
			return name_ > r.name_;
		else
			return priority_ > r.priority_;
	}

	void   clear() { keys_.clear(); }
	void   addKey(const string& key, bool alt = false, bool ctrl = false, bool shift = false);
	string name() const { return name_; }
	string group() const { return group_; }
	string description() const { return description_; }
	string keysAsString();

	int      nKeys() const { return keys_.size(); }
	Keypress key(unsigned index)
	{
		if (index >= keys_.size())
			return Keypress();
		else
			return keys_[index];
	}
	int      nDefaults() const { return defaults_.size(); }
	Keypress defaultKey(unsigned index) { return defaults_[index]; }

	// Static functions
	static KeyBind&      bind(const string& name);
	static wxArrayString bindsForKey(Keypress key);
	static bool          isPressed(const string& name);
	static bool          addBind(
				 const string&   name,
				 const Keypress& key,
				 const string&   desc         = "",
				 const string&   group        = "",
				 bool            ignore_shift = false,
				 int             priority     = -1);
	static string   keyName(int key);
	static string   mbName(int button);
	static bool     keyPressed(Keypress key);
	static bool     keyReleased(const string& key);
	static Keypress asKeyPress(int keycode, int modifiers);
	static void     allKeyBinds(vector<KeyBind*>& list);
	static void     releaseAll();
	static void     pressBind(const string& name);

	static void   initBinds();
	static string writeBinds();
	static bool   readBinds(Tokenizer& tz);
	static void   updateSortedBindsList();

private:
	string           name_;
	vector<Keypress> keys_;
	vector<Keypress> defaults_;
	bool             pressed_ = false;
	string           description_;
	string           group_;
	bool             ignore_shift_ = false;
	int              priority_     = 0;
};


class KeyBindHandler
{
public:
	KeyBindHandler();
	virtual ~KeyBindHandler();

	virtual void onKeyBindPress(const string& name) {}
	virtual void onKeyBindRelease(const string& name) {}
};
