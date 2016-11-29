
#ifndef __TEXTLANGUAGE_H__
#define __TEXTLANGUAGE_H__

class TLFunction
{
private:
	string			name;
	vector<string>	arg_sets;
	string			description;
	string			return_type;

public:
	TLFunction(string name = "", string return_type = "void");
	~TLFunction();

	string		getName() { return name; }
	string		getArgSet(unsigned index);
	string		getDescription() { return description; }
	string		getReturnType() { return return_type; }
	unsigned	nArgSets() { return arg_sets.size(); }

	void	setName(string name) { this->name = name; }
	void	addArgSet(string args) { arg_sets.push_back(args); }
	void	setDescription(string desc) { description = desc; }

	string		generateCallTipString(int arg_set = 0);
	point2_t	getArgTextExtent(int arg, int arg_set = 0);
};

class TextLanguage
{
private:
	string				id;				// Used internally
	string				name;			// The language 'name' (will show up in the language selection dropdown, etc)
	string				line_comment;	// The beginning token for a line comment
	string				comment_begin;	// The beginning token for a block comment
	string				comment_end;	// The ending token for a block comment
	string				preprocessor;	// The beginning token for a preprocessor directive
	string				doc_comment;	// The beginning token for a 'doc' comment (eg. /// in c/c++)
	bool				case_sensitive;
	vector<string>		jump_blocks;	// The keywords to search for when creating jump to list (eg. 'script')
	vector<string>		jb_ignore;		// The keywords to ignore when creating jump to list (eg. 'optional')

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

	string	getId() { return id; }
	string	getName() { return name; }
	string	getLineComment() { return line_comment; }
	string	getCommentBegin() { return comment_begin; }
	string	getCommentEnd() { return comment_end; }
	string	getPreprocessor() { return preprocessor; }
	string	getDocComment() { return doc_comment; }

	void	copyTo(TextLanguage* copy);

	void	setName(string name) { this->name = name; }
	void	setLineComment(string token) { line_comment = token; }
	void	setCommentBegin(string token) { comment_begin = token; }
	void	setCommentEnd(string token) { comment_end = token; }
	void	setPreprocessor(string token) { preprocessor = token; }
	void	setDocComment(string token) { doc_comment = token; }
	void	setCaseSensitive(bool cs) { case_sensitive = cs; }
	void	addWord(WordType type, string word);
	void	addFunction(
		string name,
		string args,
		string desc = "",
		bool replace = false,
		string return_type = ""
	);

	string	getWordList(WordType type);
	string	getFunctionsList();
	string	getAutocompletionList(string start = "");

	wxArrayString	getWordListSorted(WordType type);
	wxArrayString	getFunctionsSorted();

	string	getWordLink(WordType type) { return word_lists[type].lookup_url; }
	string	getFunctionLink() { return f_lookup_url; }

	bool	isWord(WordType type, string word);
	bool	isFunction(string word);

	TLFunction*	getFunction(string name);

	unsigned	nJumpBlocks() { return jump_blocks.size(); }
	string		jumpBlock(unsigned index) { return jump_blocks[index]; }
	unsigned	nJBIgnore() { return jb_ignore.size(); }
	string		jBIgnore(unsigned index) { return jb_ignore[index]; }

	void	clearWordList(WordType type) { word_lists[type].list.clear(); }
	void	clearFunctions() { functions.clear(); }

	// Static functions
	static bool				readLanguageDefinition(MemChunk& mc, string source);
	static bool				loadLanguages();
	static TextLanguage*	getLanguage(string id);
	static TextLanguage*	getLanguage(unsigned index);
	static TextLanguage*	getLanguageByName(string name);
	static wxArrayString	getLanguageNames();
};

#endif//__TEXTLANGUAGE_H__
