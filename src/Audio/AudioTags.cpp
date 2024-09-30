
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    AudioTags.cpp
// Description: Functions for parsing metadata tags in audio files.
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
#include "AudioTags.h"
#include "Utility/Memory.h"
#include <SFML/System.hpp>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Structs
//
// -----------------------------------------------------------------------------
namespace
{
#pragma pack(1)
struct ID3v1
{
	char    tag[3]; // TAG
	char    title[30];
	char    artist[30];
	char    album[30];
	char    year[4];
	char    comment[30];
	uint8_t genre;
};

struct ID3v1e
{
	char    tag[4]; // TAG+
	char    title[60];
	char    artist[60];
	char    album[60];
	uint8_t speed;
	char    subgenre[30];
	char    start[6];
	char    stop[6];
};

struct OggPageHeader
{
	char     tag[4];     // "OggS"
	uint8_t  version;    // should be 0
	uint8_t  type;       // should be 1, 2, or 4
	uint32_t granule[2]; // abstract time marker
	uint32_t serialnum;  // bitstream serial number
	uint32_t pagenum;    // page sequence number, should always be 1 more than previous
	uint32_t checksum;   // CRC-32 of entire page (including header but with blank checksum)
	uint8_t  segments;   // number of segments
};

struct VorbisHeader
{
	uint8_t packettype; // 1 (identification), 3 (comment), or 5 (setup)
	char    tag[6];     // "vorbis"
};
struct VorbisId
{
	uint32_t version;    // should be 0
	uint8_t  channels;   // should not be 0
	uint32_t samplerate; // should not be 0 either
	int32_t  maxbitrate;
	int32_t  nombitrate;
	int32_t  minbitrate;
	uint32_t blocksize0;
	uint32_t blocksize1;
	uint8_t  framingflag;
};
struct ITHeader
{
	uint32_t id; // "IMPM"
	char     songname[26];
	uint16_t reserved1; // 0x1004
	uint16_t ordnum;
	uint16_t insnum;
	uint16_t smpnum;
	uint16_t patnum;
	uint32_t dontcare;
	uint16_t flags;
	uint16_t special;
	uint16_t dontcare2[3];
	uint16_t msglength;
	uint32_t msgoffset;
	uint32_t dontcare3[33];
};
struct S3MHeader
{
	char     songname[28];
	uint32_t dontcare;
	uint16_t ordnum;
	uint16_t insnum;
	uint16_t patnum;
	uint32_t dontcare2;
	uint16_t version;
	uint32_t scrm; // "SCRM" = 0x4D524353
	uint16_t dontcare3[7];
	uint16_t special;
};
struct S3MSample
{
	uint8_t  type;
	char     dosname[12];
	uint8_t  dontcare[3];
	uint32_t dontcare2[8];
	char     comment[28];
	uint32_t scr; // either "SCRI" or "SCRS"
};
struct XMHeader
{
	char     id[17];       // "Extended Module: " or "Extended module: "
	char     songname[20]; // song name
	uint8_t  reserved;     // 0x1a
	char     tracker[20];  // tracker program name
	uint16_t version;      // either 0x0401 or 0x0301
	uint32_t headersize;   // not counting the first 60 bytes
	uint32_t dontcare;
	uint16_t chnnum;
	uint16_t patnum;
	uint16_t insnum;
	// more after, but we don't care about them here
};
struct WavChunk
{
	char     id[4];
	uint32_t size;
};

struct WavFmtChunk
{
	WavChunk header;
	uint16_t tag;
	uint16_t channels   = 0;
	uint32_t samplerate = 0;
	uint32_t datarate;
	uint16_t blocksize;
	uint16_t bps;
	uint16_t extsize;
	uint16_t vbps;
	uint32_t channelmask;
	uint32_t guid[4];
};
struct BExtChunk
{
	char     Description[256];
	char     Originator[32];
	char     OrigRef[32];
	char     OrigDate[10];
	char     OrigTime[8];
	uint32_t TimeReferenceLow;
	uint32_t TimeReferenceHigh;
	uint16_t Version;
	uint8_t  UMID[64];
	uint16_t LoudnessValue;
	uint16_t LoudnessRange;
	uint16_t MaxTruePeakLevel;
	uint16_t MaxMomentaryLoudness;
	uint16_t MaxShortTermLoudness;
	uint8_t  Reserved[180];
	char     CodingHistory[1];
};
struct WavCue
{
	uint32_t dwName;
	uint32_t dwPosition;
	char     fccChunk[4];
	uint32_t dwChunkStart;
	uint32_t dwBlockStart;
	uint32_t dwSampleOffset;
};
struct AIFFComm
{
	uint32_t name;
	uint32_t size;
	uint16_t channels;
	uint32_t frames;
	uint16_t bitsize;
	uint8_t  xsr[10];
};
#pragma pack()

enum ID3v2Frames
{
	ID3_COM  = 0x434F4D,
	ID3_TAL  = 0x54414C,
	ID3_TCM  = 0x54434D,
	ID3_TCO  = 0x54434F,
	ID3_TCR  = 0x544352,
	ID3_TOA  = 0x544F41,
	ID3_TOL  = 0x544F4C,
	ID3_TOT  = 0x544F54,
	ID3_TP1  = 0x545031,
	ID3_TP2  = 0x545032,
	ID3_TP3  = 0x545033,
	ID3_TP4  = 0x545034,
	ID3_TRK  = 0x54524B,
	ID3_TT1  = 0x545431,
	ID3_TT2  = 0x545432,
	ID3_TT3  = 0x545433,
	ID3_TXT  = 0x545854,
	ID3_TYE  = 0x545945,
	ID3_COMM = 0x434F4D4D,
	ID3_TALB = 0x54414C42,
	ID3_TCOM = 0x54434F4D,
	ID3_TCON = 0x54434F4E,
	ID3_TCOP = 0x54434F50,
	ID3_TDRC = 0x54445243,
	ID3_TEXT = 0x54455854,
	ID3_TIT1 = 0x54495431,
	ID3_TIT2 = 0x54495432,
	ID3_TIT3 = 0x54495433,
	ID3_TOAL = 0x544F414C,
	ID3_TOLY = 0x544F4C59,
	ID3_TOPE = 0x544F5045,
	ID3_TPE1 = 0x54504531,
	ID3_TPE2 = 0x54504532,
	ID3_TPE3 = 0x54504533,
	ID3_TPE4 = 0x54504534,
	ID3_TPOS = 0x54504F53,
	ID3_TRCK = 0x5452434B,
	ID3_TYER = 0x54594552,
};
} // namespace


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
// clang-format off
const char * const id3v1_genres[] =
{
	"Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",		//   0-  5
	"Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",			//   6- 11
	"Other", "Pop", "Rhythm and Blues",	"Rap", "Reggae", "Rock",		//  12- 17
	"Techno", "Industrial",	"Alternative", "Ska", "Death Metal",		//  18- 22
	"Pranks", "Soundtrack",	"Euro-Techno", "Ambient", "Trip-Hop",		//  23- 27
	"Vocal", "Jazz & Funk",	"Fusion", "Trance", "Classical",			//  28- 32
	"Instrumental", "Acid",	"House", "Game", "Sound Clip", "Gospel",	//  33- 38
	"Noise", "Alternative Rock", "Bass", "Soul", "Punk", "Space",		//  39- 44
	"Meditative", "Instrumental Pop", "Instrumental Rock", "Ethnic",	//  45- 48
	"Gothic", "Darkwave", 	"Techno-Industrial", "Electronic",			//  49- 52
	"Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy",		//  53- 57
	"Cult", "Gangsta Rap", "Top 40", "Christian Rap", "Pop & Funk",		//  58- 62
	"Jungle", "Native American", "Cabaret", "New Wave", "Psychedelic",	//  63- 67
	"Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk",		//  68- 73
	"Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll",			//  74- 78
	"Hard Rock", "Folk", "Folk-Rock", "National Folk", "Swing",			//  79- 83
	"Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass",	//  84- 89
	"Avantgarde", "Gothic Rock", "Progressive Rock",					//  90- 92
	"Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",		//  93- 96
	"Chorus", "Easy Listening", "Acoustic", "Humour", "Speech",			//  97-101
	"Chanson", "Opera", "Chamber Music", "Sonata", "Symphony",			// 102-106
	"Booty Bass", "Primus", "Porn Groove", "Satire", "Slow Jam",		// 107-111
	"Club", "Tango", "Samba", "Folklore", "Ballad", "Power Ballad",		// 112-117
	"Rhythmic Soul", "Freestyle", "Duet", "Punk Rock", "Drum Solo",		// 118-122
	"A Cappella", "Euro-House", "Dance Hall", "Goa", "Drum & Bass",		// 123-127
	"Club-House", "Hardcore", "Terror", "Indie", "BritPop",				// 128-132
	"Afro-Punk", "Polsk Punk", "Beat", "Christian Gangsta Rap",			// 133-136
	"Heavy Metal", "Black Metal", "Crossover",							// 137-139
	"Contemporary Christian", "Christian Rock", "Merengue", "Salsa",	// 140-143
	"Thrash Metal", "Anime", "JPop", "Synthpop", "Abstract",			// 144-148
	"Art Rock", "Baroque", "Bhangra", "Big Beat", "Breakbeat",			// 149-153
	"Chillout", "Downtempo", "Dub", "EBM", "Eclectic", "Electro",		// 154-159
	"Electroclash", "Emo", "Experimental", "Garage", "Global", "IDM",	// 160-165
	"Illbient", "Industro-Goth", "Jam Band", "Krautrock", "Leftfield",	// 166-170
	"Lounge", "Math Rock", "New Romantic", "Nu-Breakz", "Post-Punk",	// 171-175
	"Post-Rock", "Psytrance", "Shoegaze", "Space Rock", "Trop Rock",	// 176-180
	"World Music", "Neoclassical", "Audiobook", "Audio Theatre",		// 181-184
	"Neue Deutsche Welle", "Podcast", "Indie Rock", "G-Funk",			// 185-188
	"Dubstep", "Garage Rock", "Psybient"								// 188-191
};
const char * const speaker_pos[] =
{
	"Front Left", "Front Right", "Front Center",
	"Low Frequency", "Back Left", "Back Right",
	"Front Left of Center", "Front Right of Center",
	"Back Center", "Side Left", "Side Right", "Top Center",
	"Top Front Left", "Top Front Center", "Top Front Right",
	"Top Back Left", "Top Back Center", "Top Back Right",
};
}
// clang-format on


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
wxString buildID3v2GenreString(wxString content)
{
	wxString genre = "";
	size_t   i     = 0;
	while (i < content.length())
	{
		if (content[i] == '(' && content[i + 1] != '(')
		{
			if (content[i + 1] == 'R' && content[i + 2] == 'X' && content[i + 3] == ')')
			{
				genre += "Remix";
				i += 4;
			}
			else if (content[i + 1] == 'C' && content[i + 2] == 'R' && content[i + 3] == ')')
			{
				genre += "Cover";
				i += 4;
			}
			else
			{
				int index = 0;
				int j     = i + 1;
				while (content[j] >= '0' && content[j] <= '9' && index < 192)
				{
					index = index * 10 + (content[j] - '0');
					++j;
				}
				if (content[j] != ')')
					index = 192;
				if (index < 192)
					genre += wxString::FromAscii(id3v1_genres[index]);
				i = j + 1;
			}
		}
		else
		{
			genre += content.Right(content.length() - i);
			i = content.length() + 1;
		}
		if (i < content.length() && content[i] == '(')
			genre += " / ";
	}
	return genre;
}

wxString parseID3v1Tag(MemChunk& mc, size_t start)
{
	ID3v1    tag;
	wxString version, title, artist, album, comment, genre, year;
	int      track = 0;

	mc.read(&tag, 128, start);
	title   = wxString::FromAscii(tag.title, 30);
	artist  = wxString::FromAscii(tag.artist, 30);
	album   = wxString::FromAscii(tag.album, 30);
	comment = wxString::FromAscii(tag.comment, 30);
	year    = wxString::FromAscii(tag.year, 4);
	genre   = (tag.genre < 192) ? wxString::FromAscii(id3v1_genres[tag.genre]) : "";
	if (tag.comment[28] == 0 && tag.comment[29] != 0)
	{
		version = "ID3v1.1";
		track   = tag.comment[29];
	}
	else
		version = "ID3v1.0";

	// Check for extended tag
	if (start > 256 && mc[start - 227] == 'T' && mc[start - 226] == 'A' && mc[start - 225] == 'G'
		&& mc[start - 224] == '+')
	{
		ID3v1e etag;
		version += '+';

		mc.read(&etag, 227, start - 227);
		title += wxString::FromAscii(etag.title, 60);
		artist += wxString::FromAscii(etag.artist, 60);
		album += wxString::FromAscii(etag.album, 60);
		genre += wxString::Format(" (%s)", etag.subgenre);
	}
	wxString ret = version + '\n';
	if (title.length())
		ret += wxString::Format("Title: %s\n", title);
	if (album.length())
		ret += wxString::Format("Album: %s\n", album);
	if (track != 0)
		ret += wxString::Format("Track: %d\n", track);
	if (artist.length())
		ret += wxString::Format("Artist: %s\n", artist);
	if (year.length())
		ret += wxString::Format("Year: %s\n", year);
	if (genre.length())
		ret += wxString::Format("Genre: %s\n", genre);
	ret += "\n";
	return ret;
}

wxString parseID3v2Tag(MemChunk& mc, size_t start)
{
	wxString version, title, artist, composer, copyright, album, genre, year, group, subtitle, track, comments;
	bool     artists = false;

	version  = wxString::Format("ID3v2.%d.%d", mc[start + 3], mc[start + 4]);
	bool v22 = mc[start + 3] < 3;

	// ID3v2.2 frame headers have a size of 6 (3 byte identifier, 3 byte size).
	// ID3v2.3 and v2.4 frame headers have a size of 10 (4 byte identifier, 4 byte size, 2 byte flags)
	size_t step = v22 ? 6 : 10;

	// Compute synchsafe size
	size_t size = (mc[start + 6] << 21) + (mc[start + 7] << 14) + (mc[start + 8] << 7) + mc[start + 9] + 10;
	size_t end  = start + size;

	// ID3v2.2 tag frames have three-char identifiers.
	// We only care about a little subset of them, though.
	size_t s = start + 10;

	// Iterate through frames. The minimal size of a frame is 1 byte of data.
	while (s + step + 1 < end)
	{
		size_t fsize = v22 ? mc.readB24(s + 3) : mc.readB32(s + 4);

		// One byte for encoding
		size_t tsize = fsize - 1;

		// Process only text frames that aren't empty
		// Also skip flags, not gonna bother with encryption or compression
		if ((mc[s] == 'T' || (mc[s] == 'C' && mc[s + 1] == 'O' && mc[s + 2] == 'M')) && tsize > 0
			&& (v22 || (mc[s + 8] == 0 && mc[s + 9] == 0)))
		{
			wxString content;

			// First step: retrieve the text (UTF-16 massively sucks)
			auto buffer = new char[fsize];
			mc.read(buffer, tsize, s + step + 1);
			size_t frame = v22 ? mc.readB24(s) : mc.readB32(s);

			bool   bomle = true;
			size_t bom   = 0;
			switch (mc[s + step])
			{
			case 0: // Plain old ASCII
				content = wxString::From8BitData(buffer, tsize);
				break;
			case 1: // UTF-16 with byte order mark
			{
				size_t i = 1;
				while (i < tsize - 3)
				{
					bom = i + 1;
					// Looks like stuffing garbage before the actual content is popular
					if (mc[s + step + i] == 0xFF && mc[s + step + i + 1] == 0xFE && mc[s + step + i + 2] != 0)
					{
						bomle = true;
						break;
					}
					if (mc[s + step + i] == 0xFE && mc[s + step + i + 1] == 0xFF && mc[s + step + i + 3] != 0)
					{
						bomle = false;
						break;
					}
					++i;
				}
			}
				// Fall through
			case 2: // UTF-16 without byte order mark
			{
				// You're right, wxWidgets. Code like this is so much
				// better than having something like wxString::FromUTF16()
				size_t u16c   = (tsize - bom) / 2;
				auto   wchars = new wchar_t[u16c];
				for (size_t i = 0; i < u16c; ++i)
				{
					if (bomle)
						wchars[i] = (wchar_t)memory::readL16((const uint8_t*)buffer, bom + 2 * i);
					else
						wchars[i] = (wchar_t)memory::readB16((const uint8_t*)buffer, bom + 2 * i);
				}
				content = wxString(wchars, u16c);
				delete[] wchars;
			}
			break;
			case 3: // UTF-8
				content = wxString::FromUTF8(buffer, tsize);
				break;
			}
			delete[] buffer;

			// Second step: treat frame accordingly to type
			switch (frame)
			{
			case ID3_COM:  // Comments
			case ID3_COMM: // Comments
				if (comments.length())
					comments += "\n\n";
				comments += content;
				break;
			case ID3_TAL:  // Album/Movie/Show title
			case ID3_TOT:  // Original album/movie/show title
			case ID3_TALB: // Album/Movie/Show title
			case ID3_TOAL: // Original album/movie/show title
				if (album.length())
					album += " / ";
				album += content;
				break;
			case ID3_TCM:  // Composer
			case ID3_TCOM: // Composer
				composer = content;
				break;
			case ID3_TCO:  // Content type
			case ID3_TCON: // Content type
				genre = buildID3v2GenreString(content);
				break;
			case ID3_TCR:  // Copyright message
			case ID3_TCOP: // Copyright message
				copyright = content;
				break;
			case ID3_TOA:  // Original artist(s)/performer(s)
			case ID3_TOL:  // Original Lyricist(s)/text writer(s)
			case ID3_TP1:  // Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group
			case ID3_TP2:  // Band/Orchestra/Accompaniment
			case ID3_TP3:  // Conductor/Performer refinement
			case ID3_TP4:  // Interpreted, remixed, or otherwise modified by
			case ID3_TXT:  // Lyricist/text writer
			case ID3_TEXT: // Lyricist/Text writer
			case ID3_TOLY: // Original lyricist(s)/text writer(s)
			case ID3_TOPE: // Original artist(s)/performer(s)
			case ID3_TPE1: // Lead performer(s)/Soloist(s)
			case ID3_TPE2: // Band/orchestra/accompaniment
			case ID3_TPE3: // Conductor/performer refinement
			case ID3_TPE4: // Interpreted, remixed, or otherwise modified by
				if (artist.length())
				{
					artist += " / ";
					artists = true;
				}
				artist += content;
				break;
			case ID3_TRK:  // Track number/Position in set
			case ID3_TRCK: // Track number/Position in set
				track = content;
				break;
			case ID3_TT1:  // Content group description
			case ID3_TIT1: // Content group description
				group = content;
				break;
			case ID3_TT2:  // Title/Songname/Content description
			case ID3_TIT2: // Title/songname/content description
				title = content;
				break;
			case ID3_TT3:  // Subtitle/Description refinement
			case ID3_TIT3: // Subtitle/Description refinement
				group = content;
				break;
			case ID3_TYE:  // Year
			case ID3_TYER: // Year
				year = content;
				break;
			case ID3_TDRC: // Recording time (precision varies between yyyy and yyyy-MM-ddTHH:mm:ss)
				year = content.Left(4);
				break;
			}
		}
		// Parsing stops when padding starts
		else if (mc[s] == 0 && fsize == 0)
			break;
		s += (fsize + step);
	}

	wxString ret = version + '\n';
	if (group.length())
		ret += wxString::Format("Group: %s\n", group);
	if (title.length())
		ret += wxString::Format("Title: %s\n", title);
	if (album.length())
		ret += wxString::Format("Album: %s\n", album);
	if (track.length())
		ret += wxString::Format("Track: %s\n", track);
	if (artist.length())
		ret += wxString::Format("Artist%s: %s\n", artists ? "s" : "", artist);
	if (copyright.length())
		ret += wxString::Format(wxString::FromUTF8("Copyright \xC2\xA9 %s\n"), copyright);
	if (year.length())
		ret += wxString::Format("Year: %s\n", year);
	if (genre.length())
		ret += wxString::Format("Genre: %s\n", genre);
	if (comments.length())
		ret += wxString::Format("Comments:\n%s\n", comments);
	ret += "\n";
	return ret;
}

wxString parseVorbisComment(MemChunk& mc, size_t start)
{
	sf::Clock timer;
	wxString  ret;
	auto      data = (const char*)mc.data();
	size_t    end  = mc.size();

	if (start + 10 > end)
		return ret + "\nInvalid Vorbis comment segment (A)\n";
	size_t strlen = mc.readL32(start);
	if (start + 10 + strlen > end)
		return ret + "\nInvalid Vorbis comment segment (B)\n";

	wxString vendor = wxString::FromUTF8(data + start + 4, strlen);

	size_t numcomments = mc.readL32(start + 4 + strlen);
	size_t s           = start + 8 + strlen;

	for (size_t i = 0; i < numcomments && s + 6 < end; ++i)
	{
		strlen = mc.readL32(s);
		if (s + strlen + 4 > end)
			return ret + "\nInvalid Vorbis comment segment (C)\n";
		ret += wxString::FromUTF8(data + s + 4, strlen);
		ret += "\n";
		s += 4 + strlen;
	}

	ret += wxString::Format("\nVendor string: %s\n", vendor);

	return ret;
}

wxString parseIFFChunks(MemChunk& mc, size_t s, size_t samplerate, const WavChunk* cue, bool bigendian = false)
{
	const WavChunk* temp  = nullptr;
	auto            data  = (const char*)mc.data();
	auto            udata = (const uint8_t*)data;
	wxString        ret   = "";

	while (s + 8 < mc.size())
	{
		temp            = (const WavChunk*)(data + s);
		size_t tempsize = bigendian ? wxUINT32_SWAP_ON_LE(temp->size) : wxUINT32_SWAP_ON_BE(temp->size);
		size_t offset   = s + 8;
		size_t end      = offset + tempsize;
		s               = end;
		if (s % 2)
			s++;

		if (end > mc.size())
			break;

		// Broadcast extensions (bext chunk)
		if (temp->id[0] == 'b' && temp->id[1] == 'e' && temp->id[2] == 'x' && temp->id[3] == 't' && tempsize >= 602)
		{
			wxString bextstr = "Broadcast extensions:\n";
			auto     bext    = (const BExtChunk*)(data + offset);
			if (bext->Description[0])
				bextstr += wxString::Format("Description: %s\n", wxString::From8BitData(bext->Description, 256));
			if (bext->Originator[0])
				bextstr += wxString::Format("Originator: %s\n", wxString::From8BitData(bext->Originator, 32));
			if (bext->OrigRef[0])
				bextstr += wxString::Format("Reference: %s\n", wxString::From8BitData(bext->OrigRef, 32));
			if (bext->OrigDate[0])
				bextstr += wxString::Format("Date: %s\n", wxString::From8BitData(bext->OrigDate, 10));
			if (bext->OrigTime[0])
				bextstr += wxString::Format("Time: %s\n", wxString::From8BitData(bext->OrigTime, 8));
			if (bext->TimeReferenceLow | bext->TimeReferenceHigh)
			{
				uint64_t timeref = wxUINT32_SWAP_ON_BE(bext->TimeReferenceLow)
								   + (((uint64_t)(wxUINT32_SWAP_ON_BE(bext->TimeReferenceHigh)) << 32));
				double timesec = (double)timeref / (double)samplerate;
				size_t milsec  = 1000 * timesec;
				size_t sec     = (milsec / 1000) % 60;
				size_t min     = (milsec / 60000) % 60;
				size_t hor     = (milsec / 3600000) % 24;
				milsec %= 1000;
				bextstr += wxString::Format("Time Reference: %ld:%02ld:%02ld.%03ld\n", hor, min, sec, milsec);
			}
			bextstr += wxString::Format("BWFVersion: %d\n", wxUINT16_SWAP_ON_BE(bext->Version));
			if (bext->LoudnessValue)
				bextstr += wxString::Format("Integrated Loudness: %d\n", wxUINT16_SWAP_ON_BE(bext->LoudnessValue));
			if (bext->LoudnessRange)
				bextstr += wxString::Format("Loudness Range: %d\n", wxUINT16_SWAP_ON_BE(bext->LoudnessRange));
			if (bext->MaxTruePeakLevel)
				bextstr += wxString::Format(
					"Maximum True Peak Level: %d\n", wxUINT16_SWAP_ON_BE(bext->MaxTruePeakLevel));
			if (bext->MaxMomentaryLoudness)
				bextstr += wxString::Format(
					"Highest Momentary Loudness Level: %d\n", wxUINT16_SWAP_ON_BE(bext->MaxMomentaryLoudness));
			if (bext->MaxShortTermLoudness)
				bextstr += wxString::Format(
					"Highest Short-Term Loudness Level: %d\n", wxUINT16_SWAP_ON_BE(bext->MaxShortTermLoudness));
			if (tempsize > 602 && bext->CodingHistory[0])
				bextstr += wxString::Format(
					"History: %s\n", wxString::From8BitData(bext->CodingHistory, tempsize - 602));

			ret += wxString::Format("%s\n", bextstr);
		}
		// ID3 tag (yes, they may happen in a WAV, Audacity embeds them in an 'id3 ' chunk so they're still valid RIFF
		// files)
		if (temp->id[0] == 'i' && temp->id[1] == 'd' && temp->id[2] == '3' && temp->id[3] == ' ' && tempsize > 14)
		{
			if (mc[offset] == 'T' && mc[offset + 1] == 'A' && mc[offset + 2] == 'G')
				ret += parseID3v1Tag(mc, offset);
			else if (mc[offset] == 'I' && mc[offset + 1] == 'D' && mc[offset + 2] == '3')
				ret += parseID3v2Tag(mc, offset);
		}
		// RIFF Data Listing
		if (temp->id[0] == 'L' && temp->id[1] == 'I' && temp->id[2] == 'S' && temp->id[3] == 'T' && tempsize > 4)
		{
			// LIST INFO chunk
			if (mc[offset] == 'I' && mc[offset + 1] == 'N' && mc[offset + 2] == 'F' && mc[offset + 3] == 'O')
			{
				wxString liststr = "Information:\n";
				offset += 4;
				while (offset + 8 < end)
				{
					auto     chunk   = (const WavChunk*)(data + offset);
					size_t   chsz    = wxUINT32_SWAP_ON_BE(chunk->size);
					wxString tagname = wxString::Format("%s: ", wxString::From8BitData(chunk->id, 4));
					offset += 8;
					if (offset + chsz > end)
						break;
					else if (chunk->id[0] == 'I')
					{
						if (chunk->id[1] == 'A' && chunk->id[2] == 'R')
						{
							if (chunk->id[3] == 'L')
								tagname = "Archival Location: ";
							else if (chunk->id[3] == 'T')
								tagname = "Artist: ";
						}
						else if (chunk->id[1] == 'C')
						{
							if (chunk->id[2] == 'M' && chunk->id[3] == 'S')
								tagname = "Commissioned: ";
							else if (chunk->id[2] == 'M' && chunk->id[3] == 'T')
								tagname = "Comment: ";
							else if (chunk->id[2] == 'O' && chunk->id[3] == 'P')
								tagname = "Copyright: ";
							else if (chunk->id[2] == 'R' && chunk->id[3] == 'D')
								tagname = "Date Created: ";
							else if (chunk->id[2] == 'R' && chunk->id[3] == 'P')
								tagname = "Cropped: ";
						}
						else if (chunk->id[1] == 'E' && chunk->id[2] == 'N' && chunk->id[3] == 'G')
							tagname = "Engineer: ";
						else if (chunk->id[1] == 'G' && chunk->id[2] == 'N' && chunk->id[3] == 'R')
							tagname = "Genre: ";
						else if (chunk->id[1] == 'K' && chunk->id[2] == 'E' && chunk->id[3] == 'Y')
							tagname = "Keywords: ";
						else if (chunk->id[1] == 'M' && chunk->id[2] == 'E' && chunk->id[3] == 'D')
							tagname = "Medium: ";
						else if (chunk->id[1] == 'N' && chunk->id[2] == 'A' && chunk->id[3] == 'M')
							tagname = "Title: ";
						else if (chunk->id[1] == 'P' && chunk->id[2] == 'R' && chunk->id[3] == 'D')
							tagname = "Product: ";
						else if (chunk->id[1] == 'S' && chunk->id[2] == 'B' && chunk->id[3] == 'J')
							tagname = "Subject: ";
						else if (chunk->id[1] == 'S' && chunk->id[2] == 'F' && chunk->id[3] == 'T')
							tagname = "Software: ";
						else if (chunk->id[1] == 'S' && chunk->id[2] == 'R' && chunk->id[3] == 'C')
							tagname = "Source: ";
						else if (chunk->id[1] == 'S' && chunk->id[2] == 'R' && chunk->id[3] == 'F')
							tagname = "Source Form: ";
						else if (chunk->id[1] == 'T' && chunk->id[2] == 'C' && chunk->id[3] == 'H')
							tagname = "Technician: ";
					}
					liststr += wxString::Format("%s%s\n", tagname, wxString::From8BitData(data + offset, chsz));
					offset += chsz;
					if (offset % 2)
						offset++;
				}
				ret += wxString::Format("%s\n", liststr);
			}
			// LIST adtl chunk
			else if (mc[offset] == 'a' && mc[offset + 1] == 'd' && mc[offset + 2] == 't' && mc[offset + 3] == 'l')
			{
				// We need to have a cue chunk, wav specs say there can be only one at most
				if (cue && wxUINT32_SWAP_ON_BE(cue->size) >= 4)
				{
					size_t cueofs        = 8 + (const char*)cue - data;
					size_t cuesize       = wxUINT32_SWAP_ON_BE(cue->size);
					size_t numcuepoints  = memory::readL32(udata, cueofs);
					auto   alreadylisted = new bool[numcuepoints];
					memset(alreadylisted, false, numcuepoints * sizeof(bool));
					if (cuesize >= 4 + numcuepoints * sizeof(WavCue))
					{
						wxString liststr   = wxString::Format("Associated Data List:\n%ld cue points\n", numcuepoints);
						auto     cuepoints = (const WavCue*)(data + cueofs + 4);
						size_t   ioffset   = offset + 4;
						while (ioffset < end)
						{
							auto   note  = (const WavChunk*)(data + ioffset);
							size_t isize = wxUINT32_SWAP_ON_BE(note->size);
							ioffset += 8;
							size_t cuepoint = memory::readL32(udata, ioffset);
							int    cpindex  = -1;
							for (size_t i = 0; i < numcuepoints; ++i)
							{
								if (wxUINT32_SWAP_ON_BE(cuepoints[i].dwName) == cuepoint)
								{
									cpindex = i;
									break;
								}
							}
							if (cpindex >= 0 && !alreadylisted[cpindex])
							{
								liststr += wxString::Format(
									"Cue point %ld: sample %d from %s, offset %d, block offset %d, chunk %d\n",
									cuepoint,
									wxUINT32_SWAP_ON_BE(cuepoints[cpindex].dwPosition),
									wxString::From8BitData(cuepoints[cpindex].fccChunk, 4),
									wxUINT32_SWAP_ON_BE(cuepoints[cpindex].dwSampleOffset),
									wxUINT32_SWAP_ON_BE(cuepoints[cpindex].dwBlockStart),
									wxUINT32_SWAP_ON_BE(cuepoints[cpindex].dwChunkStart));
								alreadylisted[cpindex] = true;
							}
							if (note->id[0] == 'l' && note->id[1] == 'a' && note->id[2] == 'b' && note->id[3] == 'l')
							{
								wxString content = wxString::From8BitData(data + ioffset + 4, isize - 4);
								content.Trim();
								liststr += wxString::Format("Cue point %ld label: %s\n", cuepoint, content);
							}
							else if (
								note->id[0] == 'l' && note->id[1] == 't' && note->id[2] == 'x' && note->id[3] == 't')
							{
								liststr += wxString::Format(
									"Cue point %ld: sample length %d, purpose %s\n",
									cuepoint,
									memory::readL32(udata, (ioffset + 4)),
									wxString::From8BitData(data + ioffset + 8, 4));
							}
							else if (
								note->id[0] == 'n' && note->id[1] == 'o' && note->id[2] == 't' && note->id[3] == 'e')
							{
								wxString content = wxString::From8BitData(data + ioffset + 4, isize - 4);
								content.Trim();
								liststr += wxString::Format("Cue point %ld note: %s\n", cuepoint, content);
							}

							ioffset += isize;
						}
						ret += wxString::Format("%s\n", liststr);
					}
				}
			}
		}
		// Other ASCII metadata, if they aren't too big
		else if (tempsize > 4 && tempsize < 8192)
		{
			bool pure_ascii = true;
			bool zerochar   = false; // true if previous character was '\0'
			for (size_t i = 0; pure_ascii && i < tempsize; ++i)
			{
				// Allow them to have several substrings separated by single null bytes
				// This is found notably in afsp chunks.
				if (udata[offset + i] == 0)
				{
					// Two nulls in a row? Then break.
					if (zerochar)
					{
						pure_ascii = false;
						break;
					}
					zerochar = true;
				}
				// Only accept CR, LF, tabs, and printable characters
				else if (
					(udata[offset + i] < 0x20 && udata[offset + i] != 9 && udata[offset + i] != 10
					 && udata[offset + i] != 13)
					|| udata[offset + i] > 0x7E)
				{
					pure_ascii = false;
					break;
				}
				else
					zerochar = false; // Reset zero status of previous character
			}
			if (pure_ascii)
			{
				auto asciidata = new char[tempsize];
				memcpy(asciidata, data + offset, tempsize);
				for (size_t i = 0; i < tempsize - 1; ++i)
					if (asciidata[i] == 0)
						asciidata[i] = '\n';

				ret += wxString::Format(
					"%s chunk:\n%s\n\n",
					wxString::From8BitData(temp->id, 4),
					wxString::From8BitData(asciidata, tempsize));
				delete[] asciidata;
			}
		}
	}
	return ret;
}
} // namespace


// -----------------------------------------------------------------------------
//
// Audio Namespace Functions
//
// -----------------------------------------------------------------------------

wxString audio::getID3Tag(MemChunk& mc)
{
	// We actually identify RIFF-WAVE files as MP3 if they are encoded with
	// the MP3 codec, but that means the metadata format is different, so
	// call the RIFF-WAVE metadata function instead. We might end up finding
	// an ID3 tag anyway, provided it's nicely embedded in an "id3 " chunk.
	if (mc.size() > 64 && mc[0] == 'R' && mc[1] == 'I' && mc[2] == 'F' && mc[3] == 'F' && mc[8] == 'W' && mc[9] == 'A'
		&& mc[10] == 'V' && mc[11] == 'E')
		return getWavInfo(mc);

	wxString ret;
	// Check for empty wasted space at the beginning, since it's apparently
	// quite popular in MP3s to start with a useless blank frame.
	size_t s = 0;
	if (mc.size() > 0 && mc[0] == 0)
	{
		// Completely arbitrary limit to how long to seek for data.
		size_t limit = std::min<size_t>(1200, mc.size() / 16);
		while ((s < limit) && (mc[s] == 0))
			++s;
	}

	if (mc.size() > s + 14)
	{
		// Check for ID3 header (ID3v2). Version and revision numbers cannot be FF.
		// Only the four upper flags are valid.
		while (mc.size() > s + 14 && mc[s + 0] == 'I' && mc[s + 1] == 'D' && mc[s + 2] == '3' && mc[s + 3] != 0xFF
			   && mc[s + 4] != 0xFF && ((mc[s + 5] & 0x0F) == 0) && mc[s + 6] < 0x80 && mc[s + 7] < 0x80
			   && mc[s + 8] < 0x80 && mc[s + 9] < 0x80)
		{
			ret += parseID3v2Tag(mc, s);

			// Compute size. It is stored as a "synchsafe integer", that is to say,
			// a big-endian value where the highest bit of each byte is not used.
			size_t size = (mc[s + 6] << 21) + (mc[s + 7] << 14) + (mc[s + 8] << 7) + mc[s + 9] + 10;
			// If there is a footer, then add 10 more to the size
			if (mc[s + 5] & 0x10)
				size += 10;
			// Needs to be at least that big
			if (mc.size() >= size + 4)
				s += size;
		}
	}
	// It's also possible to get an ID3v1 (or v1.1) tag.
	// Though normally they're at the end of the file.
	if (mc.size() > s + 132)
	{
		// Check for ID3 header (ID3v1).
		if (mc[s + 0] == 'T' && mc[s + 1] == 'A' && mc[s + 2] == 'G')
		{
			ret += parseID3v1Tag(mc, s);
		}
	}
	// Look for ID3v1 tag at end of file.
	if (mc.size() > 132)
	{
		s = mc.size() - 128;
		// Check for ID3 header (ID3v1).
		if (mc[s + 0] == 'T' && mc[s + 1] == 'A' && mc[s + 2] == 'G')
		{
			ret += parseID3v1Tag(mc, s);
		}
	}
	return ret;
}

wxString audio::getOggComments(MemChunk& mc)
{
	OggPageHeader ogg;
	VorbisHeader  vorb;
	size_t        pagestart = 58;
	size_t        end       = mc.size();
	wxString      ret       = "";

	while (pagestart + 28 < end)
	{
		mc.read(&ogg, 27, pagestart);
		size_t pagesize = 27;

		for (int i = 0; i < ogg.segments && pagestart + 27 + i < end; ++i)
		{
			size_t segsize = mc[pagestart + 27 + i];

			if (segsize > 16 && ogg.pagenum < 3)
			{
				size_t datastart = pagestart + pagesize + ogg.segments;

				// Stop right now in case of problem
				if (datastart + 7 >= end)
					return ret;

				// Look if we have a vorbis comment header in that segment
				mc.read(&vorb, 7, datastart);
				if (vorb.packettype == 3 && vorb.tag[0] == 'v' && vorb.tag[1] == 'o' && vorb.tag[2] == 'r'
					&& vorb.tag[3] == 'b' && vorb.tag[4] == 'i' && vorb.tag[5] == 's')
				{
					ret += parseVorbisComment(mc, datastart + 7);

					// There's only one vorbis comment header per vorbis stream, so get out
					pagestart = end;
					break;
				}
			}
			pagesize += segsize;
		}
		pagesize += ogg.segments;
		pagestart += pagesize;
	}

	return ret;
}

wxString audio::getFlacComments(MemChunk& mc)
{
	wxString ret = "";
	// FLAC files begin with identifier "fLaC"; skip them
	size_t s = 4;
	// FLAC metadata blocks have a 4-byte header
	while (s + 4 < mc.size())
	{
		// Last three bytes are big-endian value for size of metadata
		size_t blocksize = mc.readB24(s + 1);

		// First byte contains block type and "last block" flag (128)
		// Type 4 is the VORBIS_COMMENT type
		if ((mc[s] & 0x7F) == 4)
			ret += parseVorbisComment(mc, s + 4);

		// If this was the last block, no need to keep processing
		if (mc[s] & 0x80)
			break;
		// Otherwise, keep on trucking to next block
		s += 4 + blocksize;
	}
	return ret;
}

wxString audio::getITComments(MemChunk& mc)
{
	auto   data = (const char*)mc.data();
	auto   head = (const ITHeader*)data;
	size_t s    = sizeof(ITHeader);

	// Get song name
	wxString ret = wxString::Format("%s\n", wxString::From8BitData(head->songname, 26));

	// Get song comment, if any
	if ((wxUINT16_SWAP_ON_BE(head->special) & 1) && (wxUINT16_SWAP_ON_BE(head->msglength) > 0))
	{
		// To keep only valid strings, we trim whitespace and then print the string into itself.
		// The second step gets rid of strings full of invalid characters where length() does not
		// report the actual printable length correctly.
		wxString comment = wxString::From8BitData(
			data + wxUINT16_SWAP_ON_BE(head->msgoffset), wxUINT16_SWAP_ON_BE(head->msglength));
		comment.Trim();
		comment = wxString::Format("%s", comment);
		if (comment.length())
			ret += wxString::Format("%s\n", comment);
	}

	// Get instrument comments
	size_t offset = s + wxUINT16_SWAP_ON_BE(head->ordnum);
	if (head->insnum)
		ret += wxString::Format("\n%d instruments:\n", wxUINT16_SWAP_ON_BE(head->insnum));
	for (size_t i = 0; i < wxUINT16_SWAP_ON_BE(head->insnum); ++i)
	{
		size_t ofs = memory::readL32((const uint8_t*)data, (offset + (i << 2)));
		if (ofs > offset && ofs + 60 < mc.size() && data[ofs] == 'I' && data[ofs + 1] == 'M' && data[ofs + 2] == 'P'
			&& data[ofs + 3] == 'I')
		{
			wxString instrument = wxString::From8BitData(data + ofs + 4, 12);
			instrument.Trim();
			instrument       = wxString::Format("%s", instrument);
			wxString comment = wxString::From8BitData(data + ofs + 32, 26);
			comment.Trim();
			comment = wxString::Format("%s", comment);
			if (instrument.length() && comment.length())
				ret += wxString::Format("%i: %s - %s\n", i, instrument, comment);
			else if (instrument.length())
				ret += wxString::Format("%i: %s\n", i, instrument);
			else if (comment.length())
				ret += wxString::Format("%i - %s\n", i, comment);
		}
	}

	// Get sample comments
	offset += wxUINT16_SWAP_ON_BE(head->insnum) << 2;
	if (head->smpnum)
		ret += wxString::Format("\n%d samples:\n", wxUINT16_SWAP_ON_BE(head->smpnum));
	for (size_t i = 0; i < wxUINT16_SWAP_ON_BE(head->smpnum); ++i)
	{
		size_t pos = offset + (i << 2);
		size_t ofs = mc.readL32(pos);
		if (ofs > offset && ofs + 60 < mc.size() && data[ofs] == 'I' && data[ofs + 1] == 'M' && data[ofs + 2] == 'P'
			&& data[ofs + 3] == 'S')
		{
			wxString sample = wxString::From8BitData(data + ofs + 4, 12);
			sample.Trim();
			sample           = wxString::Format("%s", sample);
			wxString comment = wxString::From8BitData(data + ofs + 20, 26);
			comment.Trim();
			comment = wxString::Format("%s", comment);
			if (sample.length() && comment.length())
				ret += wxString::Format("%i: %s - %s\n", i, sample, comment);
			else if (sample.length())
				ret += wxString::Format("%i: %s\n", i, sample);
			else if (comment.length())
				ret += wxString::Format("%i - %s\n", i, comment);
		}
	}

	return ret;
}

wxString audio::getModComments(MemChunk& mc)
{
	auto   data = (const char*)mc.data();
	size_t s    = 20;

	// Get song name
	wxString ret = wxString::Format("%s\n", wxString::From8BitData(data, 20));

	// Get instrument/sample comments
	// We only recognize mods that have their magic identifier at offset 1080 (31 samples),
	// so no need to bother with mods that only have 15 samples (magic at offset 600).
	ret += "\n31 samples:\n";
	for (size_t i = 0; i < 31; ++i)
	{
		wxString comment = wxString::From8BitData(data + s, 22);
		comment.Trim();
		comment = wxString::Format("%s", comment);
		if (comment.length())
			ret += wxString::Format("%i - %s\n", i, comment);

		// Move to next offset
		s += 30;
	}
	return ret;
}

wxString audio::getS3MComments(MemChunk& mc)
{
	auto   data = (const char*)mc.data();
	auto   head = (const S3MHeader*)data;
	size_t s    = 96;

	// Get song name
	wxString ret = wxString::Format("%s\n", wxString::From8BitData(head->songname, 28));

	// Get instrument/sample comments
	if (head->insnum)
		ret += wxString::Format("\n%d instruments and samples:\n", wxUINT16_SWAP_ON_BE(head->insnum));
	s += wxUINT16_SWAP_ON_BE(head->ordnum);
	for (size_t i = 0; i < wxUINT16_SWAP_ON_BE(head->insnum); ++i)
	{
		size_t t = (mc.readL16((s + 2 * i))) << 4;
		if (t + 80 > mc.size())
			return ret;
		auto     sample  = (const S3MSample*)(data + t);
		wxString dosname = wxString::From8BitData(sample->dosname, 12);
		dosname.Trim();
		dosname          = wxString::Format("%s", dosname);
		wxString comment = wxString::From8BitData(sample->comment, 28);
		comment.Trim();
		comment = wxString::Format("%s", comment);
		if (dosname.length() && comment.length())
			ret += wxString::Format("%i: %s - %s\n", i, dosname, comment);
		else if (dosname.length())
			ret += wxString::Format("%i: %s\n", i, dosname);
		else if (comment.length())
			ret += wxString::Format("%i - %s\n", i, comment);
	}
	return ret;
}

wxString audio::getXMComments(MemChunk& mc)
{
	auto   data = (const char*)mc.data();
	auto   head = (const XMHeader*)data;
	size_t s    = 60 + wxUINT32_SWAP_ON_BE(head->headersize);

	// Get song name
	wxString ret = wxString::Format("%s\n", wxString::From8BitData(head->songname, 20));

	// Get tracker name
	ret += wxString::Format("Tracked with %s\n", wxString::From8BitData(head->tracker, 20));

	// Skip over patterns
	if (head->patnum)
		ret += wxString::Format("\n%d patterns\n", wxUINT16_SWAP_ON_BE(head->patnum));
	for (size_t i = 0; i < wxUINT16_SWAP_ON_BE(head->patnum); ++i)
	{
		if (s + 9 < mc.size())
		{
			size_t patsize = mc.readL32(s) + mc.readL16(s + 7);
			s += patsize;
		}
		else
			return ret;
	}

	// Get instrument comments
	if (head->insnum)
		ret += wxString::Format("\n%d instruments:\n", wxUINT16_SWAP_ON_BE(head->insnum));
	for (size_t i = 0; i < wxUINT16_SWAP_ON_BE(head->insnum); ++i)
	{
		if (s + 29 < mc.size())
		{
			size_t instsize = mc.readL32(s);
			if (instsize < 33)
				return ret;
			// To keep only valid strings, we trim whitespace and then print the string into itself.
			// The second step gets rid of strings full of invalid characters where length() does not
			// report the actual printable length correctly.
			wxString comment = wxString::From8BitData(data + s + 4, 22);
			comment.Trim();
			comment = wxString::Format("%s", comment);
			if (comment.length())
				ret += wxString::Format("%i: %s\n", i, comment);
			size_t samples = mc.readL16(s + 27);

			if (samples > 0 && s + instsize < mc.size())
			{
				size_t shsz = mc.readL32(s + 29);
				if (shsz < 40)
					return ret;
				s += instsize;
				size_t samplesize = 0;
				for (size_t j = 0; j < samples && s + shsz < mc.size(); ++j)
				{
					size_t smsz = mc.readL32(s);
					comment     = wxString::From8BitData(data + s + 18, 22);
					comment.Trim();
					comment = wxString::Format("%s", comment);
					if (comment.length())
						ret += wxString::Format("%i-%i: %s\n", i, j, comment);
					s += shsz;
					samplesize += smsz;
				}
				s += samplesize;
			}
			else
				s += instsize;
		}
		else
			return ret;
	}

	return ret;
}

wxString audio::getSunInfo(MemChunk& mc)
{
	size_t datasize   = mc.readB32(8);
	size_t codec      = mc.readB32(12);
	size_t samplerate = mc.readB32(16);
	size_t channels   = mc.readB32(20);

	wxString format = "Format: ";
	switch (codec)
	{
	case 1: format += wxString::FromUTF8("\xCE\xBC-Law"); break;
	case 2:
	case 3:
	case 4:
	case 5: format += wxString::Format("PCM (signed)"); break;
	case 6:
	case 7: format += wxString::Format("PCM (float)"); break;
	case 27: format += wxString::Format("a-Law"); break;
	default: format += wxString::Format("Unknown (%lu)", codec); break;
	}
	wxString ret = "Mono";
	if (channels == 2)
		ret = "Stereo";
	else if (channels > 2)
		ret = wxString::Format("%lu channels", channels);
	int bps = 1;
	if (codec > 1 && codec < 6)
		bps = codec - 1;
	else if (codec == 6 || codec == 7)
		bps = codec - 2;
	size_t samples = datasize / bps;
	ret += wxString::Format(" %u-bit", bps * 8);
	ret += wxString::Format(" sound with %lu samples at %lu Hz\n%s\n", samples, samplerate, format);

	return ret;
}

wxString audio::getVocInfo(MemChunk& mc)
{
	int         codec      = -1;
	int         blockcount = 0;
	size_t      datasize   = 0;
	size_t      i = 26, e = mc.size();
	bool        gotextra = false;
	WavFmtChunk fmtchunk;
	while (i < e)
	{
		// Parses through blocks
		uint8_t blocktype = mc[i];
		size_t  blocksize = mc.readL24(i + 1);
		i += 4;
		if (i + blocksize > e && blocktype != 0)
		{
			return wxString::Format("Invalid sound: VOC file cut abruptly in block %i (offset %i)", blockcount, i - 4);
		}
		blockcount++;
		switch (blocktype)
		{
		case 0: // Terminator, the rest should be ignored
			i = e;
			break;
		case 1: // Sound data
			if (!gotextra && codec >= 0 && codec != mc[i + 1])
				return "Invalid sound: VOC files with different codecs are not supported";
			else if (codec == -1)
			{
				fmtchunk.samplerate = 1000000 / (256 - mc[i]);
				fmtchunk.channels   = 1;
				fmtchunk.tag        = 1;
				codec               = mc[i + 1];
			}
			datasize += blocksize - 2;
			break;
		case 2: // Sound data continuation
			if (codec == -1)
				return "Invalid sound: Sound data without codec in VOC file";
			datasize += blocksize;
			break;
		case 3: // Silence
		case 4: // Marker
		case 5: // Text
		case 6: // Repeat start point
		case 7: // Repeat end point
			break;
		case 8: // Extra info, overrides any following sound data codec info
			if (codec != -1)
			{
				return "Invalid sound: Extra info block must precede sound data info block in VOC file";
			}
			else
			{
				fmtchunk.samplerate = 256000000 / ((mc[i + 3] + 1) * (65536 - mc.readL16(i)));
				fmtchunk.channels   = mc[i + 3] + 1;
				fmtchunk.tag        = 1;
				codec               = mc[i + 2];
			}
			break;
		case 9: // Sound data in new format
			if (codec >= 0 && codec != mc.readL16(i + 6))
				return "Invalid sound: VOC files with different codecs are not supported";
			else if (codec == -1)
			{
				fmtchunk.samplerate = mc.readL32(i);
				fmtchunk.bps        = mc[i + 4];
				fmtchunk.channels   = mc[i + 5];
				fmtchunk.tag        = 1;
				codec               = mc.readL16(i + 6);
			}
			datasize += blocksize - 12;
			break;
		}
		i += blocksize;
	}
	wxString format = "Format: ";
	switch (codec)
	{
	case 0: format += wxString::Format("PCM (unsigned)"); break;
	case 1: format += wxString::Format("4-to-8 ADPCM"); break;
	case 2: format += wxString::Format("3-to-8 ADPCM"); break;
	case 3: format += wxString::Format("2-to-8 ADPCM"); break;
	case 4: format += wxString::Format("PCM (signed)"); break;
	case 6: format += wxString::Format("a-Law"); break;
	case 7: format += wxString::FromUTF8("\xCE\xBC-Law"); break;
	case 0x200: format += wxString::Format("4to-16 ADPCM"); break;
	default: format += wxString::Format("Unknown (%u)", codec); break;
	}
	wxString ret = "Mono";
	if (fmtchunk.channels == 2)
		ret = "Stereo";
	else if (fmtchunk.channels > 2)
		ret = wxString::Format("%u channels", fmtchunk.channels);
	size_t samples = datasize / (codec == 4 ? 2 : 1);
	ret += wxString::Format(" %u-bit", codec == 4 ? 16 : 8);
	ret += wxString::Format(" sound with %lu samples at %u Hz\n%s\n", samples, fmtchunk.samplerate, format);
	ret += wxString::Format("%d blocks\n", blockcount);
	return ret;
}

wxString audio::getWavInfo(MemChunk& mc)
{
	auto               data  = (const char*)mc.data();
	auto               udata = (const uint8_t*)data;
	auto               head  = (const WavChunk*)data;
	const WavChunk*    temp  = nullptr;
	const WavChunk*    wdat  = nullptr;
	const WavChunk*    fact  = nullptr;
	const WavChunk*    cue   = nullptr;
	const WavFmtChunk* fmt   = nullptr;
	size_t             s     = 12;

	wxString chunksfound = "Chunks: ";

	// Find data chunks
	while (s + 8 < mc.size())
	{
		temp = (const WavChunk*)(data + s);
		if (temp->id[0] == 'L' && temp->id[1] == 'I' && temp->id[2] == 'S' && temp->id[3] == 'T')
			chunksfound += wxString::Format(
				"%s_%c%c%c%c, ",
				wxString::From8BitData(temp->id, 4),
				data[s + 8],
				data[s + 9],
				data[s + 10],
				data[s + 11]);
		else
			chunksfound += wxString::Format("%s, ", wxString::From8BitData(temp->id, 4));

		if (temp->id[0] == 'f' && temp->id[1] == 'm' && temp->id[2] == 't' && temp->id[3] == ' ')
			fmt = (const WavFmtChunk*)temp;
		else if (temp->id[0] == 'd' && temp->id[1] == 'a' && temp->id[2] == 't' && temp->id[3] == 'a')
			wdat = temp;
		else if (temp->id[0] == 'f' && temp->id[1] == 'a' && temp->id[2] == 'c' && temp->id[3] == 't')
			fact = temp;
		else if (temp->id[0] == 'c' && temp->id[1] == 'u' && temp->id[2] == 'e' && temp->id[3] == ' ')
			cue = temp;
		size_t offset = 8 + wxUINT32_SWAP_ON_BE(temp->size);
		if (offset % 2)
			++offset;
		s += offset;
	}
	chunksfound.RemoveLast(2);

	if (wdat == nullptr || fmt == nullptr)
		return wxString::Format(
			"Invalid RIFF-WAVE file, %s",
			(fmt == nullptr) ? (wdat == nullptr) ? "no format or data" : "no format" : "no data");

	wxString format  = "Format: ";
	size_t   tag     = wxUINT16_SWAP_ON_BE(fmt->tag);
	size_t   formnum = tag;
	if (formnum == 65534)
	{
		format  = "Format: Extensible - ";
		formnum = wxUINT32_SWAP_ON_BE(fmt->guid[0]);
	}
	switch (formnum)
	{
	case 1: format += wxString::Format("PCM"); break;
	case 2: format += wxString::Format("Microsoft ADPCM"); break;
	case 3: format += wxString::Format("IEEE754"); break;
	case 6: format += wxString::Format("ITU G.711 a-Law"); break;
	case 7: format += wxString::FromUTF8("ITU G.711 \xCE\xBC-Law"); break;
	case 17: format += wxString::Format("IMA ADPCM"); break;
	case 20: format += wxString::Format("ITU G.723 ADPCM"); break;
	case 49: format += wxString::Format("GSM 6.10"); break;
	case 64: format += wxString::Format("ITU G.721 ADPCM"); break;
	case 85: format += wxString::Format("MPEG Layer 3"); break;
	default: format += wxString::Format("Unknown (%u)", fmt->tag); break;
	}
	wxString ret      = "Mono";
	size_t   channels = wxUINT16_SWAP_ON_BE(fmt->channels);
	if (channels == 2)
		ret = "Stereo";
	else if (channels > 2)
		ret = wxString::Format("%lu channels", channels);
	size_t smplsize = fmt->blocksize;
	size_t datasize = wdat->size;
	size_t samples  = datasize / (smplsize > 0 ? smplsize : 1);
	if (fact && wxUINT32_SWAP_ON_BE(fact->size) >= 4 && tag != 1)
	{
		size_t offset = (const uint8_t*)fact - udata + 8;
		samples       = memory::readL32(udata, offset);
	}
	size_t bps = wxUINT16_SWAP_ON_BE(fmt->bps);
	if (tag == 65534 && bps != 0)
		bps = wxUINT16_SWAP_ON_BE(fmt->vbps);
	if (bps == 0)
		ret += wxString::Format(" variable bit rate");
	else
		ret += wxString::Format(" %lu-bit", bps);
	size_t samplerate = wxUINT32_SWAP_ON_BE(fmt->samplerate);
	ret += wxString::Format(" sound with %lu samples at %lu Hz\n%s\n", samples, samplerate, format);
	if (tag == 65534 && fmt->channelmask > 0)
	{
		size_t   channelmask = wxUINT32_SWAP_ON_BE(fmt->channelmask);
		wxString channelstr  = "Channels: ";
		for (size_t i = 0; i < 18; ++i)
		{
			size_t mask = 1 << i;
			if (channelmask & mask)
			{
				channelstr += wxString::Format("%s", speaker_pos[i]);
				channelmask &= ~mask;
				if (channelmask && i < 17)
					channelstr += wxString::Format(", ");
			}
		}
		ret += wxString::Format("%s\n", channelstr);
	}

	// Parse metadata chunks
	ret += wxString::Format("\n%s%s\n", parseIFFChunks(mc, 12, samplerate, cue), chunksfound);
	return ret;
}

wxString audio::getRmidInfo(MemChunk& mc)
{
	auto            data  = (const char*)mc.data();
	auto            udata = (const uint8_t*)data;
	auto            head  = (const WavChunk*)data;
	const WavChunk* temp  = nullptr;
	const WavChunk* cue   = nullptr;
	size_t          s     = 12;

	wxString chunksfound = "Chunks: ";
	wxString ret         = "\n";

	// Find data chunks
	while (s + 8 < mc.size())
	{
		temp = (const WavChunk*)(data + s);
		if (temp->id[0] == 'L' && temp->id[1] == 'I' && temp->id[2] == 'S' && temp->id[3] == 'T')
			chunksfound += wxString::Format(
				"%s_%c%c%c%c, ",
				wxString::From8BitData(temp->id, 4),
				data[s + 8],
				data[s + 9],
				data[s + 10],
				data[s + 11]);
		else
			chunksfound += wxString::Format("%s, ", wxString::From8BitData(temp->id, 4));

		if (temp->id[0] == 'c' && temp->id[1] == 'u' && temp->id[2] == 'e' && temp->id[3] == ' ')
			cue = temp;
		size_t offset = 8 + wxUINT32_SWAP_ON_BE(temp->size);
		if (offset % 2)
			++offset;
		s += offset;
	}
	chunksfound.RemoveLast(2);

	// Find data chunks
	ret += wxString::Format("%s%s\n", parseIFFChunks(mc, 12, 1, cue), chunksfound);
	return ret;
}

wxString audio::getAiffInfo(MemChunk& mc)
{
	auto            data  = (const char*)mc.data();
	auto            udata = (const uint8_t*)data;
	auto            head  = (const WavChunk*)data;
	const AIFFComm* comm  = nullptr;
	const WavChunk* temp  = nullptr;
	const WavChunk* cue   = nullptr;
	size_t          s     = 12;

	wxString chunksfound = "Chunks: ";

	// Find data chunks
	while (s + 8 < mc.size())
	{
		temp = (const WavChunk*)(data + s);
		if (temp->id[0] == 'L' && temp->id[1] == 'I' && temp->id[2] == 'S' && temp->id[3] == 'T')
			chunksfound += wxString::Format(
				"%s_%c%c%c%c, ",
				wxString::From8BitData(temp->id, 4),
				data[s + 8],
				data[s + 9],
				data[s + 10],
				data[s + 11]);
		else
			chunksfound += wxString::Format("%s, ", wxString::From8BitData(temp->id, 4));

		if (temp->id[0] == 'C' && temp->id[1] == 'O' && temp->id[2] == 'M' && temp->id[3] == 'M')
			comm = (const AIFFComm*)temp;
		else if (temp->id[0] == 'c' && temp->id[1] == 'u' && temp->id[2] == 'e' && temp->id[3] == ' ')
			cue = temp;
		size_t offset = 8 + wxUINT32_SWAP_ON_LE(temp->size);
		if (offset % 2)
			++offset;
		s += offset;
	}
	chunksfound.RemoveLast(2);

	if (comm == nullptr)
		return wxString::Format("Invalid AIFF file, no common chunk");

	size_t samplerate = 1;
	// Frame rate calculations adapted from libsndfile
	// Negative or less than 1
	if (comm->xsr[0] & 0x80 || comm->xsr[0] <= 0x3F)
		samplerate = 1;
	// Too big
	else if (comm->xsr[0] > 0x40 || (comm->xsr[0] == 0x40 && comm->xsr[1] > 0x1C))
		samplerate = 800000000;
	// Sane value for a frame rate
	else
		samplerate = ((comm->xsr[2] << 23) | (comm->xsr[3] << 15) | (comm->xsr[4] << 7) | (comm->xsr[5] >> 1))
					 >> (29 - comm->xsr[1]);

	wxString format = "Format: ";
	if (mc[11] == 'C' && comm->size > 22) // AIFC has larger COMMon chunk
	{
		const char* pos = ((const char*)comm) + 26;
		format += wxString::From8BitData(pos + 5, *(pos + 4));
		format += wxString::Format(" (%s)", wxString::From8BitData(pos, 4));
	}
	else
		format += "PCM (none)";
	wxString ret      = "Mono";
	size_t   channels = wxUINT16_SWAP_ON_LE(comm->channels);
	if (channels == 2)
		ret = "Stereo";
	else if (channels > 2)
		ret = wxString::Format("%lu channels", channels);
	size_t frames  = wxUINT32_SWAP_ON_LE(comm->frames);
	size_t samples = frames * channels;
	size_t bps     = wxUINT16_SWAP_ON_LE(comm->bitsize);
	ret += wxString::Format(" %lu-bit", bps);
	if (channels > 1)
		ret += wxString::Format(
			" sound with %lu samples in %lu frames at %lu Hz\n%s\n", samples, frames, samplerate, format);
	else
		ret += wxString::Format(" sound with %lu samples at %lu Hz\n%s\n", samples, samplerate, format);


	// Find data chunks
	ret += wxString::Format("%s%s\n", parseIFFChunks(mc, 12, samplerate, cue, true), chunksfound);
	return ret;
}

// -----------------------------------------------------------------------------
// Looks whether the memory chunk starts with an ID3 tag, and if there is one,
// returns the index at which the true audio data begins.
// Returns 0 if there is no tag before audio data.
// -----------------------------------------------------------------------------
size_t audio::checkForTags(MemChunk& mc)
{
	// Check for empty wasted space at the beginning, since it's apparently
	// quite popular in MP3s to start with a useless blank frame.
	size_t s = 0;
	// Completely arbitrary limit to how long to seek for data.
	size_t limit = std::min<size_t>(1200, mc.size() / 16);
	if (mc[0] == 0)
	{
		while ((s < limit) && (mc[s] == 0))
			++s;
	}

	if (mc.size() > s + 14)
	{
		// Check for ID3 header (ID3v2). Version and revision numbers cannot be FF.
		// Only the four upper flags are valid.
		if (mc.size() > s + 14 && mc[s + 0] == 'I' && mc[s + 1] == 'D' && mc[s + 2] == '3' && mc[s + 3] != 0xFF
			&& mc[s + 4] != 0xFF && ((mc[s + 5] & 0x0F) == 0) && mc[s + 6] < 0x80 && mc[s + 7] < 0x80
			&& mc[s + 8] < 0x80 && mc[s + 9] < 0x80)
		{
			// Compute size. It is stored as a "synchsafe integer", that is to say,
			// a big-endian value where the highest bit of each byte is not used.
			size_t size = (mc[s + 6] << 21) + (mc[s + 7] << 14) + (mc[s + 8] << 7) + mc[s + 9] + 10;
			// If there is a footer, then add 10 more to the size
			if (mc[s + 5] & 0x10)
				size += 10;
			// Needs to be at least that big
			if (mc.size() >= size + 4)
				s += size;
			return size;
		}
		// Blank frame after ID3 tag, because MP3 is awful.
		while (mc[s] == 0 && s < limit)
			++s;
		// Sometimes, the frame start is off by one for some reason.
		if ((s + 4 < limit) && (mc[s] != 0xFF) && (mc[s + 1] == 0xFF))
			++s;
	}
	// It's also possible to get an ID3v1 (or v1.1) tag.
	// Though normally they're at the end of the file.
	if (mc.size() > s + 132)
	{
		// Check for ID3 header (ID3v1).
		if (mc[s + 0] == 'T' && mc[s + 1] == 'A' && mc[s + 2] == 'G')
		{
			return s + 128;
		}
	}
	return s;
}
