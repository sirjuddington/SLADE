#pragma once

namespace slade
{
namespace zscript
{
	class Function;
	class Definitions;
} // namespace zscript

class TLFunction
{
public:
	struct Parameter
	{
		string type;
		string name;
		string default_value;
		bool   optional;

		void parse(vector<string>& tokens);
	};

	struct Context
	{
		string            context;
		vector<Parameter> params;
		string            return_type;
		string            description;
		string            qualifiers;
		string            deprecated_v;
		string            deprecated_f;
		bool              custom = false;

		Context() = default;

		Context(string_view context, string_view return_type, string_view description) :
			context{ context },
			return_type{ return_type },
			description{ description }
		{
		}

		Context(
			string_view context,
			string_view return_type,
			string_view description,
			string_view deprecated_v,
			string_view deprecated_f,
			bool        custom) :
			context{ context },
			return_type{ return_type },
			description{ description },
			deprecated_v{ deprecated_v },
			deprecated_f{ deprecated_f },
			custom{ custom }
		{
		}
	};

	TLFunction(string_view name = "") : name_{ name } {}

	const string&          name() const { return name_; }
	const vector<Context>& contexts() const { return contexts_; }
	const Context&         context(unsigned long index) const;

	void setName(string_view name) { name_ = name; }
	void addContext(
		string_view context,
		string_view args,
		string_view return_type,
		string_view description,
		string_view deprecated_f);
	void addContext(
		string_view              context,
		const zscript::Function& func,
		bool                     custom,
		string_view              desc,
		string_view              dep_f);

	void clear()
	{
		name_.clear();
		contexts_.clear();
	}
	void clearContexts() { contexts_.clear(); }
	void clearCustomContexts();

	bool hasContext(string_view name) const;

private:
	string          name_;
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

	TextLanguage(string_view id);
	~TextLanguage();

	const string&         id() const { return id_; }
	const string&         name() const { return name_; }
	const string&         lineComment() const { return line_comment_l_[preferred_comments_]; }
	const string&         commentBegin() const { return comment_begin_l_[preferred_comments_]; }
	const string&         commentEnd() const { return comment_end_l_[preferred_comments_]; }
	const vector<string>& lineCommentL() const { return line_comment_l_; }
	const vector<string>& commentBeginL() const { return comment_begin_l_; }
	const vector<string>& commentEndL() const { return comment_end_l_; }
	unsigned              preferedComments() const { return preferred_comments_; }
	const string&         preprocessor() const { return preprocessor_; }
	const string&         docComment() const { return doc_comment_; }
	bool                  caseSensitive() const { return case_sensitive_; }
	const string&         blockBegin() const { return block_begin_; }
	const string&         blockEnd() const { return block_end_; }

	const vector<string>& ppBlockBegin() const { return pp_block_begin_; }
	const vector<string>& ppBlockEnd() const { return pp_block_end_; }
	const vector<string>& wordBlockBegin() const { return word_block_begin_; }
	const vector<string>& wordBlockEnd() const { return word_block_end_; }
	const vector<string>& jumpBlocks() const { return jump_blocks_; }
	const vector<string>& jumpBlocksIgnored() const { return jb_ignore_; }

	void copyTo(TextLanguage* copy) const;

	void setName(string_view name) { name_ = name; }
	void setPreferedComments(unsigned index) { preferred_comments_ = index; }
	void setLineCommentList(vector<string> token) { line_comment_l_ = std::move(token); }
	void setCommentBeginList(vector<string> token) { comment_begin_l_ = std::move(token); }
	void setCommentEndList(vector<string> token) { comment_end_l_ = std::move(token); }
	void setPreprocessor(string_view token) { preprocessor_ = token; }
	void setDocComment(string_view token) { doc_comment_ = token; }
	void setCaseSensitive(bool cs) { case_sensitive_ = cs; }
	void addWord(WordType type, string_view word, bool custom = false);
	void addFunction(
		string_view name,
		string_view args,
		string_view desc        = "",
		string_view deprecated  = "",
		bool        replace     = false,
		string_view return_type = "");
	void loadZScript(const zscript::Definitions& defs, bool custom = false);

	string wordList(WordType type, bool include_custom = true) const;
	string functionsList() const;
	string autocompletionList(string_view start = "", bool include_custom = true);

	vector<string> wordListSorted(WordType type, bool include_custom = true) const;
	vector<string> functionsSorted() const;

	string        wordLink(WordType type) const { return word_lists_[type].lookup_url; }
	const string& functionLink() const { return f_lookup_url_; }

	bool isWord(WordType type, string_view word) const;
	bool isFunction(string_view word) const;

	TLFunction* function(string_view name);

	void clearWordList(WordType type) { word_lists_[type].list.clear(); }
	void clearFunctions() { functions_.clear(); }
	void clearCustomDefs();

	// Static functions
	static bool           readLanguageDefinition(const MemChunk& mc, string_view source);
	static bool           loadLanguages();
	static TextLanguage*  fromId(string_view id);
	static TextLanguage*  fromIndex(unsigned index);
	static TextLanguage*  fromName(string_view name);
	static vector<string> languageNames();

private:
	string         id_;                            // Used internally
	string         name_;                          // The language 'name' (will show up in the language dropdown, etc)
	unsigned       preferred_comments_ = 0;        // The preferred comment style index
	vector<string> line_comment_l_     = { "//" }; // A list of supported line comments
	vector<string> comment_begin_l_    = { "/*" }; // A list of supported block comment begin
	vector<string> comment_end_l_      = { "*/" }; // A list of supported block comment end
	string         preprocessor_       = "#";      // The beginning token for a preprocessor directive
	string         doc_comment_;                   // The beginning token for a 'doc' comment (eg. /// in c/c++)
	bool           case_sensitive_ = false;        // Whether words are case-sensitive
	vector<string> jump_blocks_;       // The keywords to search for when creating jump to list (eg. 'script')
	vector<string> jb_ignore_;         // The keywords to ignore when creating jump to list (eg. 'optional')
	string         block_begin_ = "{"; // The beginning of a block (eg. '{' in c/c++)
	string         block_end_   = "}"; // The end of a block (eg. '}' in c/c++)
	vector<string> pp_block_begin_;    // Preprocessor words to start a folding block (eg. 'ifdef')
	vector<string> pp_block_end_;      // Preprocessor words to end a folding block (eg. 'endif')
	vector<string> word_block_begin_;  // Words to start a folding block (eg. 'do' in lua)
	vector<string> word_block_end_;    // Words to end a folding block (eg. 'end' in lua)

	// Word lists
	struct WordList
	{
		vector<string> list;
		bool           upper;
		bool           lower;
		bool           caps;
		string         lookup_url;
	};
	WordList word_lists_[4];
	WordList word_lists_custom_[4];

	// Functions
	vector<TLFunction> functions_;
	string             f_lookup_url_;

	// Zscript function properties which cannot be parsed from (g)zdoom.pk3
	struct ZFuncExProp
	{
		string description;
		string deprecated_f;
	};
	std::map<string, ZFuncExProp> zfuncs_ex_props_;
};
} // namespace slade
