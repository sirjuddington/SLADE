#pragma once

class TLFunction
{
public:
	struct ArgSet
	{
		string	args;
		string	context;
	};

	TLFunction(string name = "", string return_type = "void");
	~TLFunction();

	const string&	name() const { return name_; }
	ArgSet			argSet(unsigned index) const;
	const string&	description() const { return description_; }
	const string&	returnType() const { return return_type_; }
	unsigned		nArgSets() const { return arg_sets_.size(); }

	void	setName(string name) { this->name_ = name; }
	void	addArgSet(string args, string context = "") { arg_sets_.push_back({ args, context }); }
	void	setDescription(string desc) { description_ = desc; }

	string		generateCallTipString(int arg_set = 0);
	point2_t	getArgTextExtent(int arg, int arg_set = 0);

private:
	string			name_;
	vector<ArgSet>	arg_sets_;
	string			description_;
	string			return_type_;
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

	string	id() const { return id_; }
	string	name() const { return name_; }
	string	lineComment() const { return line_comment_; }
	string	commentBegin() const { return comment_begin_; }
	string	commentEnd() const { return comment_end_; }
	string	preprocessor() const { return preprocessor_; }
	string	docComment() const { return doc_comment_; }
	bool	caseSensitive() const { return case_sensitive_; }
	string	blockBegin() const { return block_begin_; }
	string	blockEnd() const { return block_end_; }

	const vector<string>&	ppBlockBegin() const { return pp_block_begin_; }
	const vector<string>&	ppBlockEnd() const { return pp_block_end_; }
	const vector<string>&	jumpBlocks() const { return jump_blocks_; }
	const vector<string>&	jumpBlocksIgnored() const { return jb_ignore_; }

	void	copyTo(TextLanguage* copy);

	void	setName(string name) { this->name_ = name; }
	void	setLineComment(string token) { line_comment_ = token; }
	void	setCommentBegin(string token) { comment_begin_ = token; }
	void	setCommentEnd(string token) { comment_end_ = token; }
	void	setPreprocessor(string token) { preprocessor_ = token; }
	void	setDocComment(string token) { doc_comment_ = token; }
	void	setCaseSensitive(bool cs) { case_sensitive_ = cs; }
	void	addWord(WordType type, string word);
	void	addFunction(
		string name,
		string args,
		string desc = "",
		bool replace = false,
		string return_type = ""
	);

	string	wordList(WordType type);
	string	functionsList();
	string	autocompletionList(string start = "");

	wxArrayString	wordListSorted(WordType type);
	wxArrayString	functionsSorted();

	string	wordLink(WordType type) const { return word_lists[type].lookup_url; }
	string	functionLink() const { return f_lookup_url; }

	bool	isWord(WordType type, string word);
	bool	isFunction(string word);

	TLFunction*	function(string name);

	void	clearWordList(WordType type) { word_lists[type].list.clear(); }
	void	clearFunctions() { functions.clear(); }

	

	// Static functions
	static bool				readLanguageDefinition(MemChunk& mc, string source);
	static bool				loadLanguages();
	static TextLanguage*	fromId(string id);
	static TextLanguage*	fromIndex(unsigned index);
	static TextLanguage*	fromName(string name);
	static wxArrayString	languageNames();

private:
	string				id_;				// Used internally
	string				name_;				// The language 'name' (will show up in the language dropdown, etc)
	string				line_comment_;		// The beginning token for a line comment
	string				comment_begin_;		// The beginning token for a block comment
	string				comment_end_;		// The ending token for a block comment
	string				preprocessor_;		// The beginning token for a preprocessor directive
	string				doc_comment_;		// The beginning token for a 'doc' comment (eg. /// in c/c++)
	bool				case_sensitive_;	// Whether words are case-sensitive
	vector<string>		jump_blocks_;		// The keywords to search for when creating jump to list (eg. 'script')
	vector<string>		jb_ignore_;			// The keywords to ignore when creating jump to list (eg. 'optional')
	string				block_begin_;		// The beginning of a block (eg. '{' in c/c++)
	string				block_end_;			// The end of a block (eg. '}' in c/c++)
	vector<string>		pp_block_begin_;	// Preprocessor words to start a folding block (eg. 'ifdef')
	vector<string>		pp_block_end_;		// Preprocessor words to end a folding block (eg. 'endif')

	// Word lists
	struct WordList
	{
		vector<string>	list;
		bool			upper;
		bool			lower;
		bool			caps;
		string			lookup_url;
	};
	WordList	word_lists[4];

	// Functions
	vector<TLFunction*>	functions;
	bool				f_upper;
	bool				f_lower;
	bool				f_caps;
	string				f_lookup_url;
};
