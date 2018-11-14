#pragma once

namespace ZScript { class Function; class Definitions; }

class TLFunction
{
public:
	struct Parameter
	{
		string	type;
		string	name;
		string	default_value;
		bool	optional;

		void	parse(vector<string>& tokens);
	};

	struct Context
	{
		string				context;
		vector<Parameter>	params;
		string				return_type;
		string				description;
		string				qualifiers;
		string				deprecated;
		bool				custom;
	};

	TLFunction(string name = "");
	~TLFunction();

	const string&			name() const { return name_; }
	const vector<Context>&	contexts() const { return contexts_; }
	Context					context(unsigned index) const;

	void	setName(string name) { this->name_ = name; }
	void	addContext(
				const string& context,
				const string& args,
				const string& return_type = "void",
				const string& description = ""
			);
	void	addContext(const string& context, const ZScript::Function& func, bool custom);

	void	clear() { name_.clear(); contexts_.clear(); }
	void    clearContexts() { contexts_.clear();}
	void	clearCustomContexts();

	bool	hasContext(const string& name);

private:
	string			name_;
	vector<Context>	contexts_;
};

class TextLanguage
{
public:
	enum WordType
	{
		Keyword		= 0,
		Constant	= 1,
		Type		= 2,
		Property	= 3,
	};

	TextLanguage(string id);
	~TextLanguage();

	const string&			id() const { return id_; }
	const string&			name() const { return name_; }
	const string&			lineComment() const { return line_comment_l_[prefered_comments_]; }
	const string&			commentBegin() const { return comment_begin_l_[prefered_comments_]; }
	const string&			commentEnd() const { return comment_end_l_[prefered_comments_]; }
	const vector<string>&	lineCommentL() const { return line_comment_l_; }
	const vector<string>&	commentBeginL() const { return comment_begin_l_; }
	const vector<string>&	commentEndL() const { return comment_end_l_; }
	const string&			preprocessor() const { return preprocessor_; }
	const string&			docComment() const { return doc_comment_; }
	bool					caseSensitive() const { return case_sensitive_; }
	const string&			blockBegin() const { return block_begin_; }
	const string&			blockEnd() const { return block_end_; }

	const vector<string>&	ppBlockBegin() const { return pp_block_begin_; }
	const vector<string>&	ppBlockEnd() const { return pp_block_end_; }
	const vector<string>&	wordBlockBegin() const { return word_block_begin_; }
	const vector<string>&	wordBlockEnd() const { return word_block_end_; }
	const vector<string>&	jumpBlocks() const { return jump_blocks_; }
	const vector<string>&	jumpBlocksIgnored() const { return jb_ignore_; }

	void	copyTo(TextLanguage* copy);

	void		setName(string name) { this->name_ = name; }
	void 		setPreferedComments(unsigned index) { prefered_comments_ = index; }
	unsigned	getPreferedComments() { return prefered_comments_; }
	void 		setLineCommentList(vector<string> token) { line_comment_l_ = token; };
	void 		setCommentBeginList(vector<string> token) { comment_begin_l_ = token; };
	void 		setCommentEndList(vector<string> token) { comment_end_l_ = token; };
	void		setPreprocessor(string token) { preprocessor_ = token; }
	void		setDocComment(string token) { doc_comment_ = token; }
	void		setCaseSensitive(bool cs) { case_sensitive_ = cs; }
	void		addWord(WordType type, string word, bool custom = false);
	void		addFunction(string name,
							string args,
							string desc = "",
							bool replace = false,
							string return_type = "");
	void	loadZScript(ZScript::Definitions& defs, bool custom = false);

	string	wordList(WordType type, bool include_custom = true);
	string	functionsList();
	string	autocompletionList(string start = "", bool include_custom = true);

	wxArrayString	wordListSorted(WordType type, bool include_custom = true);
	wxArrayString	functionsSorted();

	string	wordLink(WordType type) const { return word_lists_[type].lookup_url; }
	string	functionLink() const { return f_lookup_url_; }

	bool	isWord(WordType type, string word);
	bool	isFunction(string word);

	TLFunction*	function(string name);

	void	clearWordList(WordType type) { word_lists_[type].list.clear(); }
	void	clearFunctions() { functions_.clear(); }
	void	clearCustomDefs();

	// Static functions
	static bool				readLanguageDefinition(MemChunk& mc, string source);
	static bool				loadLanguages();
	static TextLanguage*	fromId(string id);
	static TextLanguage*	fromIndex(unsigned index);
	static TextLanguage*	fromName(string name);
	static wxArrayString	languageNames();

private:
	string			id_;				// Used internally
	string			name_;				// The language 'name' (will show up in the language dropdown, etc)
	unsigned		prefered_comments_; // The prefered comment style index
	vector<string>	line_comment_l_;	// A list of supported line comments
	vector<string>	comment_begin_l_;	// A list of supported block comment begin
	vector<string>	comment_end_l_;		// A list of supported block comment end
	string			preprocessor_;		// The beginning token for a preprocessor directive
	string			doc_comment_;		// The beginning token for a 'doc' comment (eg. /// in c/c++)
	bool			case_sensitive_;	// Whether words are case-sensitive
	vector<string>	jump_blocks_;		// The keywords to search for when creating jump to list (eg. 'script')
	vector<string>	jb_ignore_;			// The keywords to ignore when creating jump to list (eg. 'optional')
	string			block_begin_;		// The beginning of a block (eg. '{' in c/c++)
	string			block_end_;			// The end of a block (eg. '}' in c/c++)
	vector<string>	pp_block_begin_;	// Preprocessor words to start a folding block (eg. 'ifdef')
	vector<string>	pp_block_end_;		// Preprocessor words to end a folding block (eg. 'endif')
	vector<string>	word_block_begin_;	// Words to start a folding block (eg. 'do' in lua)
	vector<string>	word_block_end_;	// Words to end a folding block (eg. 'end' in lua)

	// Word lists
	struct WordList
	{
		vector<string>	list;
		bool			upper;
		bool			lower;
		bool			caps;
		string			lookup_url;
	};
	WordList	word_lists_[4];
	WordList	word_lists_custom_[4];

	// Functions
	vector<TLFunction>	functions_;
	bool				f_upper_;
	bool				f_lower_;
	bool				f_caps_;
	string				f_lookup_url_;
};
