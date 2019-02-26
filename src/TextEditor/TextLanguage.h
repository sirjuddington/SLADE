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
		std::string type;
		std::string name;
		std::string default_value;
		bool        optional;

		void parse(vector<std::string>& tokens);
	};

	struct Context
	{
		std::string       context;
		vector<Parameter> params;
		std::string       return_type;
		std::string       description;
		std::string       qualifiers;
		std::string       deprecated_v;
		std::string       deprecated_f;
		bool              custom = false;

		Context() = default;

		Context(std::string_view context, std::string_view return_type, std::string_view description) :
			context{ context },
			return_type{ return_type },
			description{ description }
		{
		}

		Context(
			std::string_view context,
			std::string_view return_type,
			std::string_view description,
			std::string_view deprecated_v,
			std::string_view deprecated_f,
			bool             custom) :
			context{ context },
			return_type{ return_type },
			description{ description },
			deprecated_v{ deprecated_v },
			deprecated_f{ deprecated_f },
			custom{ custom }
		{
		}
	};

	TLFunction(std::string_view name = "") : name_{ name } {}
	~TLFunction() = default;

	const std::string&     name() const { return name_; }
	const vector<Context>& contexts() const { return contexts_; }
	const Context&         context(unsigned long index) const;

	void setName(std::string_view name) { this->name_ = name; }
	void addContext(
		std::string_view context,
		std::string_view args,
		std::string_view return_type,
		std::string_view description,
		std::string_view deprecated_f);
	void addContext(
		std::string_view         context,
		const ZScript::Function& func,
		bool                     custom,
		std::string_view         desc,
		std::string_view         dep_f);

	void clear()
	{
		name_.clear();
		contexts_.clear();
	}
	void clearContexts() { contexts_.clear(); }
	void clearCustomContexts();

	bool hasContext(std::string_view name);

private:
	std::string     name_;
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

	TextLanguage(std::string_view id);
	~TextLanguage();

	const std::string&         id() const { return id_; }
	const std::string&         name() const { return name_; }
	const std::string&         lineComment() const { return line_comment_l_[preferred_comments_]; }
	const std::string&         commentBegin() const { return comment_begin_l_[preferred_comments_]; }
	const std::string&         commentEnd() const { return comment_end_l_[preferred_comments_]; }
	const vector<std::string>& lineCommentL() const { return line_comment_l_; }
	const vector<std::string>& commentBeginL() const { return comment_begin_l_; }
	const vector<std::string>& commentEndL() const { return comment_end_l_; }
	unsigned                   preferedComments() const { return preferred_comments_; }
	const std::string&         preprocessor() const { return preprocessor_; }
	const std::string&         docComment() const { return doc_comment_; }
	bool                       caseSensitive() const { return case_sensitive_; }
	const std::string&         blockBegin() const { return block_begin_; }
	const std::string&         blockEnd() const { return block_end_; }

	const vector<std::string>& ppBlockBegin() const { return pp_block_begin_; }
	const vector<std::string>& ppBlockEnd() const { return pp_block_end_; }
	const vector<std::string>& wordBlockBegin() const { return word_block_begin_; }
	const vector<std::string>& wordBlockEnd() const { return word_block_end_; }
	const vector<std::string>& jumpBlocks() const { return jump_blocks_; }
	const vector<std::string>& jumpBlocksIgnored() const { return jb_ignore_; }

	void copyTo(TextLanguage* copy);

	void setName(std::string_view name) { this->name_ = name; }
	void setPreferedComments(unsigned index) { preferred_comments_ = index; }
	void setLineCommentList(vector<std::string> token) { line_comment_l_ = std::move(token); };
	void setCommentBeginList(vector<std::string> token) { comment_begin_l_ = std::move(token); };
	void setCommentEndList(vector<std::string> token) { comment_end_l_ = std::move(token); };
	void setPreprocessor(std::string_view token) { preprocessor_ = token; }
	void setDocComment(std::string_view token) { doc_comment_ = token; }
	void setCaseSensitive(bool cs) { case_sensitive_ = cs; }
	void addWord(WordType type, std::string_view word, bool custom = false);
	void addFunction(
		std::string_view name,
		std::string_view args,
		std::string_view desc        = "",
		std::string_view deprecated  = "",
		bool             replace     = false,
		std::string_view return_type = "");
	void loadZScript(ZScript::Definitions& defs, bool custom = false);

	std::string wordList(WordType type, bool include_custom = true);
	std::string functionsList();
	std::string autocompletionList(std::string_view start = "", bool include_custom = true);

	vector<std::string> wordListSorted(WordType type, bool include_custom = true);
	vector<std::string> functionsSorted();

	std::string        wordLink(WordType type) const { return word_lists_[type].lookup_url; }
	const std::string& functionLink() const { return f_lookup_url_; }

	bool isWord(WordType type, std::string_view word);
	bool isFunction(std::string_view word);

	TLFunction* function(std::string_view name);

	void clearWordList(WordType type) { word_lists_[type].list.clear(); }
	void clearFunctions() { functions_.clear(); }
	void clearCustomDefs();

	// Static functions
	static bool                readLanguageDefinition(MemChunk& mc, std::string_view source);
	static bool                loadLanguages();
	static TextLanguage*       fromId(std::string_view id);
	static TextLanguage*       fromIndex(unsigned index);
	static TextLanguage*       fromName(std::string_view name);
	static vector<std::string> languageNames();

private:
	std::string         id_;                     // Used internally
	std::string         name_;                   // The language 'name' (will show up in the language dropdown, etc)
	unsigned            preferred_comments_ = 0; // The preferred comment style index
	vector<std::string> line_comment_l_     = { "//" }; // A list of supported line comments
	vector<std::string> comment_begin_l_    = { "/*" }; // A list of supported block comment begin
	vector<std::string> comment_end_l_      = { "*/" }; // A list of supported block comment end
	std::string         preprocessor_       = "#";      // The beginning token for a preprocessor directive
	std::string         doc_comment_;                   // The beginning token for a 'doc' comment (eg. /// in c/c++)
	bool                case_sensitive_ = false;        // Whether words are case-sensitive
	vector<std::string> jump_blocks_;       // The keywords to search for when creating jump to list (eg. 'script')
	vector<std::string> jb_ignore_;         // The keywords to ignore when creating jump to list (eg. 'optional')
	std::string         block_begin_ = "{"; // The beginning of a block (eg. '{' in c/c++)
	std::string         block_end_   = "}"; // The end of a block (eg. '}' in c/c++)
	vector<std::string> pp_block_begin_;    // Preprocessor words to start a folding block (eg. 'ifdef')
	vector<std::string> pp_block_end_;      // Preprocessor words to end a folding block (eg. 'endif')
	vector<std::string> word_block_begin_;  // Words to start a folding block (eg. 'do' in lua)
	vector<std::string> word_block_end_;    // Words to end a folding block (eg. 'end' in lua)

	// Word lists
	struct WordList
	{
		vector<std::string> list;
		bool                upper;
		bool                lower;
		bool                caps;
		std::string         lookup_url;
	};
	WordList word_lists_[4];
	WordList word_lists_custom_[4];

	// Functions
	vector<TLFunction> functions_;
	std::string        f_lookup_url_;

	// Zscript function properties which cannot be parsed from (g)zdoom.pk3
	struct ZFuncExProp
	{
		std::string description;
		std::string deprecated_f;
	};
	std::map<std::string, ZFuncExProp> zfuncs_ex_props_;
};
