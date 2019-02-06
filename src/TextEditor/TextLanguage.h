#pragma once

namespace ZScript
{
class Function;
class Definitions;
} // namespace ZScript

class TLFunction
{
public:
	struct Parameter
	{
		wxString type;
		wxString name;
		wxString default_value;
		bool     optional;

		void parse(vector<wxString>& tokens);
	};

	struct Context
	{
		wxString          context;
		vector<Parameter> params;
		wxString          return_type;
		wxString          description;
		wxString          qualifiers;
		wxString          deprecated_v;
		wxString          deprecated_f;
		bool              custom;
	};

	TLFunction(const wxString& name = "") : name_{ name } {}
	~TLFunction() = default;

	const wxString&        name() const { return name_; }
	const vector<Context>& contexts() const { return contexts_; }
	Context                context(unsigned long index) const;

	void setName(const wxString& name) { this->name_ = name; }
	void addContext(
		const wxString& context,
		const wxString& args,
		const wxString& return_type,
		const wxString& description,
		const wxString& deprecated_f);
	void addContext(
		const wxString&          context,
		const ZScript::Function& func,
		bool                     custom,
		const wxString&          desc,
		const wxString&          dep_f);

	void clear()
	{
		name_.clear();
		contexts_.clear();
	}
	void clearContexts() { contexts_.clear(); }
	void clearCustomContexts();

	bool hasContext(const wxString& name);

private:
	wxString        name_;
	vector<Context> contexts_;
};

class TextLanguage
{
public:
	enum WordType
	{
		Keyword  = 0,
		Constant = 1,
		Type     = 2,
		Property = 3,
	};

	TextLanguage(const wxString& id);
	~TextLanguage();

	const wxString&         id() const { return id_; }
	const wxString&         name() const { return name_; }
	const wxString&         lineComment() const { return line_comment_l_[preferred_comments_]; }
	const wxString&         commentBegin() const { return comment_begin_l_[preferred_comments_]; }
	const wxString&         commentEnd() const { return comment_end_l_[preferred_comments_]; }
	const vector<wxString>& lineCommentL() const { return line_comment_l_; }
	const vector<wxString>& commentBeginL() const { return comment_begin_l_; }
	const vector<wxString>& commentEndL() const { return comment_end_l_; }
	unsigned                preferedComments() const { return preferred_comments_; }
	const wxString&         preprocessor() const { return preprocessor_; }
	const wxString&         docComment() const { return doc_comment_; }
	bool                    caseSensitive() const { return case_sensitive_; }
	const wxString&         blockBegin() const { return block_begin_; }
	const wxString&         blockEnd() const { return block_end_; }

	const vector<wxString>& ppBlockBegin() const { return pp_block_begin_; }
	const vector<wxString>& ppBlockEnd() const { return pp_block_end_; }
	const vector<wxString>& wordBlockBegin() const { return word_block_begin_; }
	const vector<wxString>& wordBlockEnd() const { return word_block_end_; }
	const vector<wxString>& jumpBlocks() const { return jump_blocks_; }
	const vector<wxString>& jumpBlocksIgnored() const { return jb_ignore_; }

	void copyTo(TextLanguage* copy);

	void setName(const wxString& name) { this->name_ = name; }
	void setPreferedComments(unsigned index) { preferred_comments_ = index; }
	void setLineCommentList(vector<wxString> token) { line_comment_l_ = std::move(token); };
	void setCommentBeginList(vector<wxString> token) { comment_begin_l_ = std::move(token); };
	void setCommentEndList(vector<wxString> token) { comment_end_l_ = std::move(token); };
	void setPreprocessor(const wxString& token) { preprocessor_ = token; }
	void setDocComment(const wxString& token) { doc_comment_ = token; }
	void setCaseSensitive(bool cs) { case_sensitive_ = cs; }
	void addWord(WordType type, const wxString& word, bool custom = false);
	void addFunction(
		wxString        name,
		const wxString& args,
		const wxString& desc        = "",
		const wxString& deprecated  = "",
		bool            replace     = false,
		const wxString& return_type = "");
	void loadZScript(ZScript::Definitions& defs, bool custom = false);

	wxString wordList(WordType type, bool include_custom = true);
	wxString functionsList();
	wxString autocompletionList(wxString start = "", bool include_custom = true);

	wxArrayString wordListSorted(WordType type, bool include_custom = true);
	wxArrayString functionsSorted();

	wxString wordLink(WordType type) const { return word_lists_[type].lookup_url; }
	wxString functionLink() const { return f_lookup_url_; }

	bool isWord(WordType type, const wxString& word);
	bool isFunction(const wxString& word);

	TLFunction* function(const wxString& name);

	void clearWordList(WordType type) { word_lists_[type].list.clear(); }
	void clearFunctions() { functions_.clear(); }
	void clearCustomDefs();

	// Static functions
	static bool          readLanguageDefinition(MemChunk& mc, const wxString& source);
	static bool          loadLanguages();
	static TextLanguage* fromId(const wxString& id);
	static TextLanguage* fromIndex(unsigned index);
	static TextLanguage* fromName(const wxString& name);
	static wxArrayString languageNames();

private:
	wxString         id_;                            // Used internally
	wxString         name_;                          // The language 'name' (will show up in the language dropdown, etc)
	unsigned         preferred_comments_ = 0;        // The preferred comment style index
	vector<wxString> line_comment_l_     = { "//" }; // A list of supported line comments
	vector<wxString> comment_begin_l_    = { "/*" }; // A list of supported block comment begin
	vector<wxString> comment_end_l_      = { "*/" }; // A list of supported block comment end
	wxString         preprocessor_       = "#";      // The beginning token for a preprocessor directive
	wxString         doc_comment_;                   // The beginning token for a 'doc' comment (eg. /// in c/c++)
	bool             case_sensitive_ = false;        // Whether words are case-sensitive
	vector<wxString> jump_blocks_;       // The keywords to search for when creating jump to list (eg. 'script')
	vector<wxString> jb_ignore_;         // The keywords to ignore when creating jump to list (eg. 'optional')
	wxString         block_begin_ = "{"; // The beginning of a block (eg. '{' in c/c++)
	wxString         block_end_   = "}"; // The end of a block (eg. '}' in c/c++)
	vector<wxString> pp_block_begin_;    // Preprocessor words to start a folding block (eg. 'ifdef')
	vector<wxString> pp_block_end_;      // Preprocessor words to end a folding block (eg. 'endif')
	vector<wxString> word_block_begin_;  // Words to start a folding block (eg. 'do' in lua)
	vector<wxString> word_block_end_;    // Words to end a folding block (eg. 'end' in lua)

	// Word lists
	struct WordList
	{
		vector<wxString> list;
		bool             upper;
		bool             lower;
		bool             caps;
		wxString         lookup_url;
	};
	WordList word_lists_[4];
	WordList word_lists_custom_[4];

	// Functions
	vector<TLFunction> functions_;
	wxString           f_lookup_url_;

	// Zscript function properties which cannot be parsed from (g)zdoom.pk3
	struct ZFuncExProp
	{
		wxString description;
		wxString deprecated_f;
	};
	std::map<wxString, ZFuncExProp> zfuncs_ex_props_;
};
