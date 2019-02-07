#pragma once

#define KPM_CTRL 0x01
#define KPM_ALT 0x02
#define KPM_SHIFT 0x04

class Tokenizer;

struct Keypress
{
	wxString key;
	bool     alt;
	bool     ctrl;
	bool     shift;

	Keypress(const wxString& key, bool alt, bool ctrl, bool shift) :
		key{ key },
		alt{ alt },
		ctrl{ ctrl },
		shift{ shift }
	{
	}

	Keypress(const wxString& key = "", int modifiers = 0)
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

	wxString asString() const
	{
		if (key.IsEmpty())
			return "";

		wxString ret = "";
		if (ctrl)
			ret += "Ctrl+";
		if (alt)
			ret += "Alt+";
		if (shift)
			ret += "Shift+";

		wxString keyname = key;
		keyname.Replace("_", " ");
		keyname = keyname.Capitalize();
		ret += keyname;

		return ret;
	}
};

class KeyBind
{
public:
	KeyBind(const wxString& name) : name_{ name } {}
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

	void     clear() { keys_.clear(); }
	void     addKey(const wxString& key, bool alt = false, bool ctrl = false, bool shift = false);
	wxString name() const { return name_; }
	wxString group() const { return group_; }
	wxString description() const { return description_; }
	wxString keysAsString();

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
	static KeyBind&      bind(const wxString& name);
	static wxArrayString bindsForKey(Keypress key);
	static bool          isPressed(const wxString& name);
	static bool          addBind(
				 const wxString& name,
				 const Keypress& key,
				 const wxString& desc         = "",
				 const wxString& group        = "",
				 bool            ignore_shift = false,
				 int             priority     = -1);
	static wxString keyName(int key);
	static wxString mbName(int button);
	static bool     keyPressed(Keypress key);
	static bool     keyReleased(const wxString& key);
	static Keypress asKeyPress(int keycode, int modifiers);
	static void     allKeyBinds(vector<KeyBind*>& list);
	static void     releaseAll();
	static void     pressBind(const wxString& name);

	static void     initBinds();
	static wxString writeBinds();
	static bool     readBinds(Tokenizer& tz);
	static void     updateSortedBindsList();

private:
	wxString         name_;
	vector<Keypress> keys_;
	vector<Keypress> defaults_;
	bool             pressed_ = false;
	wxString         description_;
	wxString         group_;
	bool             ignore_shift_ = false;
	int              priority_     = 0;
};


class KeyBindHandler
{
public:
	KeyBindHandler();
	virtual ~KeyBindHandler();

	virtual void onKeyBindPress(const wxString& name) {}
	virtual void onKeyBindRelease(const wxString& name) {}
};
