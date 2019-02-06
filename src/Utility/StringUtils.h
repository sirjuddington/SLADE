#pragma once

class ArchiveEntry;

namespace StringUtils
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

void processIncludes(wxString filename, wxString& out);
void processIncludes(ArchiveEntry* entry, wxString& out, bool use_res = true);

bool isInteger(const wxString& str, bool allow_hex = true);
bool isHex(const wxString& str);
bool isFloat(const wxString& str);

int    toInt(const wxString& str);
float  toFloat(const wxString& str);
double toDouble(const wxString& str);
} // namespace StringUtils
