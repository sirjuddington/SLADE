
#include "Main.h"
#include "Mods.h"
#include "Archive/Formats/WadArchive.h"
#include "Archive/Formats/ZipArchive.h"
#include "General/Misc.h"

using namespace Launcher;

Mod::Mod(const string& path) : path_{ path }
{
}

Mod::~Mod()
{
}

bool Mod::scanFile()
{
	MemChunk mc;
	if (!mc.importFile(path_))
		return false;

	crc_ = Misc::crc(mc.getData(), mc.getSize());

	return true;
}


ModLibrary::ModLibrary()
{
}

ModLibrary::~ModLibrary()
{
}

void ModLibrary::scanDir(const string& path)
{
	// Get all files in given directory
	wxArrayString files;
	wxDir::GetAllFiles(path, &files);

	for (auto& file : files)
	{
		// For now just check for wad or zip
		if (WadArchive::isWadArchive(file) ||
			ZipArchive::isZipArchive(file))
		{
			Mod mod(file);
			if (mod.scanFile())
				mods_.push_back(mod);
		}
	}
}
