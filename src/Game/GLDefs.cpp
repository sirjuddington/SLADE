
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GLDefs.cpp
// Description: *ZDoom GLDEFS parser and handler
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
#include "GLDefs.h"
#include "Archive/Archive.h"
#include "Archive/EntryType/EntryType.h"
#include "SLADEMap/MapSpecials/PointLights.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace game;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Logs a GLDEFS parsing error [message] at [token]
// -----------------------------------------------------------------------------
void logParseError(const strutil::Token& token, const string& message)
{
	log::error("GLDEFS parse error at {}:{}: {}", token.line_no, token.position, message);
}

// -----------------------------------------------------------------------------
// Checks if there are at least [required] tokens available from [idx], and that
// they aren't closing brackets.
// If not, logs an appropriate error and returns false
// -----------------------------------------------------------------------------
bool checkRequiredTokens(const vector<strutil::Token>& tokens, unsigned idx, unsigned required = 1)
{
	if (idx + required > tokens.size())
	{
		logParseError(tokens.back(), "Unexpected end of file");
		return false;
	}

	for (unsigned i = 0; i < required; ++i)
	{
		if (tokens[idx + i].text == "}")
		{
			logParseError(tokens[idx + i], "Unexpected end of block");
			return false;
		}
	}

	return true;
}
} // namespace


// -----------------------------------------------------------------------------
//
// GLDefs Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Returns the singleton instance of GLDefs
// -----------------------------------------------------------------------------
GLDefs& GLDefs::get()
{
	static GLDefs gl_defs;
	return gl_defs;
}

// -----------------------------------------------------------------------------
// Clears all loaded GLDEFS definitions
// -----------------------------------------------------------------------------
void GLDefs::clear()
{
	lightdefs_.clear();
}

// -----------------------------------------------------------------------------
// Parses all GLDEFS definitions in [archive]
// -----------------------------------------------------------------------------
void GLDefs::parse(const Archive& archive)
{
	ArchiveSearchOptions opt;
	opt.match_type = EntryType::fromId("gldefslump");
	auto gldefs    = archive.findFirst(opt);
	if (!gldefs)
		return;

	string full_defs;
	strutil::processIncludes(gldefs, full_defs);

	auto tokens = strutil::tokenize(full_defs, { .comments_cstyle = true, .comments_cppstyle = true });

	for (unsigned idx = 0; idx < tokens.size(); ++idx)
	{
		// Point light definitions (TODO: sectorlight)
		if (strutil::equalCI(tokens[idx].text, "pointlight")
			|| strutil::equalCI(tokens[idx].text, "pulselight")
			|| strutil::equalCI(tokens[idx].text, "flickerlight")
			|| strutil::equalCI(tokens[idx].text, "flickerlight2"))
			idx = parsePointLight(tokens, idx);

		// Object binding
		else if (strutil::equalCI(tokens[idx].text, "object"))
			idx = parseObjectBinding(tokens, idx);
	}

	log::debug("Parsed {} light definitions & {} object bindings", lightdefs_.size(), object_bindings_.size());
}

// -----------------------------------------------------------------------------
// Returns a PointLight definition for [name] if it exists, at [position]
// -----------------------------------------------------------------------------
optional<map::PointLight> GLDefs::pointLight(const string& name, const Vec3d& position) const
{
	auto it = lightdefs_.find(name);
	if (it != lightdefs_.end())
	{
		map::PointLight::Type type = map::PointLight::Type::Normal;
		if (it->second.attenuated)
			type = map::PointLight::Type::Attenuated;
		else if (it->second.subtractive)
			type = map::PointLight::Type::Subtractive;

		return map::PointLight{ .thing    = nullptr,
								.position = position
											+ Vec3d(0, 0, it->second.offset.y), // TODO: x/y are relative to direction?
								.r        = static_cast<u8>(it->second.colour.r * 255),
								.g        = static_cast<u8>(it->second.colour.g * 255),
								.b        = static_cast<u8>(it->second.colour.b * 255),
								.radius   = static_cast<unsigned>(it->second.radius),
								.type     = type };
	}

	return std::nullopt;
}

// -----------------------------------------------------------------------------
// Returns the name of the light bound to an object of [class_name] and
// [sprite], or an empty string if none found
// -----------------------------------------------------------------------------
string GLDefs::boundLightName(const string& class_name, const string& sprite) const
{
	auto it = object_bindings_.find(class_name);
	if (it != object_bindings_.end())
	{
		for (const auto& binding : it->second)
		{
			if (strutil::startsWithCI(sprite, binding.frame))
				return binding.light_name;
		}
	}

	return {};
}

// -----------------------------------------------------------------------------
// Parses a point light definition from [tokens] starting at [idx]
// -----------------------------------------------------------------------------
unsigned GLDefs::parsePointLight(const vector<strutil::Token>& tokens, unsigned idx)
{
	// <type> <name>
	string type{ tokens[idx++].text };
	string light_name{ tokens[idx++].text };

	// Check for opening bracket
	if (idx >= tokens.size() || tokens[idx].text != "{")
	{
		logParseError(tokens[idx], fmt::format("Expected '{{' after pointlight definition '{}'", light_name));
		return idx;
	}

	++idx; // Skip '{' token

	// Parse light properties
	LightDef light_def;
	bool     colour = false, radius = false;
	while (idx < tokens.size() && tokens[idx].text != "}")
	{
		// color <RED> <GREEN> <BLUE>
		if (strutil::equalCI(tokens[idx].text, "color"))
		{
			if (!checkRequiredTokens(tokens, idx, 4))
				return idx;

			light_def.colour.r = strutil::asFloat(tokens[idx + 1].text);
			light_def.colour.g = strutil::asFloat(tokens[idx + 2].text);
			light_def.colour.b = strutil::asFloat(tokens[idx + 3].text);
			colour             = true;

			idx += 4; // Skip 'color <RED> <GREEN> <BLUE>' tokens
		}

		// size <SIZE>
		else if (strutil::equalCI(tokens[idx].text, "size"))
		{
			if (!checkRequiredTokens(tokens, idx, 2))
				return idx;

			light_def.radius = strutil::asInt(tokens[idx + 1].text);
			radius           = true;

			idx += 2; // Skip 'size <SIZE>' tokens
		}

		// secondarySize <SECSIZE>
		else if (strutil::equalCI(tokens[idx].text, "secondarySize"))
		{
			if (!checkRequiredTokens(tokens, idx, 2))
				return idx;

			light_def.radius_secondary = strutil::asInt(tokens[idx + 1].text);

			idx += 2; // Skip 'secondarySize <SECSIZE>' tokens
		}

		// offset <X> <Y> <Z>
		else if (strutil::equalCI(tokens[idx].text, "offset"))
		{
			if (!checkRequiredTokens(tokens, idx, 4))
				return idx;

			light_def.offset.x = strutil::asInt(tokens[idx + 1].text);
			light_def.offset.y = strutil::asInt(tokens[idx + 2].text);
			light_def.offset.z = strutil::asInt(tokens[idx + 3].text);

			idx += 4; // Skip 'offset <X> <Y> <Z>' tokens
		}

		// subtractive <SUB>
		else if (strutil::equalCI(tokens[idx].text, "subtractive"))
		{
			if (!checkRequiredTokens(tokens, idx, 2))
				return idx;

			light_def.subtractive = strutil::asInt(tokens[idx + 1].text) > 0;

			idx += 2; // Skip 'subtractive <SUB>' tokens
		}

		// attenuate <ATT>
		else if (strutil::equalCI(tokens[idx].text, "attenuate"))
		{
			if (!checkRequiredTokens(tokens, idx, 2))
				return idx;

			light_def.attenuated = strutil::asInt(tokens[idx + 1].text) > 0;

			idx += 2; // Skip 'attenuate <ATT>' tokens
		}

		// intensity <INT>
		else if (strutil::equalCI(tokens[idx].text, "intensity"))
		{
			if (!checkRequiredTokens(tokens, idx, 2))
				return idx;

			light_def.intensity = strutil::asFloat(tokens[idx + 1].text);

			idx += 2; // Skip 'intensity <INT>' tokens
		}

		else
			++idx; // Unrecognized token, skip
	}

	if (colour && radius)
		lightdefs_[light_name] = light_def;

	return idx;
}

// -----------------------------------------------------------------------------
// Parses an object binding definition from [tokens] starting at [idx]
// -----------------------------------------------------------------------------
unsigned GLDefs::parseObjectBinding(const vector<strutil::Token>& tokens, unsigned idx)
{
	// object <CLASSNAME>
	idx++; // Skip 'object' token
	string class_name{ tokens[idx++].text };

	// Check for opening bracket
	if (idx >= tokens.size() || tokens[idx].text != "{")
	{
		logParseError(tokens[idx], fmt::format("Expected '{{' after object definition '{}'", class_name));
		return idx;
	}

	++idx; // Skip '{' token

	// Parse frames
	while (idx < tokens.size() && tokens[idx].text != "}")
	{
		// frame <SPRITENAME> { light <LIGHTNAME> }
		if (strutil::equalCI(tokens[idx].text, "frame"))
		{
			idx++; // Skip 'frame' token

			string frame_name{ tokens[idx++].text };

			if (idx >= tokens.size() || tokens[idx].text != "{")
			{
				logParseError(tokens[idx], fmt::format("Expected '{{' after frame definition '{}'", frame_name));
				return idx;
			}

			idx++; // Skip '{' token

			while (idx < tokens.size() && tokens[idx].text != "}")
			{
				if (strutil::equalCI(tokens[idx].text, "light"))
				{
					idx++; // Skip 'light' token
					string light_name{ tokens[idx++].text };
					object_bindings_[class_name].push_back({ .frame = frame_name, .light_name = light_name });
				}
				else
					idx++; // Unrecognized token, skip
			}

			if (idx < tokens.size() && tokens[idx].text == "}")
				idx++; // Skip '}' token
		}
		else
			idx++; // Unrecognized token, skip
	}

	return idx;
}
