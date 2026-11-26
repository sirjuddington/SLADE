// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Filename:    MapScriptCli.cpp
// Description: Standalone runner for Lua map editor scripts
// -----------------------------------------------------------------------------

#include "Main.h"
#include "MapScriptCli.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormat.h"
#include "Archive/ArchiveManager.h"
#include "Archive/MapDesc.h"
#include "Scripting/Scripting.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/StringUtils.h"
#include <filesystem>

using namespace slade;

namespace
{
struct Options
{
	string input;
	string map;
	string script;
	string output;
	bool   validate = false;
};

enum class ParseResult
{
	Ok,
	Help,
	Error,
};

void printUsage()
{
	fmt::print(
		"Usage: slade-map-script --input <archive> --map <map> --script <file.lua> --output <output.wad> "
		"[--validate]\n"
		"\n"
		"Runs a Lua map editor script against a map from an archive and writes the resulting map to a standalone "
		"WAD. The input archive is never modified.\n"
		"\n"
		"The script must return a function that accepts the map, for example:\n"
		"  return function(map)\n"
		"      -- modify map\n"
		"  end\n");
}

ParseResult parseArguments(const vector<string>& args, Options& options, string& error)
{
	for (size_t index = 0; index < args.size(); ++index)
	{
		const auto& arg = args[index];
		if (arg == "--help" || arg == "-h")
			return ParseResult::Help;

		if (arg == "--validate")
		{
			options.validate = true;
			continue;
		}

		auto readValue = [&args, &index, &error](string& value) -> bool
		{
			if (index + 1 >= args.size())
			{
				error = fmt::format("Missing value for {}", args[index]);
				return false;
			}

			value = args[++index];
			return true;
		};

		if (arg == "--input")
		{
			if (!readValue(options.input))
				return ParseResult::Error;
		}
		else if (arg == "--map")
		{
			if (!readValue(options.map))
				return ParseResult::Error;
		}
		else if (arg == "--script")
		{
			if (!readValue(options.script))
				return ParseResult::Error;
		}
		else if (arg == "--output")
		{
			if (!readValue(options.output))
				return ParseResult::Error;
		}
		else
		{
			error = fmt::format("Unknown option: {}", arg);
			return ParseResult::Error;
		}
	}

	if (options.input.empty() || options.map.empty() || options.script.empty() || options.output.empty())
	{
		error = "--input, --map, --script, and --output are required";
		return ParseResult::Error;
	}

	return ParseResult::Ok;
}

std::filesystem::path normalisedPath(const string& path)
{
	std::error_code ec;
	auto            normalised = std::filesystem::weakly_canonical(path, ec);
	if (!ec)
		return normalised;

	normalised = std::filesystem::absolute(path, ec);
	return ec ? std::filesystem::path{ path } : normalised.lexically_normal();
}

bool wouldOverwriteInput(const Options& options)
{
	return normalisedPath(options.input) == normalisedPath(options.output);
}

optional<MapDesc> findMap(const Archive& archive, string_view name)
{
	for (const auto& map : archive.detectMaps())
		if (strutil::equalCI(map.name, name))
			return map;

	return {};
}

string availableMapNames(const Archive& archive)
{
	const auto maps = archive.detectMaps();
	if (maps.empty())
		return "(none)";

	vector<string> names;
	for (const auto& map : maps)
		names.push_back(map.name);

	return strutil::join(names, ", ");
}

bool writeOutputMap(const SLADEMap& map, string_view output, string& error)
{
	if (map.mapName().size() > 8)
	{
		error = fmt::format("Map name '{}' cannot be written to a WAD", map.mapName());
		return false;
	}

	vector<ArchiveEntry*> map_data_raw;
	if (!map.writeMap(map_data_raw))
	{
		error = "Unable to serialise the generated map";
		return false;
	}

	vector<shared_ptr<ArchiveEntry>> map_data;
	map_data.reserve(map_data_raw.size());
	for (auto* entry : map_data_raw)
		map_data.emplace_back(entry);

	Archive output_archive{ ArchiveFormat::Wad };
	if (!output_archive.addNewEntry(map.mapName()))
	{
		error = "Unable to create the map marker in the output WAD";
		return false;
	}

	for (const auto& entry : map_data)
	{
		if (!output_archive.addEntry(entry))
		{
			error = fmt::format("Unable to add '{}' to the output WAD", entry->name());
			return false;
		}
	}

	if (map.currentFormat() == MapFormat::UDMF)
		output_archive.addNewEntry("ENDMAP");

	if (!output_archive.save(output))
	{
		error = global::error.empty() ? "Unable to save the output WAD" : global::error;
		return false;
	}

	return true;
}

bool validateOutputMap(string_view output, string_view map_name, string& error)
{
	Archive output_archive{ ArchiveFormat::Wad };
	if (!output_archive.open(output))
	{
		error = global::error.empty() ? "Unable to reopen the output WAD" : global::error;
		return false;
	}

	const auto map_desc = findMap(output_archive, map_name);
	if (!map_desc)
	{
		error = fmt::format("The output WAD does not contain map '{}'", map_name);
		return false;
	}

	SLADEMap validated_map;
	if (!validated_map.readMap(*map_desc))
	{
		error = fmt::format("The output WAD contains a map '{}' that cannot be read", map_name);
		return false;
	}

	return true;
}

int runMapScript(const Options& options)
{
	if (wouldOverwriteInput(options))
	{
		fmt::print(stderr, "Error: --output must not overwrite --input\n");
		return 1;
	}

	auto input_archive = app::archiveManager().openArchive(options.input, false, true);
	if (!input_archive)
	{
		fmt::print(
			stderr,
			"Error: unable to open input archive '{}': {}\n",
			options.input,
			global::error.empty() ? "unknown error" : global::error);
		return 1;
	}

	const auto map_desc = findMap(*input_archive, options.map);
	if (!map_desc)
	{
		fmt::print(
			stderr,
			"Error: map '{}' was not found in '{}'. Available maps: {}\n",
			options.map,
			options.input,
			availableMapNames(*input_archive));
		return 1;
	}

	SLADEMap map;
	if (!map.readMap(*map_desc))
	{
		fmt::print(stderr, "Error: unable to read map '{}' from '{}'\n", options.map, options.input);
		return 1;
	}

	if (!scripting::runMapScriptFile(options.script, &map))
	{
		const auto& script_error = scripting::error();
		fmt::print(
			stderr,
			"Error: {} script error{}: {}\n",
			script_error.type,
			script_error.line_no >= 0 ? fmt::format(" on line {}", script_error.line_no) : "",
			script_error.message);
		return 1;
	}

	string error;
	if (!writeOutputMap(map, options.output, error))
	{
		fmt::print(stderr, "Error: {}\n", error);
		return 1;
	}

	if (options.validate && !validateOutputMap(options.output, map.mapName(), error))
	{
		fmt::print(stderr, "Error: output validation failed: {}\n", error);
		return 1;
	}

	fmt::print(
		"Generated {} ({} vertices, {} lines, {} sides, {} sectors, {} things){}\n",
		options.output,
		map.nVertices(),
		map.nLines(),
		map.nSides(),
		map.nSectors(),
		map.nThings(),
		options.validate ? "; validation passed" : "");
	return 0;
}
} // namespace

int mapscriptcli::run(const vector<string>& args)
{
	Options options;
	string  error;
	switch (parseArguments(args, options, error))
	{
	case ParseResult::Help:
		printUsage();
		return 0;
	case ParseResult::Error:
		fmt::print(stderr, "Error: {}\n\n", error);
		printUsage();
		return 2;
	case ParseResult::Ok: break;
	}

	if (!app::initHeadless())
	{
		fmt::print(stderr, "Error: unable to initialise SLADE's headless services\n");
		return 1;
	}

	return runMapScript(options);
}
