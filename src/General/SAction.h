#pragma once

class ParseTreeNode;
class wxMenu;
class wxAuiToolBar;
class wxToolBar;
class CBoolCVar;

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
		const wxString& id,
		const wxString& text,
		const wxString& icon        = "",
		const wxString& helptext    = "",
		const wxString& shortcut    = "",
		Type            type        = Type::Normal,
		int             radio_group = -1,
		int             reserve_ids = 1);
	~SAction() = default;

	wxString id() const { return id_; }
	int      wxId() const { return wx_id_; }
	wxString text() const { return text_; }
	wxString iconName() const { return icon_; }
	wxString helpText() const { return helptext_; }
	wxString shortcut() const { return shortcut_; }
	wxString shortcutText() const;
	Type     type() const { return type_; }
	bool     isChecked() const { return checked_; }
	bool     isRadio() const { return type_ == Type::Radio; }
	bool     isWxId(int id) const { return id >= wx_id_ && id < wx_id_ + reserved_ids_; }
	void     setChecked(bool checked = true);
	void     toggle() { setChecked(!checked_); }

	bool addToMenu(
		wxMenu*         menu,
		const wxString& text_override = "NO",
		const wxString& icon_override = "NO",
		int             wx_id_offset  = 0);
	bool addToMenu(
		wxMenu*         menu,
		bool            show_shortcut,
		const wxString& text_override = "NO",
		const wxString& icon_override = "NO",
		int             wx_id_offset  = 0);
	bool addToToolbar(wxAuiToolBar* toolbar, const wxString& icon_override = "NO", int wx_id_offset = 0) const;
	bool addToToolbar(wxToolBar* toolbar, const wxString& icon_override = "NO", int wx_id_offset = 0) const;

	// Static functions
	static void     initWxId(int id) { cur_id_ = id; }
	static bool     initActions();
	static int      newGroup();
	static SAction* fromId(const wxString& id);
	static SAction* fromWxId(int wx_id);
	static void     add(SAction* action);

private:
	// The id associated with this action - to keep things consistent, it should be of the format xxxx_*,
	// where xxxx is some 4 letter identifier for the SActionHandler that handles this action
	wxString id_;

	int        wx_id_;
	int        reserved_ids_; // Can reserve a range of wx ids
	wxString   text_;
	wxString   icon_;
	wxString   helptext_;
	wxString   shortcut_;
	Type       type_;
	int        group_;
	bool       checked_;
	wxString   keybind_;
	CBoolCVar* linked_cvar_;

	// Internal functions
	bool parse(ParseTreeNode* node);

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
	static bool doAction(const wxString& id);

protected:
	static int wx_id_offset_;

	virtual bool handleAction(const wxString& id) { return false; }

private:
	static vector<SActionHandler*> action_handlers_;
};
