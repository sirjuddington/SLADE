
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    JsonUtils.cpp
// Description: Various JSON-related utility functions, mostly to keep JSON
//              parsing and writing consistent (eg. always allow comments, log
//              errors, etc. etc.)
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
#include "JsonUtils.h"
#include "FileUtils.h"
#include <nlohmann/json.hpp>

using namespace slade;
using namespace jsonutil;


// -----------------------------------------------------------------------------
//
// jsonutil Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Parses a JSON string [json] and returns the resulting json object.
// If parsing fails, logs an error and returns value_t::discarded
// -----------------------------------------------------------------------------
json jsonutil::parse(string_view json)
{
	try
	{
		return json::parse(json, nullptr, true, true);
	}
	catch (const json::parse_error& ex)
	{
		auto msg = string{ ex.what() };
		log::error("Error parsing JSON: {}", msg);
		return nlohmann::detail::value_t::discarded;
	}
}

// -----------------------------------------------------------------------------
// Parses [mc] as a JSON string and returns the resulting json object.
// If parsing fails, logs an error and returns value_t::discarded
// -----------------------------------------------------------------------------
json jsonutil::parse(const MemChunk& mc)
{
	return parse(mc.asString());
}

// -----------------------------------------------------------------------------
// Parses a JSON file at [path] and returns the resulting json object.
// If parsing fails or the file cannot be opened or read, logs an error and
// returns value_t::discarded
// -----------------------------------------------------------------------------
json jsonutil::parseFile(string_view path)
{
	if (SFile file(path); file.isOpen())
		return parseFile(file);

	log::error("Unable to open or read JSON file {}", path);
	return nlohmann::detail::value_t::discarded;
}

// -----------------------------------------------------------------------------
// Parses a JSON file [file] and returns the resulting json object.
// If parsing fails, logs an error and returns value_t::discarded
// -----------------------------------------------------------------------------
json jsonutil::parseFile(const SFile& file)
{
	try
	{
		return json::parse(file.handle(), nullptr, true, true);
	}
	catch (const json::parse_error& ex)
	{
		log::error("Error parsing JSON file {}: {}", file.path(), ex.what());
		return nlohmann::detail::value_t::discarded;
	}
}

// -----------------------------------------------------------------------------
// Writes the given json object [j] to a file at [path].
// Returns true if the file was successfully written, false otherwise
// -----------------------------------------------------------------------------
bool jsonutil::writeFile(const json& j, string_view path)
{
	SFile file(path, SFile::Mode::Write);
	if (!file.isOpen())
		return false;

	return file.writeStr(j.dump(2));
}
bool jsonutil::writeFile(const ordered_json& j, string_view path)
{
	SFile file(path, SFile::Mode::Write);
	if (!file.isOpen())
		return false;

	return file.writeStr(j.dump(2));
}
