
#ifndef __TEXTLANGUAGE_H__
#define __TEXTLANGUAGE_H__

class TLFunction
{
private:
	string			name;
	vector<string>	arg_sets;

public:
	TLFunction(string name = "");
	~TLFunction();

	string		getName() { return name; }
	string		getArgSet(unsigned index);
	unsigned	nArgSets() { return arg_sets.size(); }

	void	setName(string name) { this->name = name; }
	void	addArgSet(string args) { arg_sets.push_back(args); }

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
	bool				case_sensitive;
	vector<string>		jump_blocks;	// The keywords to search for when creating jump to list (eg. 'script')

	// Keywords
	vector<string>		keywords;
	bool				k_upper;
	bool				k_lower;
	bool				k_caps;
	string				k_lookup_url;

	// Constants
	vector<string>		constants;
	bool				c_upper;
	bool				c_lower;
	bool				c_caps;
	string				c_lookup_url;

	// Functions
	vector<TLFunction*>	functions;
	bool				f_upper;
	bool				f_lower;
	bool				f_caps;
	string				f_lookup_url;

public:
	TextLanguage(string id);
	~TextLanguage();

	string	getId() { return id; }
	string	getName() { return name; }

	void	copyTo(TextLanguage* copy);

	void	setName(string name) { this->name = name; }
	void	setLineComment(string token) { line_comment = token; }
	void	setCommentBegin(string token) { comment_begin = token; }
	void	setCommentEnd(string token) { comment_end = token; }
	void	setPreprocessor(string token) { preprocessor = token; }
	void	setCaseSensitive(bool cs) { case_sensitive = cs; }
	void	addKeyword(string keyword);
	void	addConstant(string constant);
	void	addFunction(string name, string args);

	string	getKeywordsList();
	string	getConstantsList();
	string	getFunctionsList();
	string	getAutocompletionList(string start = "");

	wxArrayString	getKeywordsSorted();
	wxArrayString	getConstantsSorted();
	wxArrayString	getFunctionsSorted();

	string	getKeywordLink() { return k_lookup_url; }
	string	getConstantLink() { return c_lookup_url; }
	string	getFunctionLink() { return f_lookup_url; }

	bool	isKeyword(string word);
	bool	isConstant(string word);
	bool	isFunction(string word);

	TLFunction*	getFunction(string name);

	unsigned	nJumpBlocks() { return jump_blocks.size(); }
	string		jumpBlock(unsigned index) { return jump_blocks[index]; }

	void	clearKeywords() { keywords.clear(); }
	void	clearConstants() { constants.clear(); }
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
