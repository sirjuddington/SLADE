#pragma once

class wxMenu;
class wxAuiToolBar;
class wxToolBar;

namespace slade
{
class ParseTreeNode;
struct CBoolCVar;

class SAction
{
public:
	// Enum for action types
	enum class Type
	{
		Normal,
		Check,
		Radio,
	};

	SAction(
		string_view id,
		string_view text,
		string_view icon        = "",
		string_view helptext    = "",
		string_view shortcut    = "",
		Type        type        = Type::Normal,
		int         radio_group = -1,
		int         reserve_ids = 1);
	~SAction() = default;

	string     id() const { return id_; }
	int        wxId() const { return wx_id_; }
	string     text() const { return text_; }
	string     iconName() const { return icon_; }
	string     helpText() const { return helptext_; }
	string     shortcut() const { return shortcut_; }
	string     shortcutText() const;
	Type       type() const { return type_; }
	bool       isChecked() const { return checked_; }
	bool       isRadio() const { return type_ == Type::Radio; }
	bool       isWxId(int id) const { return id >= wx_id_ && id < wx_id_ + reserved_ids_; }
	CBoolCVar* linkedCVar() const { return linked_cvar_; }

	void setChecked(bool checked = true);
	void toggle() { setChecked(!checked_); }
	void initWxId();

	bool addToMenu(
		wxMenu*     menu,
		int         show_shortcut = 2, // 0 = don't show, 1 = always show, 2 = auto (show if ctrl or alt)
		string_view text_override = "NO",
		string_view icon_override = "NO",
		int         wx_id_offset  = 0);

	// Static functions
	static void                  setBaseWxId(int id) { cur_id_ = id; }
	static bool                  initActions();
	static int                   newGroup();
	static SAction*              fromId(string_view id);
	static SAction*              fromWxId(int wx_id);
	static void                  add(SAction* action);
	static int                   nextWxId();
	static const vector<string>& history();

private:
	// The id associated with this action - to keep things consistent, it should be of the format xxxx_*,
	// where xxxx is some 4 letter identifier for the SActionHandler that handles this action
	string id_;

	int        wx_id_;
	int        reserved_ids_; // Can reserve a range of wx ids
	string     text_;
	string     icon_;
	string     helptext_;
	string     shortcut_;
	Type       type_;
	int        group_;
	bool       checked_;
	string     keybind_;
	CBoolCVar* linked_cvar_;

	// Internal functions
	bool parse(const ParseTreeNode* node);

	// Static functions
	static SAction* invalidAction();

	// Static variables
	static int              n_groups_;
	static int              cur_id_;
	static vector<SAction*> actions_;
	static SAction*         action_invalid_;
};

// Basic 'interface' class for classes that handle SActions (yay multiple inheritance)
class SActionHandler
{
public:
	SActionHandler();
	virtual ~SActionHandler();

	static void setWxIdOffset(int offset) { wx_id_offset_ = offset; }
	static bool doAction(string_view id);

protected:
	static int wx_id_offset_;

	virtual bool handleAction(string_view id) { return false; }

private:
	static vector<SActionHandler*> action_handlers_;
};
} // namespace slade
