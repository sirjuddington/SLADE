// -----------------------------------------------------------------------------
// Creates and verifies the deliberately tiny WAD used by the map-script CLI
// regression test. Keeping the source map here avoids committing a binary test
// fixture while still driving the command-line program through real WAD I/O.
// -----------------------------------------------------------------------------

#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace
{
struct Lump
{
	std::string name;
	std::string data;
};

void appendU32(std::vector<std::uint8_t>& output, std::uint32_t value)
{
	for (unsigned shift = 0; shift < 32; shift += 8)
		output.push_back(static_cast<std::uint8_t>(value >> shift));
}

bool readU32(const std::vector<std::uint8_t>& input, size_t offset, std::uint32_t& value)
{
	if (offset > input.size() || input.size() - offset < 4)
		return false;

	value = static_cast<std::uint32_t>(input[offset]) | (static_cast<std::uint32_t>(input[offset + 1]) << 8)
		| (static_cast<std::uint32_t>(input[offset + 2]) << 16) | (static_cast<std::uint32_t>(input[offset + 3]) << 24);
	return true;
}

bool createFixture(const std::filesystem::path& path)
{
	const std::array<Lump, 3> lumps{ {
		{ "MAP01", "" },
		{ "TEXTMAP", "namespace=\"zdoom\";\n" },
		{ "ENDMAP", "" },
	} };

	std::vector<std::uint8_t> wad;
	wad.reserve(12 + lumps[1].data.size() + lumps.size() * 16);
	wad.insert(wad.end(), { 'P', 'W', 'A', 'D' });
	appendU32(wad, static_cast<std::uint32_t>(lumps.size()));
	appendU32(wad, static_cast<std::uint32_t>(12 + lumps[1].data.size()));

	std::vector<std::uint32_t> offsets;
	for (const auto& lump : lumps)
	{
		offsets.push_back(static_cast<std::uint32_t>(wad.size()));
		wad.insert(wad.end(), lump.data.begin(), lump.data.end());
	}

	for (size_t index = 0; index < lumps.size(); ++index)
	{
		appendU32(wad, offsets[index]);
		appendU32(wad, static_cast<std::uint32_t>(lumps[index].data.size()));

		std::array<char, 8> name{};
		std::memcpy(name.data(), lumps[index].name.data(), lumps[index].name.size());
		wad.insert(wad.end(), name.begin(), name.end());
	}

	std::error_code error;
	std::filesystem::create_directories(path.parent_path(), error);
	if (error)
	{
		std::cerr << "Unable to create fixture directory: " << error.message() << '\n';
		return false;
	}

	std::ofstream output(path, std::ios::binary);
	if (!output)
	{
		std::cerr << "Unable to create fixture WAD: " << path << '\n';
		return false;
	}

	output.write(reinterpret_cast<const char*>(wad.data()), static_cast<std::streamsize>(wad.size()));
	return static_cast<bool>(output);
}

bool readLumps(const std::filesystem::path& path, std::vector<Lump>& lumps)
{
	std::ifstream input(path, std::ios::binary);
	if (!input)
	{
		std::cerr << "Unable to read output WAD: " << path << '\n';
		return false;
	}

	const std::vector<std::uint8_t> wad{ std::istreambuf_iterator<char>(input), {} };
	if (wad.size() < 12 || std::memcmp(wad.data(), "PWAD", 4) != 0)
	{
		std::cerr << "Output is not a PWAD\n";
		return false;
	}

	std::uint32_t count = 0;
	std::uint32_t directory_offset = 0;
	if (!readU32(wad, 4, count) || !readU32(wad, 8, directory_offset)
		|| directory_offset > wad.size() || count > (wad.size() - directory_offset) / 16)
	{
		std::cerr << "Output WAD has an invalid directory\n";
		return false;
	}

	for (std::uint32_t index = 0; index < count; ++index)
	{
		const auto entry_offset = static_cast<size_t>(directory_offset) + index * 16;
		std::uint32_t data_offset = 0;
		std::uint32_t data_size = 0;
		if (!readU32(wad, entry_offset, data_offset) || !readU32(wad, entry_offset + 4, data_size)
			|| data_offset > wad.size() || data_size > wad.size() - data_offset)
		{
			std::cerr << "Output WAD has an invalid lump\n";
			return false;
		}

		std::string name(reinterpret_cast<const char*>(wad.data() + entry_offset + 8), 8);
		if (const auto null = name.find('\0'); null != std::string::npos)
			name.erase(null);
		lumps.push_back({
			std::move(name),
			std::string(reinterpret_cast<const char*>(wad.data() + data_offset), data_size),
		});
	}

	return true;
}

size_t blockCount(std::string_view text, std::string_view block_name)
{
	size_t count = 0;
	size_t offset = 0;
	while ((offset = text.find(block_name, offset)) != std::string_view::npos)
	{
		const auto before = offset == 0 ? '\0' : text[offset - 1];
		const auto after  = offset + block_name.size() < text.size() ? text[offset + block_name.size()] : '\0';
		if ((std::isalnum(static_cast<unsigned char>(before)) || before == '_')
			|| (std::isalnum(static_cast<unsigned char>(after)) || after == '_'))
		{
			offset += block_name.size();
			continue;
		}

		auto brace = offset + block_name.size();
		while (brace < text.size() && std::isspace(static_cast<unsigned char>(text[brace])))
			++brace;
		if (brace + 1 < text.size() && text[brace] == '/' && text[brace + 1] == '/')
		{
			brace = text.find('\n', brace + 2);
			if (brace == std::string_view::npos)
				break;
			while (brace < text.size() && std::isspace(static_cast<unsigned char>(text[brace])))
				++brace;
		}
		if (brace < text.size() && text[brace] == '{')
			++count;

		offset += block_name.size();
	}

	return count;
}

bool verifyFixture(const std::filesystem::path& path)
{
	std::vector<Lump> lumps;
	if (!readLumps(path, lumps))
		return false;

	if (lumps.size() != 3 || lumps[0].name != "MAP01" || lumps[1].name != "TEXTMAP" || lumps[2].name != "ENDMAP")
	{
		std::cerr << "Output WAD does not contain the expected UDMF map lumps\n";
		return false;
	}

	const auto& textmap = lumps[1].data;
	if (blockCount(textmap, "vertex") != 2 || blockCount(textmap, "linedef") != 1
		|| textmap.find("v1=0;") == std::string::npos || textmap.find("v2=1;") == std::string::npos)
	{
		std::cerr << "Generated geometry is not the expected line between two vertices\n";
		return false;
	}

	return true;
}
} // namespace

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: map-script-cli-fixture <create|verify> <file.wad>\n";
		return 2;
	}

	const std::filesystem::path path{ argv[2] };
	if (std::string_view{ argv[1] } == "create")
		return createFixture(path) ? 0 : 1;
	if (std::string_view{ argv[1] } == "verify")
		return verifyFixture(path) ? 0 : 1;

	std::cerr << "Unknown action: " << argv[1] << '\n';
	return 2;
}
