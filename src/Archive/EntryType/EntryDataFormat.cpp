
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    EntryDataFormat.cpp
// Description: Entry data format detection system
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
#include "EntryDataFormat.h"
#include "Archive/Formats/All.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
vector<unique_ptr<EntryDataFormat>> data_formats;
EntryDataFormat*                    edf_any  = nullptr;
EntryDataFormat*                    edf_text = nullptr;
} // namespace


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Creates an instance of EntryDataFormat T and adds it to the formats list.
// Returns the created instance
// -----------------------------------------------------------------------------
template<class T> T* registerDataFormat()
{
	data_formats.push_back(std::make_unique<T>());
	return static_cast<T*>(data_formats.back().get());
}

// -----------------------------------------------------------------------------
// Creates an instance of EntryDataFormat with [id] and adds it to the formats
// list.
// Returns the created instance
// -----------------------------------------------------------------------------
EntryDataFormat* registerDataFormat(string_view id)
{
	data_formats.push_back(std::make_unique<EntryDataFormat>(id));
	return data_formats.back().get();
}
} // namespace


// -----------------------------------------------------------------------------
//
// EntryDataFormat Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// To be overridden by specific data types, returns true if the data in [mc]
// matches the data format
// -----------------------------------------------------------------------------
int EntryDataFormat::isThisFormat(const MemChunk& mc)
{
	return MATCH_TRUE;
}

// -----------------------------------------------------------------------------
// Copies data format properties to [target]
// -----------------------------------------------------------------------------
void EntryDataFormat::copyToFormat(EntryDataFormat& target) const
{
	target.patterns_ = patterns_;
	target.size_min_ = size_min_;
}


// -----------------------------------------------------------------------------
//
// EntryDataFormat Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the entry data format matching [id], or the 'any' type if no match
// found
// -----------------------------------------------------------------------------
EntryDataFormat* EntryDataFormat::format(string_view id)
{
	for (const auto& format : data_formats)
		if (format->id_ == id)
			return format.get();

	return edf_any;
}

// -----------------------------------------------------------------------------
// Returns the 'any' data format
// -----------------------------------------------------------------------------
EntryDataFormat* EntryDataFormat::anyFormat()
{
	return edf_any;
}

// -----------------------------------------------------------------------------
// Returns the 'text' data format
// -----------------------------------------------------------------------------
EntryDataFormat* EntryDataFormat::textFormat()
{
	return edf_text;
}

// -----------------------------------------------------------------------------
// Parses a user data format definition (unimplemented, currently)
// -----------------------------------------------------------------------------
bool EntryDataFormat::readDataFormatDefinition(const MemChunk& mc)
{
	// Parse the definition
	Parser p;
	p.parseText(mc);

	// Get data_formats tree
	auto pt_formats = p.parseTreeRoot()->childPTN("data_formats");

	// Check it exists
	if (!pt_formats)
		return false;

	// Go through all parsed types
	for (unsigned a = 0; a < pt_formats->nChildren(); a++)
	{
		// Get child as ParseTreeNode
		auto formatnode = pt_formats->childPTN(a);

		// Create+add new data format
		new EntryDataFormat(strutil::lower(formatnode->name()));
	}

	return true;
}

// Special format that always returns false on detection
// Used when a format is requested that doesn't exist
class AnyDataFormat : public EntryDataFormat
{
public:
	AnyDataFormat() : EntryDataFormat("any") {}
	~AnyDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return MATCH_FALSE; }
};

// Required for below #includes
#include "MainEditor/BinaryControlLump.h"

// Format enumeration moved to separate files
#include "DataFormats/ArchiveFormats.h"
#include "DataFormats/AudioFormats.h"
#include "DataFormats/ImageFormats.h"
#include "DataFormats/LumpFormats.h"
#include "DataFormats/MiscFormats.h"
#include "DataFormats/ModelFormats.h"


// -----------------------------------------------------------------------------
// Initialises all built-in data formats (this is currently all formats, as
// externally defined formats are not implemented yet)
// -----------------------------------------------------------------------------
void EntryDataFormat::initBuiltinFormats()
{
	// Create the 'any' format
	edf_any = registerDataFormat<AnyDataFormat>();

	// Register each builtin format class
	// TODO: Ugly ugly ugly, need a better way of doing this, defining each data format in a single place etc
	registerDataFormat<PNGDataFormat>();
	registerDataFormat<BMPDataFormat>();
	registerDataFormat<GIFDataFormat>();
	registerDataFormat<PCXDataFormat>();
	registerDataFormat<TGADataFormat>();
	registerDataFormat<TIFFDataFormat>();
	registerDataFormat<JPEGDataFormat>();
	registerDataFormat<ILBMDataFormat>();
	registerDataFormat<WebPDataFormat>();
	registerDataFormat<DoomGfxDataFormat>();
	registerDataFormat<DoomGfxAlphaDataFormat>();
	registerDataFormat<DoomGfxBetaDataFormat>();
	registerDataFormat<DoomSneaDataFormat>();
	registerDataFormat<DoomArahDataFormat>();
	registerDataFormat<DoomPSXDataFormat>();
	registerDataFormat<DoomJaguarDataFormat>();
	registerDataFormat<DoomJaguarColMajorDataFormat>();
	registerDataFormat<DoomJagTexDataFormat>();
	registerDataFormat<DoomJagSpriteDataFormat>();
	registerDataFormat<ShadowCasterSpriteFormat>();
	registerDataFormat<ShadowCasterWallFormat>();
	registerDataFormat<ShadowCasterGfxFormat>();
	registerDataFormat<AnaMipImageFormat>();
	registerDataFormat<BuildTileFormat>();
	registerDataFormat<Heretic2M8Format>();
	registerDataFormat<Heretic2M32Format>();
	registerDataFormat<HalfLifeTextureFormat>();
	registerDataFormat<IMGZDataFormat>();
	registerDataFormat<QuakeGfxDataFormat>();
	registerDataFormat<QuakeSpriteDataFormat>();
	registerDataFormat<QuakeTexDataFormat>();
	registerDataFormat<QuakeIIWalDataFormat>();
	registerDataFormat<RottGfxDataFormat>();
	registerDataFormat<RottTransGfxDataFormat>();
	registerDataFormat<RottLBMDataFormat>();
	registerDataFormat<RottRawDataFormat>();
	registerDataFormat<RottPicDataFormat>();
	registerDataFormat<WolfPicDataFormat>();
	registerDataFormat<WolfSpriteDataFormat>();
	registerDataFormat<JediBMFormat>();
	registerDataFormat<JediFMEFormat>();
	registerDataFormat<JediWAXFormat>();
	registerDataFormat<JediFNTFormat>();
	registerDataFormat<JediFONTFormat>();
	// registerDataFormat<JediDELTFormat>();
	// registerDataFormat<JediANIMFormat>();
	registerDataFormat<WadDataFormat>();
	registerDataFormat<ZipDataFormat>();
	registerDataFormat<Zip7DataFormat>();
	registerDataFormat<LibDataFormat>();
	registerDataFormat<DatDataFormat>();
	registerDataFormat<ResDataFormat>();
	registerDataFormat<PakDataFormat>();
	registerDataFormat<BSPDataFormat>();
	registerDataFormat<GrpDataFormat>();
	registerDataFormat<RffDataFormat>();
	registerDataFormat<GobDataFormat>();
	registerDataFormat<LfdDataFormat>();
	registerDataFormat<HogDataFormat>();
	registerDataFormat<ADatDataFormat>();
	registerDataFormat<Wad2DataFormat>();
	registerDataFormat<WadJDataFormat>();
	registerDataFormat<WolfDataFormat>();
	registerDataFormat<GZipDataFormat>();
	registerDataFormat<BZip2DataFormat>();
	registerDataFormat<TarDataFormat>();
	registerDataFormat<DiskDataFormat>();
	registerDataFormat<PodArchiveDataFormat>();
	registerDataFormat<ChasmBinArchiveDataFormat>();
	registerDataFormat<SinArchiveDataFormat>();
	registerDataFormat<MUSDataFormat>();
	registerDataFormat<MIDIDataFormat>();
	registerDataFormat<XMIDataFormat>();
	registerDataFormat<HMIDataFormat>();
	registerDataFormat<HMPDataFormat>();
	registerDataFormat<GMIDDataFormat>();
	registerDataFormat<RMIDDataFormat>();
	registerDataFormat<ITModuleDataFormat>();
	registerDataFormat<XMModuleDataFormat>();
	registerDataFormat<S3MModuleDataFormat>();
	registerDataFormat<MODModuleDataFormat>();
	registerDataFormat<OKTModuleDataFormat>();
	registerDataFormat<DRODataFormat>();
	registerDataFormat<RAWDataFormat>();
	registerDataFormat<IMFDataFormat>();
	registerDataFormat<IMFRawDataFormat>();
	registerDataFormat<DoomSoundDataFormat>();
	registerDataFormat<WolfSoundDataFormat>();
	registerDataFormat<DoomMacSoundDataFormat>();
	registerDataFormat<DoomPCSpeakerDataFormat>();
	registerDataFormat<AudioTPCSoundDataFormat>();
	registerDataFormat<AudioTAdlibSoundDataFormat>();
	registerDataFormat<JaguarDoomSoundDataFormat>();
	registerDataFormat<VocDataFormat>();
	registerDataFormat<AYDataFormat>();
	registerDataFormat<GBSDataFormat>();
	registerDataFormat<GYMDataFormat>();
	registerDataFormat<HESDataFormat>();
	registerDataFormat<KSSDataFormat>();
	registerDataFormat<NSFDataFormat>();
	registerDataFormat<NSFEDataFormat>();
	registerDataFormat<SAPDataFormat>();
	registerDataFormat<SPCDataFormat>();
	registerDataFormat<VGMDataFormat>();
	registerDataFormat<VGZDataFormat>();
	registerDataFormat<BloodSFXDataFormat>();
	registerDataFormat<WAVDataFormat>();
	registerDataFormat<SunSoundDataFormat>();
	registerDataFormat<AIFFSoundDataFormat>();
	registerDataFormat<OggDataFormat>();
	registerDataFormat<FLACDataFormat>();
	registerDataFormat<MP2DataFormat>();
	registerDataFormat<MP3DataFormat>();
	registerDataFormat<TextureXDataFormat>();
	registerDataFormat<PNamesDataFormat>();
	registerDataFormat<ACS0DataFormat>();
	registerDataFormat<ACSEDataFormat>();
	registerDataFormat<ACSeDataFormat>();
	registerDataFormat<BoomAnimatedDataFormat>();
	registerDataFormat<BoomSwitchesDataFormat>();
	registerDataFormat<Font0DataFormat>();
	registerDataFormat<Font1DataFormat>();
	registerDataFormat<Font2DataFormat>();
	registerDataFormat<BMFontDataFormat>();
	registerDataFormat<FontWolfDataFormat>();
	registerDataFormat<ZNodesDataFormat>();
	registerDataFormat<ZGLNodesDataFormat>();
	registerDataFormat<ZGLNodes2DataFormat>();
	registerDataFormat<XNodesDataFormat>();
	registerDataFormat<XGLNodesDataFormat>();
	registerDataFormat<XGLNodes2DataFormat>();
	registerDataFormat<XGLNodes3DataFormat>();
	registerDataFormat<DMDModelDataFormat>();
	registerDataFormat<MDLModelDataFormat>();
	registerDataFormat<MD2ModelDataFormat>();
	registerDataFormat<MD3ModelDataFormat>();
	registerDataFormat<VOXVoxelDataFormat>();
	registerDataFormat<KVXVoxelDataFormat>();
	registerDataFormat<RLE0DataFormat>();

	// And here are some dummy formats needed for certain image formats
	// that can't be detected by anything but size (which is done in EntryType detection anyway)
	registerDataFormat("img_raw");
	registerDataFormat("img_rottwall");
	registerDataFormat("img_planar");
	registerDataFormat("img_4bitchunk");
	registerDataFormat("font_mono");

	// Dummy for generic raw data format
	registerDataFormat("rawdata");

	// Another dummy for the generic text format
	edf_text = registerDataFormat("text");
}
