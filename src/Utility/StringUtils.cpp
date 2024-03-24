
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    StringUtils.cpp
// Description: Various string utility functions
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "StringUtils.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Tokenizer.h"
#include "UI/WxUtils.h"
#include <charconv>
#include <regex>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::wxStringUtils
{
wxRegEx re_int1{ "^[+-]?[0-9]+[0-9]*$", wxRE_DEFAULT | wxRE_NOSUB };
wxRegEx re_int2{ "^0[0-9]+$", wxRE_DEFAULT | wxRE_NOSUB };
wxRegEx re_int3{ "^0x[0-9A-Fa-f]+$", wxRE_DEFAULT | wxRE_NOSUB };
wxRegEx re_float{ "^[-+]?[0-9]*.?[0-9]+([eE][-+]?[0-9]+)?$", wxRE_DEFAULT | wxRE_NOSUB };
} // namespace slade::wxStringUtils

namespace slade::strutil
{
std::regex re_int1{ "^[+-]?[0-9]+[0-9]*$" };
std::regex re_int2{ "^0[0-9]+$" };
std::regex re_int3{ "^0x[0-9A-Fa-f]+$" };
std::regex re_float{ "^[+-]?[0-9]+[.][0-9]*([eE][+-]?[0-9]+)?$" };
} // namespace slade::strutil


// -----------------------------------------------------------------------------
//
// strutil Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if [str] is a valid integer. If [allow_hex] is true, can also
// be a valid hex string
// -----------------------------------------------------------------------------
bool strutil::isInteger(const string& str, bool allow_hex)
{
	return std::regex_search(str, re_int1) || std::regex_search(str, re_int2)
		   || (allow_hex && std::regex_search(str, re_int3));
}

// -----------------------------------------------------------------------------
// Returns true if [str] is a valid hex string
// -----------------------------------------------------------------------------
bool strutil::isHex(const string& str)
{
	return std::regex_search(str, re_int3);
}

// -----------------------------------------------------------------------------
// Returns true if [str] is a valid floating-point number
// -----------------------------------------------------------------------------
bool strutil::isFloat(const string& str)
{
	if (str.empty() || str[0] == '$')
		return false;

	return std::regex_search(str, re_float);
}

bool strutil::equalCI(string_view left, string_view right)
{
	const auto sz = left.size();
	if (right.size() != sz)
		return false;

	for (auto a = 0u; a < sz; ++a)
		if (tolower(left[a]) != tolower(right[a]))
			return false;

	return true;
}

// This one is a bit tricky - the string_view == string_view one above should be sufficient
// bool strutil::equalCI(string_view left, const char* right)
//{
//	if (!right)
//		return false;
//	if (left.empty())
//		return right[0] == 0;
//
//	auto a = 0u;
//	auto sz = left.size();
//	while (a < sz && right[a] != 0)
//	{
//		if (tolower(left[a]) != tolower(right[a]))
//			return false;
//		++a;
//	}
//
//	return true;
//}

bool strutil::startsWith(string_view str, string_view check)
{
	return check.size() <= str.size() && str.compare(0, check.size(), check) == 0;
}

bool strutil::startsWith(string_view str, char check)
{
	return !str.empty() && str[0] == check;
}

bool strutil::startsWithCI(string_view str, string_view check)
{
	const auto c_size = check.size();
	if (c_size > str.size())
		return false;

	for (unsigned c = 0; c < c_size; ++c)
		if (tolower(str[c]) != tolower(check[c]))
			return false;

	return true;
}

bool strutil::startsWithCI(string_view str, char check)
{
	return !str.empty() && tolower(str[0]) == tolower(check);
}

bool strutil::endsWith(string_view str, string_view check)
{
	return check.size() <= str.size() && str.compare(str.size() - check.size(), check.size(), check) == 0;
}

bool strutil::endsWith(string_view str, char check)
{
	return !str.empty() && str.back() == check;
}

bool strutil::endsWithCI(string_view str, string_view check)
{
	const auto c_size = check.size();
	if (c_size > str.size())
		return false;

	const auto s_size = str.size();
	for (unsigned c = 0; c < c_size; ++c)
		if (tolower(str[s_size - c_size + c]) != tolower(check[c]))
			return false;

	return true;
}

bool endsWithCI(string_view str, char check)
{
	return !str.empty() && tolower(str.back()) == tolower(check);
}

bool strutil::contains(string_view str, char check)
{
	return str.find(check) != string::npos;
}

bool strutil::containsCI(string_view str, char check)
{
	const auto lc = tolower(check);
	for (auto c : str)
		if (tolower(c) == lc)
			return true;

	return false;
}

bool strutil::contains(string_view str, string_view check)
{
	return str.find(check) != string::npos;
}

bool strutil::containsCI(string_view str, string_view check)
{
	if (str.size() < check.size())
		return false;

	// TODO: Could be improved to avoid copying strings - it's currently unused so not a priority
	return lower(str).find(lower(check)) != string::npos;
}

bool strutil::matches(string_view str, string_view match)
{
	// Empty [match] only matches empty [str]
	if (match.empty())
		return str.empty();

	unsigned m_pos = 0;
	unsigned m_start;
	unsigned t_pos = 0;
	unsigned i;
	bool     wildcard;

	while (true)
	{
		wildcard = false;

		if (m_pos == match.size())
			return t_pos == str.size();

		if (match[m_pos] == '*')
		{
			wildcard = true;
			while (match[m_pos] == '*')
			{
				++m_pos;
				if (m_pos == match.size())
					return true; // [match] ends with *, rest of [str] doesn't matter
			}
		}

		// Get part of [match] to check
		m_start = m_pos;
		while (m_pos < match.size() && match[m_pos] != '*')
			++m_pos;

		// Find above part of [match] in [str], beginning at t_pos
		i = 0;
		while (i < m_pos - m_start)
		{
			if (t_pos == str.size())
				return false; // Not found, no match

			if (match[m_start + i] == '?' || str[t_pos] == match[m_start + i])
				++i;
			else if (wildcard)
				i = 0;
			else
				return false;

			++t_pos;
		}
	}
}

bool strutil::matchesCI(string_view str, string_view match)
{
	// Empty [match] only matches empty [str]
	if (match.empty())
		return str.empty();

	unsigned m_pos = 0;
	unsigned m_start;
	unsigned t_pos = 0;
	unsigned i;
	bool     wildcard;

	while (true)
	{
		wildcard = false;

		if (m_pos == match.size())
			return t_pos == str.size();

		if (match[m_pos] == '*')
		{
			wildcard = true;
			while (match[m_pos] == '*')
			{
				++m_pos;
				if (m_pos == match.size())
					return true; // [match] ends with *, rest of [str] doesn't matter
			}
		}

		// Get part of [match] to check
		m_start = m_pos;
		while (m_pos < match.size() && match[m_pos] != '*')
			++m_pos;

		// Find above part of [match] in [str], beginning at t_pos
		i = 0;
		while (i < m_pos - m_start)
		{
			if (t_pos == str.size())
				return false; // Not found, no match

			if (match[m_start + i] == '?' || tolower(str[t_pos]) == tolower(match[m_start + i]))
				++i;
			else if (wildcard)
				i = 0;
			else
				return false;

			++t_pos;
		}
	}
}

// -----------------------------------------------------------------------------
// StringUtils::escapedString
//
// Returns a copy of [str] with escaped double quotes and backslashes.
// If [swap_backslash] is true, instead of escaping it will swap backslashes to
// forward slashes
// -----------------------------------------------------------------------------
string strutil::escapedString(string_view str, bool swap_backslash, bool escape_backslash)
{
	auto escaped = string{ str };

	if (escape_backslash)
		replaceIP(escaped, SLASH_BACK, swap_backslash ? SLASH_FORWARD : ESCAPED_SLASH_BACK);

	replaceIP(escaped, QUOTE_DOUBLE, ESCAPED_QUOTE_DOUBLE);

	return escaped;
}

string& strutil::replaceIP(string& str, string_view from, string_view to)
{
	size_t start_pos = 0;
	while ((start_pos = str.find(from.data(), start_pos)) != string::npos)
	{
		str.replace(start_pos, from.length(), to.data(), to.size());
		start_pos += to.length();
	}
	return str;
}

string strutil::replace(string_view str, string_view from, string_view to)
{
	auto s = string{ str };
	replaceIP(s, from, to);
	return s;
}

string& strutil::replaceFirstIP(string& str, string_view from, string_view to)
{
	const auto pos = str.find(from.data(), 0);
	if (pos != string::npos)
		str.replace(pos, from.length(), to.data(), to.size());
	return str;
}

string strutil::replaceFirst(string_view str, string_view from, string_view to)
{
	auto s = string{ str };
	replaceFirstIP(s, from, to);
	return s;
}

string& strutil::lowerIP(string& str)
{
	transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

string& strutil::upperIP(string& str)
{
	transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}

string strutil::lower(string_view str)
{
	auto s = string{ str };
	transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

string strutil::upper(string_view str)
{
	auto s = string{ str };
	transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

string& strutil::ltrimIP(string& str)
{
	str.erase(0, str.find_first_not_of(WHITESPACE_CHARACTERS));
	return str;
}

string& strutil::rtrimIP(string& str)
{
	str.erase(str.find_last_not_of(WHITESPACE_CHARACTERS) + 1);
	return str;
}

string& strutil::trimIP(string& str)
{
	str.erase(0, str.find_first_not_of(WHITESPACE_CHARACTERS)); // left
	str.erase(str.find_last_not_of(WHITESPACE_CHARACTERS) + 1); // right
	return str;
}

string strutil::ltrim(string_view str)
{
	auto s = string{ str };
	s.erase(0, s.find_first_not_of(WHITESPACE_CHARACTERS));
	return s;
}

string strutil::rtrim(string_view str)
{
	auto s = string{ str };
	s.erase(s.find_last_not_of(WHITESPACE_CHARACTERS) + 1);
	return s;
}

string strutil::trim(string_view str)
{
	auto s = string{ str };
	s.erase(0, s.find_first_not_of(WHITESPACE_CHARACTERS)); // left
	s.erase(s.find_last_not_of(WHITESPACE_CHARACTERS) + 1); // right
	return s;
}

string& strutil::capitalizeIP(string& str)
{
	if (str.empty())
		return str;

	transform(str.begin(), str.end(), str.begin(), tolower);
	str[0] = static_cast<char>(toupper(str[0]));

	return str;
}

string strutil::capitalize(string_view str)
{
	if (str.empty())
		return {};

	auto s = string{ str };
	transform(s.begin(), s.end(), s.begin(), tolower);
	s[0] = static_cast<char>(toupper(s[0]));
	return s;
}

string strutil::wildcardToRegex(string_view str)
{
	// Process [str] to be a valid regex string
	// (replace ? with . and add . before any *)
	auto regex = string{ str };
	for (unsigned c = 0; c < regex.size(); ++c)
	{
		if (regex[c] == '?')
			regex[c] = '.';
		else if (regex[c] == '*')
		{
			regex.insert(regex.begin() + c, '.');
			++c;
		}
	}

	return regex;
}

string strutil::prepend(string_view str, string_view prefix)
{
	string s{ str.data(), str.size() };
	s.insert(s.begin(), prefix.begin(), prefix.end());
	return s;
}

string& strutil::prependIP(string& str, string_view prefix)
{
	str.insert(str.begin(), prefix.begin(), prefix.end());
	return str;
}

string strutil::left(string_view str, unsigned n)
{
	return string{ str.substr(0, n) };
}

string_view strutil::leftV(string_view str, unsigned n)
{
	return str.substr(0, n);
}

string strutil::right(string_view str, unsigned n)
{
	if (str.size() <= n)
		return string{ str };

	return string{ str.substr(str.size() - n, n) };
}

string_view strutil::rightV(string_view str, unsigned n)
{
	if (str.size() <= n)
		return str;

	return str.substr(str.size() - n, n);
}

string strutil::afterLast(string_view str, char chr)
{
	for (int i = static_cast<int>(str.size()) - 1; i >= 0; --i)
		if (str[i] == chr)
			return string{ str.substr(i + 1) };

	return string{ str };
}

string_view strutil::afterLastV(string_view str, char chr)
{
	for (int i = static_cast<int>(str.size()) - 1; i >= 0; --i)
		if (str[i] == chr)
			return str.substr(i + 1);

	return str;
}

string strutil::afterFirst(string_view str, char chr)
{
	for (unsigned i = 0; i < str.size(); ++i)
		if (str[i] == chr)
			return string{ str.substr(i + 1) };

	return string{ str };
}

string_view strutil::afterFirstV(string_view str, char chr)
{
	for (unsigned i = 0; i < str.size(); ++i)
		if (str[i] == chr)
			return str.substr(i + 1);

	return str;
}

string strutil::beforeLast(string_view str, char chr)
{
	for (int i = static_cast<int>(str.size()) - 1; i >= 0; --i)
		if (str[i] == chr)
			return string{ str.substr(0, i) };

	return string{ str };
}

string_view strutil::beforeLastV(string_view str, char chr)
{
	for (int i = static_cast<int>(str.size()) - 1; i >= 0; --i)
		if (str[i] == chr)
			return str.substr(0, i);

	return str;
}

string strutil::beforeFirst(string_view str, char chr)
{
	for (unsigned i = 0; i < str.size(); i++)
		if (str[i] == chr)
			return string{ str.substr(0, i) };

	return string{ str };
}

string_view strutil::beforeFirstV(string_view str, char chr)
{
	for (unsigned i = 0; i < str.size(); i++)
		if (str[i] == chr)
			return str.substr(0, i);

	return str;
}

vector<string> strutil::split(string_view str, char separator)
{
	unsigned       start = 0;
	const auto     size  = str.size();
	vector<string> split;
	for (unsigned c = 0; c < size; ++c)
	{
		if (str[c] == separator)
		{
			split.emplace_back(str.substr(start, c - start));
			start = c + 1;
		}
	}

	split.emplace_back(str.substr(start, size - start));

	return split;
}

vector<string_view> strutil::splitV(string_view str, char separator)
{
	unsigned            start = 0;
	const auto          size  = str.size();
	vector<string_view> split;
	for (unsigned c = 0; c < size; ++c)
	{
		if (str[c] == separator)
		{
			split.push_back(str.substr(start, c - start));
			start = c + 1;
		}
	}

	split.push_back(str.substr(start, size - start));

	return split;
}

string strutil::truncate(string_view str, unsigned length)
{
	if (str.size() > length)
		return { str.data(), length };
	else
		return { str.data(), str.size() };
}

string& strutil::truncateIP(string& str, unsigned length)
{
	if (str.size() > length)
		str.resize(length);
	return str;
}

string strutil::removeLast(string_view str, unsigned n)
{
	if (str.size() <= n)
		return {};

	return { str.data(), str.size() - n };
}

string& strutil::removeLastIP(string& str, unsigned n)
{
	if (str.size() <= n)
		str.clear();
	else
		str.resize(str.size() - n);

	return str;
}

string strutil::removePrefix(string_view str, char prefix)
{
	if (!str.empty() && str[0] == prefix)
		str.remove_prefix(1);
	return { str.data(), str.size() };
}

string& strutil::removePrefixIP(string& str, char prefix)
{
	if (!str.empty() && str[0] == prefix)
		str.erase(0, 1);
	return str;
}

string strutil::removeSuffix(string_view str, char suffix)
{
	if (!str.empty() && str.back() == suffix)
		str.remove_suffix(1);
	return { str.data(), str.size() };
}

string& strutil::removeSuffixIP(string& str, char suffix)
{
	if (!str.empty() && str.back() == suffix)
		str.pop_back();
	return str;
}

strutil::Path::Path(string_view full_path) : full_path_{ full_path.data(), full_path.size() }
{
	// Enforce / as separators
	std::replace(full_path_.begin(), full_path_.end(), '\\', '/');

	const auto last_sep_pos = full_path_.find_last_of('/');
	filename_start_         = last_sep_pos == string::npos ? 0 : last_sep_pos + 1;
	const auto ext_pos      = full_path_.find_last_of('.');
	filename_end_           = ext_pos == string::npos ? full_path_.size() : ext_pos;
}

string_view strutil::Path::path(bool include_end_sep) const
{
	if (filename_start_ == 0 || filename_start_ == string::npos)
		return {};

	return include_end_sep ? string_view{ full_path_.data(), filename_start_ } :
							 string_view{ full_path_.data(), filename_start_ - 1 };
}

string_view strutil::Path::fileName(bool include_extension) const
{
	if (filename_start_ == string::npos)
		return {};

	return include_extension ? string_view{ full_path_.data() + filename_start_ } :
							   string_view{ full_path_.data() + filename_start_, filename_end_ - filename_start_ };
}

string_view strutil::Path::extension() const
{
	if (filename_start_ == string::npos || filename_end_ >= full_path_.size())
		return {};

	return string_view{ full_path_.data() + filename_end_ + 1 };
}

vector<string_view> strutil::Path::pathParts() const
{
	if (filename_start_ == 0 || filename_start_ == string::npos)
		return {};

	return splitV({ full_path_.data(), filename_start_ - 1 }, '/');
}

bool strutil::Path::hasExtension() const
{
	return filename_start_ != string::npos && filename_end_ < full_path_.size();
}

void strutil::Path::set(string_view full_path)
{
	full_path_ = full_path;

	// Enforce / as separators
	std::replace(full_path_.begin(), full_path_.end(), '\\', '/');

	const auto last_sep_pos = full_path_.find_last_of('/');
	filename_start_         = last_sep_pos == string::npos ? 0 : last_sep_pos + 1;
	const auto ext_pos      = full_path_.find_last_of('.');
	filename_end_           = ext_pos == string::npos ? full_path_.size() : ext_pos;
}

void strutil::Path::setPath(string_view path)
{
	if (filename_start_ == string::npos)
		return;

	// Ensure given path doesn't begin or end with a separator
	if (path.back() == '/' || path.back() == '\\')
		path.remove_suffix(1);
	if (path[0] == '/' || path[0] == '\\')
		path.remove_prefix(1);

	if (filename_start_ == 0)
	{
		// No path, insert path and separator at beginning
		full_path_.insert(full_path_.begin(), '/');
		full_path_.insert(full_path_.begin(), path.begin(), path.end());
		filename_start_ += path.size() + 1;
		filename_end_ += path.size() + 1;
	}
	else
	{
		full_path_.replace(0, filename_start_ - 1, path.data(), path.size());
		const auto fn_start_old = filename_start_;
		filename_start_         = path.size() + 1;
		filename_end_ -= fn_start_old - filename_start_;
	}
}

void strutil::Path::setPath(const vector<string_view>& parts) {}

void strutil::Path::setPath(const vector<string>& parts) {}

void strutil::Path::setFileName(string_view file_name)
{
	if (filename_start_ == string::npos)
		return;

	if (filename_start_ >= full_path_.size())
	{
		filename_start_ = full_path_.size();
		full_path_.append(file_name.data(), file_name.size());
		filename_end_ = full_path_.size();
	}
	else
	{
		full_path_.replace(filename_start_, filename_end_ - filename_start_, file_name.data(), file_name.size());
		filename_end_ = filename_start_ + file_name.size();
	}

	// Sanitize new filename
	std::replace(full_path_.begin() + filename_start_, full_path_.begin() + filename_end_, '/', '_');
	std::replace(full_path_.begin() + filename_start_, full_path_.begin() + filename_end_, '\\', '_');
	std::replace(full_path_.begin() + filename_start_, full_path_.begin() + filename_end_, '.', '_');
}

void strutil::Path::setExtension(string_view extension)
{
	if (extension.empty())
	{
		full_path_.erase(filename_end_);
		return;
	}

	if (filename_end_ < full_path_.size())
		full_path_.erase(filename_end_ + 1);
	else
		full_path_.push_back('.');

	full_path_.append(extension.data(), extension.size());

	// Sanitize new extension
	std::replace(full_path_.begin() + filename_end_, full_path_.end(), '/', '_');
	std::replace(full_path_.begin() + filename_end_, full_path_.end(), '\\', '_');
}

string_view strutil::Path::fileNameOf(string_view full_path, bool include_extension)
{
	int pos;
	for (pos = static_cast<int>(full_path.size()) - 1; pos > 0; --pos)
		if (full_path[pos - 1] == '/' || full_path[pos - 1] == '\\')
			break;

	if (!include_extension)
	{
		const auto ext_pos = full_path.find_last_of('.');
		if (ext_pos != string_view::npos)
			full_path.remove_suffix(full_path.size() - ext_pos);
	}

	return pos > 0 ? full_path.substr(pos) : full_path;
}

string_view strutil::Path::extensionOf(string_view full_path)
{
	const auto ext_pos = full_path.find_last_of('.');
	return ext_pos == string_view::npos ? string_view{} : full_path.substr(ext_pos + 1);
}

string_view strutil::Path::pathOf(string_view full_path, bool include_end_sep)
{
	const auto last_sep_pos = full_path.find_last_of("/\\");
	return last_sep_pos == string_view::npos ? string_view{} :
											   full_path.substr(0, include_end_sep ? last_sep_pos + 1 : last_sep_pos);
}

bool strutil::Path::filePathsMatch(string_view left, string_view right)
{
	const auto sz = left.size();
	if (right.size() != sz)
		return false;

	if (app::platform() == app::Platform::Linux)
	{
		for (auto a = 0u; a < sz; ++a)
		{
			// Backslash and forward-slash are the same
			if (left[a] == '\\' && right[a] == '/' || left[a] == '/' && right[a] == '\\')
				continue;

			// Case-sensitive
			if (left[a] != right[a])
				return false;
		}
	}
	else
	{
		for (auto a = 0u; a < sz; ++a)
		{
			// Backslash and forward-slash are the same
			if (left[a] == '\\' && right[a] == '/' || left[a] == '/' && right[a] == '\\')
				continue;

			// Case-insensitive
			if (tolower(left[a]) != tolower(right[a]))
				return false;
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// StringUtils::processIncludes
//
// Reads the text file at [filename], processing any #include statements in the
// file recursively. The resulting 'expanded' text is written to [out]
// -----------------------------------------------------------------------------
void strutil::processIncludes(const string& filename, string& out)
{
	// Open file
	wxTextFile file;
	if (!file.Open(filename))
		return;

	// Get file path
	const Path fn(filename);
	auto       path = fn.path(true);

	// Go through line-by-line
	string    line = file.GetNextLine().ToStdString();
	Tokenizer tz;
	tz.setSpecialCharacters("");
	while (!file.Eof())
	{
		// Check for #include
		if (startsWithCI(ltrim(line), "#include"))
		{
			// Get filename to include
			tz.openString(line);
			tz.adv(); // Skip #include

			// Process the file
			processIncludes(fmt::format("{}{}", path, tz.next().text), out);
		}
		else
			out.append(line + "\n");

		line = file.GetNextLine();
	}
}

// -----------------------------------------------------------------------------
// StringUtils::processIncludes
//
// Reads the text entry [entry], processing any #include statements in the
// entry text recursively. This will search in the resource folder and archive
// as well as in the parent archive. The resulting 'expanded' text is written
// to [out]
// -----------------------------------------------------------------------------
void strutil::processIncludes(const ArchiveEntry* entry, string& out, bool use_res)
{
	// Check entry was given
	if (!entry)
		return;

	// Write entry to temp file
	const auto filename = app::path(entry->name(), app::Dir::Temp);
	entry->exportFile(filename);

	// Open file
	wxTextFile file;
	if (!file.Open(filename))
		return;

	// Go through line-by-line
	Tokenizer tz;
	tz.setSpecialCharacters("");
	auto line = file.GetFirstLine().ToStdString();
	while (!file.Eof())
	{
		// Check for #include
		if (startsWithCI(ltrim(line), "#include"))
		{
			// Get name of entry to include
			tz.openString(line);
			auto name = entry->path() + tz.next().text;

			// Get the entry
			bool          done      = false;
			ArchiveEntry* entry_inc = entry->parent()->entryAtPath(name);
			// DECORATE paths start from the root, not from the #including entry's directory
			if (!entry_inc)
				entry_inc = entry->parent()->entryAtPath(tz.current().text);
			if (entry_inc)
			{
				processIncludes(entry_inc, out);
				done = true;
			}
			else
				log::warning(2, fmt::format("Couldn't find entry to #include: {}", name));

			// Look in resource pack
			if (use_res && !done && app::archiveManager().programResourceArchive())
			{
				name      = "config/games/" + tz.current().text;
				entry_inc = app::archiveManager().programResourceArchive()->entryAtPath(name);
				if (entry_inc)
				{
					processIncludes(entry_inc, out);
					done = true;
				}
			}

			// Okay, we've exhausted all possibilities
			if (!done)
				log::error(
					fmt::format("Attempting to #include nonexistant entry \"{}\" from entry {}", name, entry->name()));
		}
		else
			out.append(line + "\n");

		line = file.GetNextLine();
	}

	// Delete temp file
	wxRemoveFile(filename);
}

int strutil::asInt(string_view str, int base)
{
	int        val    = 0;
	const auto result = std::from_chars(str.data(), str.data() + str.size(), val, base);
	if (result.ec == std::errc::invalid_argument)
		log::error("Can't convert \"{}\" to an integer (invalid)", str);
	else if (result.ec == std::errc::result_out_of_range)
		log::error("Can't convert \"{}\" to an integer (out of range)", str);

	return val;
}

unsigned strutil::asUInt(string_view str, int base)
{
	unsigned   val    = 0;
	const auto result = std::from_chars(str.data(), str.data() + str.size(), val, base);
	if (result.ec == std::errc::invalid_argument)
		log::error("Can't convert \"{}\" to an unsigned integer (invalid)", str);
	else if (result.ec == std::errc::result_out_of_range)
		log::error("Can't convert \"{}\" to an unsigned integer (out of range)", str);

	return val;
}

float strutil::asFloat(string_view str)
{
	float val = 0;

#ifdef _MSC_VER
	const auto result = std::from_chars(str.data(), str.data() + str.size(), val);
	if (result.ec == std::errc::invalid_argument)
		log::error("Can't convert \"{}\" to a float (invalid)", str);
	else if (result.ec == std::errc::result_out_of_range)
		log::error("Can't convert \"{}\" to a float (out of range)", str);
#else
	// TODO: Remove this once non-MSVC compilers support std::from_chars with float
	try
	{
		val = std::stof(string{ str });
	}
	catch (const std::exception& ex)
	{
		log::error("Can't convert \"{}\" to a float ({})", str, ex.what());
		return 0.f;
	}
#endif

	return val;
}

double strutil::asDouble(string_view str)
{
	double val = 0;

#ifdef _MSC_VER
	const auto result = std::from_chars(str.data(), str.data() + str.size(), val);
	if (result.ec == std::errc::invalid_argument)
		log::error("Can't convert \"{}\" to a double (invalid)", str);
	else if (result.ec == std::errc::result_out_of_range)
		log::error("Can't convert \"{}\" to a double (out of range)", str);
#else
	// TODO: Remove this once non-MSVC compilers support std::from_chars with double
	try
	{
		val = std::stod(string{ str });
	}
	catch (const std::exception& ex)
	{
		log::error("Can't convert \"{}\" to a double ({})", str, ex.what());
		return 0.;
	}
#endif

	return val;
}

bool strutil::asBoolean(string_view str)
{
	// Empty, 0 or "false" are false, everything else true
	return !(str.empty() || str == "0" || equalCI(str, "false"));
}

bool strutil::toInt(string_view str, int& target, int base)
{
	const auto result = std::from_chars(str.data(), str.data() + str.size(), target, base);

	if (result.ec == std::errc::invalid_argument)
	{
		log::error("Can't convert \"{}\" to an integer (invalid)", str);
		return false;
	}
	if (result.ec == std::errc::result_out_of_range)
	{
		log::error("Can't convert \"{}\" to an integer (out of range)", str);
		return false;
	}

	return true;
}

bool strutil::toUInt(string_view str, unsigned& target, int base)
{
	const auto result = std::from_chars(str.data(), str.data() + str.size(), target, base);

	if (result.ec == std::errc::invalid_argument)
	{
		log::error("Can't convert \"{}\" to an unsigned integer (invalid)", str);
		return false;
	}
	if (result.ec == std::errc::result_out_of_range)
	{
		log::error("Can't convert \"{}\" to an unsigned integer (out of range)", str);
		return false;
	}

	return true;
}

bool strutil::toFloat(string_view str, float& target)
{
#ifdef _MSC_VER
	const auto result = std::from_chars(str.data(), str.data() + str.size(), target);

	if (result.ec == std::errc::invalid_argument)
	{
		log::error("Can't convert \"{}\" to a float (invalid)", str);
		return false;
	}
	if (result.ec == std::errc::result_out_of_range)
	{
		log::error("Can't convert \"{}\" to a float (out of range)", str);
		return false;
	}
#else
	// TODO: Remove this once non-MSVC compilers support std::from_chars with float
	try
	{
		target = std::stof(string{ str });
	}
	catch (const std::exception& ex)
	{
		log::error("Can't convert \"{}\" to a float ({})", str, ex.what());
		return false;
	}
#endif

	return true;
}

bool strutil::toDouble(string_view str, double& target)
{
#ifdef _MSC_VER
	const auto result = std::from_chars(str.data(), str.data() + str.size(), target);

	if (result.ec == std::errc::invalid_argument)
	{
		log::error("Can't convert \"{}\" to a double (invalid)", str);
		return false;
	}
	if (result.ec == std::errc::result_out_of_range)
	{
		log::error("Can't convert \"{}\" to a double (out of range)", str);
		return false;
	}
#else
	// TODO: Remove this once non-MSVC compilers support std::from_chars with double
	try
	{
		target = std::stod(string{ str });
	}
	catch (const std::exception& ex)
	{
		log::error("Can't convert \"{}\" to a double ({})", str, ex.what());
		return false;
	}
#endif

	return true;
}

// -----------------------------------------------------------------------------
// Converts a string_view [str] to a string, where [str] can be null-terminated.
// If not null-terminated, the size of the string_view is used instead.
// -----------------------------------------------------------------------------
string strutil::toString(string_view str)
{
	const auto end = str.find('\0');
	return string{ str.data(), end == string_view::npos ? str.size() : end };
}

// -----------------------------------------------------------------------------
// Creates and returns a string_view from the given [chars].
// The size of the string_view is either the position of the first found null in
// [chars], or [max_length], whatever comes first.
// -----------------------------------------------------------------------------
string_view strutil::viewFromChars(const char* chars, unsigned max_length)
{
	string_view::size_type size = 0;
	while (size < max_length)
	{
		if (chars[size] == '\0')
			break;

		++size;
	}

	return { chars, size };
}

// -----------------------------------------------------------------------------
// Encodes [str] to UTF8
// -----------------------------------------------------------------------------
string strutil::toUTF8(string_view str)
{
	return wxutil::strFromView(str).ToUTF8().data();
}

// -----------------------------------------------------------------------------
// Decodes [str] from UTF8
// -----------------------------------------------------------------------------
string strutil::fromUTF8(string_view str)
{
	return wxString::FromUTF8(str.data(), str.length()).ToStdString();
}

// -----------------------------------------------------------------------------
// Splits [str] into tokens, using given [options]
// -----------------------------------------------------------------------------
vector<strutil::Token> strutil::tokenize(const string& str, const TokenizeOptions& options)
{
	vector<Token> tokens;
	tokenize(tokens, str, options);
	return tokens;
}

// -----------------------------------------------------------------------------
// Splits [str] into [tokens], using given [options]
// -----------------------------------------------------------------------------
void strutil::tokenize(vector<Token>& tokens, const string& str, const TokenizeOptions& options)
{
	auto     end           = str.size();
	auto     pos           = 0u;
	auto     line_no       = 1u;
	auto     line_comment  = false;
	auto     block_comment = 0; // 0=none, 1=c, 2=lua
	unsigned token_start   = end;

	while (pos < end)
	{
		auto& current = str[pos];

		// Newline
		if (current == '\n')
		{
			// Add current token if needed
			if (token_start < end)
			{
				tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });
				token_start = end;
			}

			++line_no;
			++pos;
			line_comment = false;

			continue;
		}

		// Within line comment
		if (line_comment)
		{
			++pos;
			continue;
		}

		// Within block comment
		if (block_comment > 0)
		{
			if ((block_comment == 1 && current == '*' && pos + 1 < end && str[pos + 1] == '/') || // C-style
				(block_comment == 2 && current == ']' && pos + 1 < end && str[pos + 1] == ']'))   // Lua
			{
				pos += 2;
				block_comment = 0;
			}
			else
				++pos;

			continue;
		}

		// Whitespace
		// if (current == ' ' || current == '\t' || current == '\r' || current == '\f' || current == '\v')
		if (options.whitespace_characters.find(current) != string::npos)
		{
			// Add current token if needed
			if (token_start < end)
			{
				tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });
				token_start = end;
			}

			++pos;
			continue;
		}

		// C-style block comment
		if (options.comments_cstyle && current == '/' && pos + 1 < end && str[pos + 1] == '*')
		{
			// Add current token if needed
			if (token_start < end)
			{
				tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });
				token_start = end;
			}

			pos += 2;
			block_comment = 1;
			continue;
		}

		// Lua-style comment (block or line)
		if (options.comments_lua && current == '-' && pos + 1 < end && str[pos + 1] == '-')
		{
			// Add current token if needed
			if (token_start < end)
			{
				tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });
				token_start = end;
			}

			pos += 2;

			// Check for block comment (--[[)
			if (pos + 1 < end && str[pos] == '[' && str[pos + 1] == '[')
			{
				block_comment = 2;
				pos += 2;
			}
			else
				line_comment = true;

			continue;
		}

		// CPP-style line comment
		if (options.comments_cppstyle && current == '/' && pos + 1 < end && str[pos + 1] == '/')
		{
			// Add current token if needed
			if (token_start < end)
			{
				tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });
				token_start = end;
			}

			pos += 2;

			// Check for DB editor comment
			if (options.decorate && pos < end && str[pos] == '$')
			{
				// Read to the end of the line
				token_start = pos - 2;
				while (++pos < end)
					if (str[pos] == '\n')
						break;

				// Add as token
				tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });
				token_start = end;
			}
			else
				line_comment = true;

			continue;
		}

		// Double hash line comment
		if (options.comments_doublehash && current == '#' && pos + 1 < end && str[pos + 1] == '#')
		{
			// Add current token if needed
			if (token_start < end)
			{
				tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });
				token_start = end;
			}

			pos += 2;
			line_comment = true;
			continue;
		}

		// Single hash line comment
		if (options.comments_hash && current == '#')
		{
			// Add current token if needed
			if (token_start < end)
			{
				tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });
				token_start = end;
			}

			++pos;
			line_comment = true;
			continue;
		}

		// Shell line comment
		if (options.comments_shell && current == ';')
		{
			// Add current token if needed
			if (token_start < end)
			{
				tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });
				token_start = end;
			}

			++pos;
			line_comment = true;
			continue;
		}

		// Quoted string
		if (current == '"' && pos + 1 < end)
		{
			// Add current token if needed
			if (token_start < end)
				tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });

			++pos;
			token_start = pos;
			while (pos < end)
			{
				if (str[pos] == '"')
				{
					tokens.push_back({ { str.data() + token_start, pos - token_start }, true, line_no, token_start });
					++pos;
					break;
				}
				if (str[pos] == options.string_escape)
					++pos;

				++pos;
			}

			token_start = end;
			continue;
		}

		// Special character
		if (options.special_characters.find(current) != string::npos)
		{
			// Add current token if needed
			if (token_start < end)
			{
				tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });
				token_start = end;
			}

			tokens.push_back({ { str.data() + pos, 1 }, false, line_no, pos });
			++pos;
			continue;
		}

		// Anything else - begin or continue token
		if (token_start == end)
			token_start = pos;

		++pos;
	}

	// Add last token if needed
	if (token_start < end)
	{
		tokens.push_back({ { str.data() + token_start, pos - token_start }, false, line_no, token_start });
		token_start = end;
	}
}


// -----------------------------------------------------------------------------
//
// wxStringUtils Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns a copy of [str] with escaped double quotes and backslashes.
// If [swap_backslash] is true, instead of escaping it will swap backslashes to
// forward slashes
// -----------------------------------------------------------------------------
wxString wxStringUtils::escapedString(const wxString& str, bool swap_backslash)
{
	wxString escaped = str;

	escaped.Replace(SLASH_BACK, swap_backslash ? SLASH_FORWARD : ESCAPED_SLASH_BACK);
	escaped.Replace(QUOTE_DOUBLE, ESCAPED_QUOTE_DOUBLE);

	return escaped;
}

// -----------------------------------------------------------------------------
// Reads the text file at [filename], processing any #include statements in the
// file recursively. The resulting 'expanded' text is written to [out]
// -----------------------------------------------------------------------------
void wxStringUtils::processIncludes(const wxString& filename, wxString& out)
{
	// Open file
	wxTextFile file;
	if (!file.Open(filename))
		return;

	// Get file path
	wxFileName fn(filename);
	wxString   path = fn.GetPath(true);

	// Go through line-by-line
	wxString  line = file.GetNextLine();
	Tokenizer tz;
	tz.setSpecialCharacters("");
	while (!file.Eof())
	{
		// Check for #include
		if (line.Lower().Trim().StartsWith("#include"))
		{
			// Get filename to include
			tz.openString(line.ToStdString());
			tz.adv(); // Skip #include

			// Process the file
			processIncludes(path + tz.next().text, out);
		}
		else
			out.Append(line + "\n");

		line = file.GetNextLine();
	}
}

// -----------------------------------------------------------------------------
// Reads the text entry [entry], processing any #include statements in the
// entry text recursively. This will search in the resource folder and archive
// as well as in the parent archive. The resulting 'expanded' text is written
// to [out]
// -----------------------------------------------------------------------------
void wxStringUtils::processIncludes(const ArchiveEntry* entry, wxString& out, bool use_res)
{
	// Check entry was given
	if (!entry)
		return;

	// Write entry to temp file
	const auto filename = app::path(entry->name(), app::Dir::Temp);
	entry->exportFile(filename);

	// Open file
	wxTextFile file;
	if (!file.Open(filename))
		return;

	// Go through line-by-line
	Tokenizer tz;
	tz.setSpecialCharacters("");
	wxString line = file.GetFirstLine();
	while (!file.Eof())
	{
		// Check for #include
		if (line.Lower().Trim().StartsWith("#include"))
		{
			// Get name of entry to include
			tz.openString(line.ToStdString());
			wxString name = entry->path() + tz.next().text;

			// Get the entry
			bool done      = false;
			auto entry_inc = entry->parent()->entryAtPath(name.ToStdString());
			// DECORATE paths start from the root, not from the #including entry's directory
			if (!entry_inc)
				entry_inc = entry->parent()->entryAtPath(tz.current().text);
			if (entry_inc)
			{
				processIncludes(entry_inc, out);
				done = true;
			}
			else
				log::info(2, wxString::Format("Couldn't find entry to #include: %s", name));

			// Look in resource pack
			if (use_res && !done && app::archiveManager().programResourceArchive())
			{
				name      = "config/games/" + tz.current().text;
				entry_inc = app::archiveManager().programResourceArchive()->entryAtPath(name.ToStdString());
				if (entry_inc)
				{
					processIncludes(entry_inc, out);
					done = true;
				}
			}

			// Okay, we've exhausted all possibilities
			if (!done)
				log::info(
					1,
					wxString::Format(
						"Error: Attempting to #include nonexistant entry \"%s\" from entry %s", name, entry->name()));
		}
		else
			out.Append(line + "\n");

		line = file.GetNextLine();
	}

	// Delete temp file
	wxRemoveFile(filename);
}

// -----------------------------------------------------------------------------
// Returns true if [str] is a valid integer. If [allow_hex] is true, can also
// be a valid hex string
// -----------------------------------------------------------------------------
bool wxStringUtils::isInteger(const wxString& str, bool allow_hex)
{
	return (re_int1.Matches(str) || re_int2.Matches(str) || (allow_hex && re_int3.Matches(str)));
}

// -----------------------------------------------------------------------------
// Returns true if [str] is a valid hex string
// -----------------------------------------------------------------------------
bool wxStringUtils::isHex(const wxString& str)
{
	return re_int3.Matches(str);
}

// -----------------------------------------------------------------------------
// Returns true if [str] is a valid floating-point number
// -----------------------------------------------------------------------------
bool wxStringUtils::isFloat(const wxString& str)
{
	return (re_float.Matches(str));
}

// -----------------------------------------------------------------------------
// Returns [str] as an integer, or 0 if it can't be converted
// -----------------------------------------------------------------------------
int wxStringUtils::toInt(const wxString& str)
{
	long tmp;
	if (str.ToLong(&tmp))
		return tmp;

	log::error(wxString::Format("Can't convert \"%s\" to an integer", str));
	return 0;
}

// -----------------------------------------------------------------------------
// Returns [str] as a float, or 0 if it can't be converted
// -----------------------------------------------------------------------------
float wxStringUtils::toFloat(const wxString& str)
{
	double tmp;
	if (str.ToDouble(&tmp))
		return static_cast<float>(tmp);

	log::error(wxString::Format("Can't convert \"%s\" to a float", str));
	return 0.f;
}

// -----------------------------------------------------------------------------
// Returns [str] as a double, or 0 if it can't be converted
// -----------------------------------------------------------------------------
double wxStringUtils::toDouble(const wxString& str)
{
	double tmp;
	if (str.ToDouble(&tmp))
		return tmp;

	log::error(wxString::Format("Can't convert \"%s\" to a double", str));
	return 0.;
}
