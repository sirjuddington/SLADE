#pragma once

#define KPM_CTRL 0x01
#define KPM_ALT 0x02
#define KPM_SHIFT 0x04

class Tokenizer;

struct Keypress
{
	std::string key;
	bool        alt;
	bool        ctrl;
	bool        shift;

	Keypress(std::string_view key, bool alt, bool ctrl, bool shift) :
		key{ key },
		alt{ alt },
		ctrl{ ctrl },
		shift{ shift }
	{
	}

	Keypress(std::string_view key = "", int modifiers = 0)
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

	std::string asString() const;
};

class KeyBind
{
public:
	KeyBind(std::string_view name) : name_{ name } {}
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

	void        clear() { keys_.clear(); }
	void        addKey(std::string_view key, bool alt = false, bool ctrl = false, bool shift = false);
	std::string name() const { return name_; }
	std::string group() const { return group_; }
	std::string description() const { return description_; }
	std::string keysAsString();

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
	static KeyBind&            bind(std::string_view name);
	static vector<std::string> bindsForKey(Keypress key);
	static bool                isPressed(std::string_view name);
	static bool                addBind(
					   std::string_view name,
					   const Keypress&  key,
					   std::string_view desc         = "",
					   std::string_view group        = "",
					   bool             ignore_shift = false,
					   int              priority     = -1);
	static std::string keyName(int key);
	static std::string mbName(int button);
	static bool        keyPressed(Keypress key);
	static bool        keyReleased(std::string_view key);
	static Keypress    asKeyPress(int keycode, int modifiers);
	static void        allKeyBinds(vector<KeyBind*>& list);
	static void        releaseAll();
	static void        pressBind(std::string_view name);

	static void        initBinds();
	static std::string writeBinds();
	static bool        readBinds(Tokenizer& tz);
	static void        updateSortedBindsList();

private:
	std::string      name_;
	vector<Keypress> keys_;
	vector<Keypress> defaults_;
	bool             pressed_ = false;
	std::string      description_;
	std::string      group_;
	bool             ignore_shift_ = false;
	int              priority_     = 0;
};


class KeyBindHandler
{
public:
	KeyBindHandler();
	virtual ~KeyBindHandler();

	virtual void onKeyBindPress(std::string_view name) {}
	virtual void onKeyBindRelease(std::string_view name) {}
};
