#pragma once

#include <sstream>

namespace slade
{
class ArchiveEntry;

namespace strutil
{
	// Static common strings
	static string FULLSTOP              = ".";
	static string COMMA                 = ",";
	static string COLON                 = ":";
	static string SEMICOLON             = ";";
	static string SLASH_FORWARD         = "/";
	static string SLASH_BACK            = "\\";
	static string QUOTE_SINGLE          = "'";
	static string QUOTE_DOUBLE          = "\"";
	static string CARET                 = "^";
	static string ESCAPED_QUOTE_DOUBLE  = "\\\"";
	static string ESCAPED_SLASH_BACK    = "\\\\";
	static string CURLYBRACE_OPEN       = "{";
	static string CURLYBRACE_CLOSE      = "}";
	static string DASH                  = "-";
	static string WHITESPACE_CHARACTERS = " \t\n\r\f\v";
	static string EMPTY                 = "";
	static string SPACE                 = " ";
	static string UNDERSCORE            = "_";
	static string AMPERSAND             = "&";
	static string EQUALS                = "=";
	static string BOOL_TRUE             = "true";
	static string BOOL_FALSE            = "false";

	// String comparisons and checks
	// CI = Case-Insensitive
	bool isInteger(const string& str, bool allow_hex = true);
	bool isHex(const string& str);
	bool isFloat(const string& str);
	bool equalCI(string_view left, string_view right);
	// bool equalCI(string_view left, const char* right);
	bool startsWith(string_view str, string_view check);
	bool startsWith(string_view str, char check);
	bool startsWithCI(string_view str, string_view check);
	bool startsWithCI(string_view str, char check);
	bool endsWith(string_view str, string_view check);
	bool endsWith(string_view str, char check);
	bool endsWithCI(string_view str, string_view check);
	bool endsWithCI(string_view str, char check);
	bool contains(string_view str, char check);
	bool containsCI(string_view str, char check);
	bool contains(string_view str, string_view check);
	bool containsCI(string_view str, string_view check);
	bool matches(string_view str, string_view match);
	bool matchesCI(string_view str, string_view match);

	// String transformations
	// IP = In-Place
	string  escapedString(string_view str, bool swap_backslash = false, bool escape_backslash = true);
	string& replaceIP(string& str, string_view from, string_view to);
	string  replace(string_view str, string_view from, string_view to);
	string& replaceFirstIP(string& str, string_view from, string_view to);
	string  replaceFirst(string_view str, string_view from, string_view to);
	string& lowerIP(string& str);
	string& upperIP(string& str);
	string  lower(string_view str);
	string  upper(string_view str);
	string& ltrimIP(string& str);
	string& rtrimIP(string& str);
	string& trimIP(string& str);
	string  ltrim(string_view str);
	string  rtrim(string_view str);
	string  trim(string_view str);
	string& capitalizeIP(string& str);
	string  capitalize(string_view str);
	string  wildcardToRegex(string_view str);
	string  prepend(string_view str, string_view prefix);
	string& prependIP(string& str, string_view prefix);

	// Substrings
	string              left(string_view str, unsigned n);
	string_view         leftV(string_view str, unsigned n);
	string              right(string_view str, unsigned n);
	string_view         rightV(string_view str, unsigned n);
	string              afterLast(string_view str, char chr);
	string_view         afterLastV(string_view str, char chr);
	string              afterFirst(string_view str, char chr);
	string_view         afterFirstV(string_view str, char chr);
	string              beforeLast(string_view str, char chr);
	string_view         beforeLastV(string_view str, char chr);
	string              beforeFirst(string_view str, char chr);
	string_view         beforeFirstV(string_view str, char chr);
	vector<string>      split(string_view str, char separator);
	vector<string_view> splitV(string_view str, char separator);
	string              truncate(string_view str, unsigned length);
	string&             truncateIP(string& str, unsigned length);
	string              removeLast(string_view str, unsigned n);
	string&             removeLastIP(string& str, unsigned n);
	string              removePrefix(string_view str, char prefix); // TODO: string_view prefix
	string&             removePrefixIP(string& str, char prefix);
	string              removeSuffix(string_view str, char suffix); // TODO: string_view suffix
	string&             removeSuffixIP(string& str, char suffix);

	// Misc
	void processIncludes(const string& filename, string& out);
	void processIncludes(ArchiveEntry* entry, string& out, bool use_res = true);

	// Conversion
	int         asInt(string_view str, int base = 10);
	unsigned    asUInt(string_view str, int base = 10);
	float       asFloat(string_view str);
	double      asDouble(string_view str);
	bool        asBoolean(string_view str);
	bool        toInt(string_view str, int& target, int base = 10);
	bool        toUInt(string_view str, unsigned& target, int base = 10);
	bool        toFloat(string_view str, float& target);
	bool        toDouble(string_view str, double& target);
	string      toString(string_view str);
	string_view viewFromChars(const char* chars, unsigned max_length);

	// Joins all given args into a single string
	template<typename... Args> string join(const Args&... args)
	{
		std::ostringstream stream;

		int a[] = { 0, ((void)(stream << args), 0)... };

		return stream.str();
	}

	// Encoding
	string toUTF8(string_view str);
	string fromUTF8(string_view str);

	// Path class
	class Path
	{
	public:
		Path(string_view full_path);

		const string& fullPath() const { return full_path_; }

		string_view         path(bool include_end_sep = true) const;
		string_view         fileName(bool include_extension = true) const;
		string_view         extension() const;
		vector<string_view> pathParts() const;
		bool                hasExtension() const;

		void set(string_view full_path);
		void setPath(string_view path);
		void setPath(const vector<string_view>& parts);
		void setPath(const vector<string>& parts);
		void setFileName(string_view file_name);
		void setExtension(string_view extension);

		// Static functions
		static string_view fileNameOf(string_view full_path, bool include_extension = true);
		static string_view extensionOf(string_view full_path);
		static string_view pathOf(string_view full_path, bool include_end_sep = true);
		static bool        filePathsMatch(string_view left, string_view right);

	private:
		string            full_path_;
		string::size_type filename_start_ = string::npos;
		string::size_type filename_end_   = string::npos;
	};

	// String transformer (to chain different string transformations)
	struct Transformer
	{
		string str;

		Transformer(string_view init) : str{ init } {}
		Transformer(const wxString& init) : str{ init.data(), init.size() } {}

		// Case
		Transformer& upper()
		{
			upperIP(str);
			return *this;
		}
		Transformer& lower()
		{
			lowerIP(str);
			return *this;
		}
		Transformer& capitalize()
		{
			capitalizeIP(str);
			return *this;
		}

		// Trim
		Transformer& ltrim()
		{
			ltrimIP(str);
			return *this;
		}
		Transformer& rtrim()
		{
			rtrimIP(str);
			return *this;
		}
		Transformer& trim()
		{
			trimIP(str);
			return *this;
		}
	};


	// Simple Tokenizer function(s)
	// Unlike the Tokenizer class, this will tokenize a string into a vector of Tokens
	// (basically string_views with some extra info) all at once,
	// so it's best to use if we know we won't want to 'abort' tokenizing part way through the source string

	struct TokenizeOptions
	{
		// Comment types to skip
		bool comments_cstyle     = false; // C comments (/* ... */)
		bool comments_cppstyle   = false; // C++ comments (//)
		bool comments_hash       = false; // Hash comments (#)
		bool comments_doublehash = false; // Double hash comments (##)
		bool comments_shell      = false; // Shell comments (;)
		bool comments_lua        = false; // Lua comments (-- (line comment) and --[[ ... ]]-- (block comment))

		string special_characters    = DEFAULT_SPECIAL_CHARACTERS;    // These will always be read as separate tokens
		string whitespace_characters = DEFAULT_WHITESPACE_CHARACTERS; // Characters to count as whitespace
		char   string_escape         = '\\';                          // Escape character for quoted strings
		bool   decorate              = false;                         // Read //$ comments as tokens
		bool   debug                 = false;                         // Log each token read

		static inline string DEFAULT_SPECIAL_CHARACTERS    = ";,:|={}/";
		static inline string DEFAULT_WHITESPACE_CHARACTERS = " \t\r\f\v";
	};

	struct Token
	{
		string_view text;
		bool        quoted_string;
		unsigned    line_no;
		unsigned    position;
	};

	void          tokenize(vector<Token>& tokens, const string& str, const TokenizeOptions& options);
	vector<Token> tokenize(const string& str, const TokenizeOptions& options);

} // namespace strutil




// TODO: Remove these
namespace wxStringUtils
{
	// Static common strings
	static wxString FULLSTOP             = ".";
	static wxString COMMA                = ",";
	static wxString COLON                = ":";
	static wxString SEMICOLON            = ";";
	static wxString SLASH_FORWARD        = "/";
	static wxString SLASH_BACK           = "\\";
	static wxString QUOTE_SINGLE         = "'";
	static wxString QUOTE_DOUBLE         = "\"";
	static wxString CARET                = "^";
	static wxString ESCAPED_QUOTE_DOUBLE = "\\\"";
	static wxString ESCAPED_SLASH_BACK   = "\\\\";
	static wxString CURLYBRACE_OPEN      = "{";
	static wxString CURLYBRACE_CLOSE     = "}";

	wxString escapedString(const wxString& str, bool swap_backslash = false);

	void processIncludes(const wxString& filename, wxString& out);
	void processIncludes(ArchiveEntry* entry, wxString& out, bool use_res = true);

	bool isInteger(const wxString& str, bool allow_hex = true);
	bool isHex(const wxString& str);
	bool isFloat(const wxString& str);

	int    toInt(const wxString& str);
	float  toFloat(const wxString& str);
	double toDouble(const wxString& str);
} // namespace wxStringUtils
} // namespace slade
