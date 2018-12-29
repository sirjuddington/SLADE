#pragma once

class ArchiveEntry;

namespace StringUtils
{
// Static common strings
static string FULLSTOP             = ".";
static string COMMA                = ",";
static string COLON                = ":";
static string SEMICOLON            = ";";
static string SLASH_FORWARD        = "/";
static string SLASH_BACK           = "\\";
static string QUOTE_SINGLE         = "'";
static string QUOTE_DOUBLE         = "\"";
static string CARET                = "^";
static string ESCAPED_QUOTE_DOUBLE = "\\\"";
static string ESCAPED_SLASH_BACK   = "\\\\";
static string CURLYBRACE_OPEN      = "{";
static string CURLYBRACE_CLOSE     = "}";

string escapedString(const string& str, bool swap_backslash = false);

void processIncludes(string filename, string& out);
void processIncludes(ArchiveEntry* entry, string& out, bool use_res = true);

bool isInteger(const string& str, bool allow_hex = true);
bool isHex(const string& str);
bool isFloat(const string& str);

int    toInt(const string& str);
float  toFloat(const string& str);
double toDouble(const string& str);
} // namespace StringUtils
