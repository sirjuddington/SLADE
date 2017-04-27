
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    AudioTags.cpp
 * Description: Functions for parsing metadata tags in audio files.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "AudioTags.h"

#pragma pack(1)
struct id3v1_t
{
	char		tag[3];	// TAG
	char		title[30];
	char		artist[30];
	char		album[30];
	char		year[4];
	char		comment[30];
	uint8_t		genre;
};

struct id3v1e_t
{
	char		tag[4];	// TAG+
	char		title[60];
	char		artist[60];
	char		album[60];
	uint8_t		speed;
	char		subgenre[30];
	char		start[6];
	char		stop[6];
};

struct oggpageheader_t
{
	char		tag[4];		// "OggS"
	uint8_t		version;	// should be 0
	uint8_t		type;		// should be 1, 2, or 4
	uint32_t	granule[2];	// abstract time marker
	uint32_t	serialnum;	// bitstream serial number
	uint32_t	pagenum;	// page sequence number, should always be 1 more than previous
	uint32_t	checksum;	// CRC-32 of entire page (including header but with blank checksum)
	uint8_t		segments;	// number of segments
};

struct vorbisheader_t
{
	uint8_t		packettype;	// 1 (identification), 3 (comment), or 5 (setup)
	char		tag[6];		// "vorbis"
};
struct vorbisid_t
{
	uint32_t	version;	// should be 0
	uint8_t		channels;	// should not be 0
	uint32_t	samplerate;	// should not be 0 either
	int32_t		maxbitrate;
	int32_t		nombitrate;
	int32_t		minbitrate;
	uint32_t	blocksize0;
	uint32_t	blocksize1;
	uint8_t		framingflag;
};
struct itheader_t
{
	uint32_t	id;				// "IMPM"
	char		songname[26];
	uint16_t	reserved1;		// 0x1004
	uint16_t	ordnum;
	uint16_t	insnum;
	uint16_t	smpnum;
	uint16_t	patnum;
	uint32_t	dontcare;
	uint16_t	flags;
	uint16_t	special;
	uint16_t	dontcare2[3];
	uint16_t	msglength;
	uint32_t	msgoffset;
	uint32_t	dontcare3[33];
};
struct s3mheader_t
{
	char		songname[28];
	uint32_t	dontcare;
	uint16_t	ordnum;
	uint16_t	insnum;
	uint16_t	patnum;
	uint32_t	dontcare2;
	uint16_t	version;
	uint32_t	scrm;		// "SCRM" = 0x4D524353
	uint16_t	dontcare3[7];
	uint16_t	special;
};
struct s3msample_t
{
	uint8_t		type;
	char		dosname[12];
	uint8_t		dontcare[3];
	uint32_t	dontcare2[8];
	char		comment[28];
	uint32_t	scr;	// either "SCRI" or "SCRS"
};
struct xmheader_t
{
	char		id[17];			// "Extended Module: " or "Extended module: "
	char		songname[20];	// song name
	uint8_t		reserved;		// 0x1a
	char		tracker[20];	// tracker program name
	uint16_t	version;		// either 0x0401 or 0x0301
	uint32_t	headersize;		// not counting the first 60 bytes
	uint32_t	dontcare;
	uint16_t	chnnum;
	uint16_t	patnum;
	uint16_t	insnum;
	// more after, but we don't care about them here
};
struct wav_chunk_t
{
	char id[4];
	uint32_t size;
};

struct wav_fmtchunk_t
{
	wav_chunk_t header;
	uint16_t tag;
	uint16_t channels;
	uint32_t samplerate;
	uint32_t datarate;
	uint16_t blocksize;
	uint16_t bps;
	uint16_t extsize;
	uint16_t vbps;
	uint32_t channelmask;
	uint32_t guid[4];
};
struct bext_chunk_t
{
	char		Description[256];
	char		Originator[32];
	char		OrigRef[32];
	char		OrigDate[10];
	char		OrigTime[8];
	uint32_t	TimeReferenceLow;
	uint32_t	TimeReferenceHigh;
	uint16_t	Version;
	uint8_t		UMID[64];
	uint16_t	LoudnessValue;
	uint16_t	LoudnessRange;
	uint16_t	MaxTruePeakLevel;
	uint16_t	MaxMomentaryLoudness;
	uint16_t	MaxShortTermLoudness;
	uint8_t		Reserved[180];
	char		CodingHistory[1];
};
struct	wav_cue_t
{
	uint32_t  dwName;
	uint32_t  dwPosition;
	char fccChunk[4];
	uint32_t  dwChunkStart;
	uint32_t  dwBlockStart;
	uint32_t  dwSampleOffset;
};
struct	aiff_comm_t
{
	uint32_t name;
	uint32_t size;
	uint16_t channels;
	uint32_t frames;
	uint16_t bitsize;
	uint8_t  xsr[10];
};
#pragma pack()

enum id3v2_frames
{
	ID3_COM = 0x434F4D,
	ID3_TAL = 0x54414C,
	ID3_TCM = 0x54434D,
	ID3_TCO = 0x54434F,
	ID3_TCR = 0x544352,
	ID3_TOA = 0x544F41,
	ID3_TOL = 0x544F4C,
	ID3_TOT = 0x544F54,
	ID3_TP1 = 0x545031,
	ID3_TP2 = 0x545032,
	ID3_TP3 = 0x545033,
	ID3_TP4 = 0x545034,
	ID3_TRK = 0x54524B,
	ID3_TT1 = 0x545431,
	ID3_TT2 = 0x545432,
	ID3_TT3 = 0x545433,
	ID3_TXT = 0x545854,
	ID3_TYE = 0x545945,
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

static const char * const id3v1_genres[] =
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
static const char * const speaker_pos[] =
{
	"Front Left", "Front Right", "Front Center",
	"Low Frequency", "Back Left", "Back Right",
	"Front Left of Center", "Front Right of Center",
	"Back Center", "Side Left", "Side Right", "Top Center",
	"Top Front Left", "Top Front Center", "Top Front Right",
	"Top Back Left", "Top Back Center", "Top Back Right",
};

string BuildID3v2GenreString(string content)
{
	string genre = "";
	size_t i = 0;
	while (i < content.length())
	{
		if (content[i] == '(' && content[i+1] != '(')
		{
			if (content[i+1] == 'R' && content[i+2] == 'X' && content[i+3] == ')')
			{
				genre += "Remix";
				i += 4;
			}
			else if (content [i+1] == 'C' && content[i+2] == 'R' && content[i+3] == ')')
			{
				genre += "Cover";
				i += 4;
			}
			else
			{
				int index = 0;
				int j = i+1;
				while (content[j] >= '0' && content[j] <= '9' && index < 192)
				{
					index = index * 10 + (content[j] - '0');
					++j;
				}
				if (content[j] != ')')
					index = 192;
				if (index < 192)
					genre += string::FromAscii(id3v1_genres[index]);
				i = j+1;
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

string ParseID3v1Tag(MemChunk& mc, size_t start)
{
	id3v1_t tag;
	string version, title, artist, album, comment, genre, year;
	int track = 0;

	mc.read(&tag, 128, start);
	title = string::FromAscii(tag.title, 30);
	artist = string::FromAscii(tag.artist, 30);
	album = string::FromAscii(tag.album, 30);
	comment = string::FromAscii(tag.comment, 30);
	year = string::FromAscii(tag.year, 4);
	genre = (tag.genre < 192) ? string::FromAscii(id3v1_genres[tag.genre]) : "";
	if (tag.comment[28] == 0 && tag.comment[29] != 0)
	{
		version = "ID3v1.1";
		track = tag.comment[29];
	}
	else version = "ID3v1.0";

	// Check for extended tag
	if (start > 256 && mc[start-227] == 'T' && mc[start-226] == 'A' && mc[start-225] == 'G' && mc[start-224] == '+')
	{
		id3v1e_t etag;
		version += '+';

		mc.read(&etag, 227, start - 227);
		title += string::FromAscii(etag.title, 60);
		artist += string::FromAscii(etag.artist, 60);
		album += string::FromAscii(etag.album, 60);
		genre += S_FMT(" (%s)", etag.subgenre);
	}
	string ret = version + '\n';
	if (title.length())		ret += S_FMT("Title: %s\n", title);
	if (album.length())		ret += S_FMT("Album: %s\n", album);
	if (track != 0)			ret += S_FMT("Track: %d\n", track);
	if (artist.length())	ret += S_FMT("Artist: %s\n", artist);
	if (year.length())		ret += S_FMT("Year: %s\n", year);
	if (genre.length())		ret += S_FMT("Genre: %s\n", genre);
	ret += "\n";
	return ret;
}

string ParseID3v2Tag(MemChunk& mc, size_t start)
{
	string version, title, artist, composer, copyright, album, genre, year, group, subtitle, track, comments;
	bool artists = false;

	version = S_FMT("ID3v2.%d.%d", mc[start+3], mc[start+4]);
	bool v22 = mc[start+3] < 3;

	// ID3v2.2 frame headers have a size of 6 (3 byte identifier, 3 byte size).
	// ID3v2.3 and v2.4 frame headers have a size of 10 (4 byte identifier, 4 byte size, 2 byte flags)
	size_t step = v22 ? 6 : 10;

	// Compute synchsafe size
	size_t size = (mc[start+6] << 21) + (mc[start+7] << 14) + (mc[start+8] << 7) + mc[start+9] + 10;
	size_t end = start + size;

	// ID3v2.2 tag frames have three-char identifiers.
	// We only care about a little subset of them, though.
	size_t s = start + 10;

	// Iterate through frames. The minimal size of a frame is 1 byte of data.
	while (s + step + 1 < end)
	{
		size_t fsize = v22 ? READ_B24(mc, s+3) : READ_B32(mc, s+4);

		// One byte for encoding
		size_t tsize = fsize - 1;

		// Process only text frames that aren't empty
		// Also skip flags, not gonna bother with encryption or compression
		if ((mc[s] == 'T' || (mc[s] == 'C' && mc[s+1] == 'O' && mc[s+2] == 'M'))
			&& tsize > 0 && (v22 || mc[s+8] == 0 && mc[s+9] == 0))
		{
			string content;

			// First step: retrieve the text (UTF-16 massively sucks)
			char * buffer = new char[fsize];
			mc.read(buffer, tsize, s+step+1);
			size_t frame = v22 ? READ_B24(mc, s) : READ_B32(mc, s);

			bool bomle = true;
			size_t bom = 0;
			switch (mc[s+step])
			{
			case 0: // Plain old ASCII
				content = string::From8BitData(buffer, tsize);
				break;
			case 1:	// UTF-16 with byte order mark
				{
					size_t i = 1;
					while (i < tsize - 3)
					{
						bom = i + 1;
						// Looks like stuffing garbage before the actual content is popular
						if (mc[s+step+i] == 0xFF && mc[s+step+i+1] == 0xFE && mc[s+step+i+2] != 0)
						{
							bomle = true;
							break;
						}
						if (mc[s+step+i] == 0xFE && mc[s+step+i+1] == 0xFF && mc[s+step+i+3] != 0)
						{
							bomle = false;
							break;
						}
						++i;
					}
				}
				// Fall through
			case 2:	// UTF-16 without byte order mark
				{
					// You're right, wxWidgets. Code like this is so much 
					// better than having something like wxString::FromUTF16()
					size_t u16c = (tsize - bom) / 2;
					wchar_t * wchars = new wchar_t[u16c];
					for (size_t i = 0; i < u16c; ++i)
					{
						if (bomle)
							wchars[i] = (wchar_t)READ_L16(buffer, bom+2*i);
						else
							wchars[i] = (wchar_t)READ_B16(buffer, bom+2*i);
					}
					content = string(wchars, u16c);
					delete[] wchars;
				}
				break;
			case 3:	// UTF-8
				content = string::FromUTF8(buffer, tsize);
				break;
			}
			delete[] buffer;

			// Second step: treat frame accordingly to type
			switch (frame)
			{
			case ID3_COM:	// Comments
			case ID3_COMM:	// Comments
				if (comments.length())
					comments += "\n\n";
				comments += content;
				break;
			case ID3_TAL:	// Album/Movie/Show title
			case ID3_TOT:	// Original album/movie/show title
			case ID3_TALB:	// Album/Movie/Show title
			case ID3_TOAL:	// Original album/movie/show title
				if (album.length())
					album += " / ";
				album += content;
				break;
			case ID3_TCM:	// Composer
			case ID3_TCOM:	// Composer
				composer = content;
				break;
			case ID3_TCO:	// Content type
			case ID3_TCON:	// Content type
				genre = BuildID3v2GenreString(content);
				break;
			case ID3_TCR:	// Copyright message
			case ID3_TCOP:	// Copyright message
				copyright = content;
				break;
			case ID3_TOA:	// Original artist(s)/performer(s)
			case ID3_TOL:	// Original Lyricist(s)/text writer(s)
			case ID3_TP1:	// Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group
			case ID3_TP2:	// Band/Orchestra/Accompaniment
			case ID3_TP3:	// Conductor/Performer refinement
			case ID3_TP4:	// Interpreted, remixed, or otherwise modified by
			case ID3_TXT:	// Lyricist/text writer
			case ID3_TEXT:	// Lyricist/Text writer
			case ID3_TOLY:	// Original lyricist(s)/text writer(s)
			case ID3_TOPE:	// Original artist(s)/performer(s)
			case ID3_TPE1:	// Lead performer(s)/Soloist(s)
			case ID3_TPE2:	// Band/orchestra/accompaniment
			case ID3_TPE3:	// Conductor/performer refinement
			case ID3_TPE4:	// Interpreted, remixed, or otherwise modified by
				if (artist.length())
				{
					artist += " / ";
					artists = true;
				}
				artist += content;
				break;
			case ID3_TRK:	// Track number/Position in set
			case ID3_TRCK:	// Track number/Position in set
				track = content;
				break;
			case ID3_TT1:	// Content group description
			case ID3_TIT1:	// Content group description
				group = content;
				break;
			case ID3_TT2:	// Title/Songname/Content description
			case ID3_TIT2:	// Title/songname/content description
				title = content;
				break;
			case ID3_TT3:	// Subtitle/Description refinement
			case ID3_TIT3:	// Subtitle/Description refinement
				group = content;
				break;
			case ID3_TYE:	// Year
			case ID3_TYER:	// Year
				year = content;
				break;
			case ID3_TDRC:	// Recording time (precision varies between yyyy and yyyy-MM-ddTHH:mm:ss)
				year = content.Left(4);
				break;
			}
		}
		// Parsing stops when padding starts
		else if (mc[s] == 0 && fsize == 0)
			break;
		s += (fsize + step);
	}

	string ret = version + '\n';
	if (group.length())		ret += S_FMT("Group: %s\n", group);
	if (title.length())		ret += S_FMT("Title: %s\n", title);
	if (album.length())		ret += S_FMT("Album: %s\n", album);
	if (track.length())		ret += S_FMT("Track: %s\n", track);
	if (artist.length())	ret += S_FMT("Artist%s: %s\n", artists ? "s" : "", artist);
	if (copyright.length())	ret += S_FMT("Copyright \x00A9 %s\n", copyright);
	if (year.length())		ret += S_FMT("Year: %s\n", year);
	if (genre.length())		ret += S_FMT("Genre: %s\n", genre);
	if (comments.length())	ret += S_FMT("Comments:\n%s\n", comments);
	ret += "\n";
	return ret;
}

#include <SFML/System.hpp>

string ParseVorbisComment(MemChunk& mc, size_t start)
{
	sf::Clock timer;
	string ret;
	const char * data = (const char *) mc.getData();
	size_t end = mc.getSize();

	if (start + 10 > end)
		return ret + "\nInvalid Vorbis comment segment (A)\n";
	size_t strlen = READ_L32(mc, start);
	if (start + 10 + strlen > end)
		return ret + "\nInvalid Vorbis comment segment (B)\n";

	string vendor = string::FromUTF8(data + start + 4, strlen);

	size_t numcomments = READ_L32(mc, start + 4 + strlen);
	size_t s = start + 8 + strlen;

	for (size_t i = 0; i < numcomments && s + 6 < end; ++i)
	{
		strlen = READ_L32(mc, s);
		if (s + strlen + 4 > end)
			return ret + "\nInvalid Vorbis comment segment (C)\n";
		ret += string::FromUTF8(data + s + 4, strlen);
		ret += "\n";
		s += 4 + strlen;
	}

	ret += S_FMT("\nVendor string: %s\n", vendor);

	return ret;
}

string parseIFFChunks(MemChunk& mc, size_t s, size_t samplerate, const wav_chunk_t* cue, bool bigendian = false)
{
	const wav_chunk_t* temp = NULL;
	const char* data = (const char*)mc.getData();
	const uint8_t* udata = (const uint8_t*)data;
	string ret = "";

	while (s + 8 < mc.getSize())
	{
		temp = (const wav_chunk_t*) (data + s);
		size_t tempsize = bigendian ? wxUINT32_SWAP_ON_LE(temp->size) : wxUINT32_SWAP_ON_BE(temp->size);
		size_t offset = s + 8;
		size_t end = offset + tempsize;
		s = end;
		if (s % 2)
			s++;

		// Broadcast extensions (bext chunk)
		if (temp->id[0] == 'b' && temp->id[1] == 'e' && temp->id[2] == 'x' && temp->id[3] == 't' && tempsize >= 602)
		{
			string bextstr = "Broadcast extensions:\n";
			const bext_chunk_t* bext = (const bext_chunk_t*) (data+offset);
			if (bext->Description[0])	bextstr += S_FMT("Description: %s\n", string::From8BitData(bext->Description, 256));
			if (bext->Originator[0])	bextstr += S_FMT("Originator: %s\n", string::From8BitData(bext->Originator, 32));
			if (bext->OrigRef[0])		bextstr += S_FMT("Reference: %s\n", string::From8BitData(bext->OrigRef, 32));
			if (bext->OrigDate[0])		bextstr += S_FMT("Date: %s\n", string::From8BitData(bext->OrigDate, 10));
			if (bext->OrigTime[0])		bextstr += S_FMT("Time: %s\n", string::From8BitData(bext->OrigTime, 8));
			if (bext->TimeReferenceLow | bext->TimeReferenceHigh)
			{
				uint64_t timeref = wxUINT32_SWAP_ON_BE(bext->TimeReferenceLow) + 
					(((uint64_t)(wxUINT32_SWAP_ON_BE(bext->TimeReferenceHigh))<<32));
				double timesec = (double)timeref / (double)samplerate;
				size_t milsec = 1000*timesec;
				size_t sec = (milsec / 1000) % 60;
				size_t min = (milsec / 60000) % 60;
				size_t hor = (milsec / 3600000) % 24;
				milsec %= 1000;
				bextstr += S_FMT("Time Reference: %d:%02d:%02d.%03d\n", hor, min, sec, milsec);
			}
			bextstr += S_FMT("BWFVersion: %d\n", wxUINT16_SWAP_ON_BE(bext->Version));
			if (bext->LoudnessValue)		bextstr += S_FMT("Integrated Loudness: %d\n", wxUINT16_SWAP_ON_BE(bext->LoudnessValue));
			if (bext->LoudnessRange)		bextstr += S_FMT("Loudness Range: %d\n", wxUINT16_SWAP_ON_BE(bext->LoudnessRange));
			if (bext->MaxTruePeakLevel)		bextstr += S_FMT("Maximum True Peak Level: %d\n", wxUINT16_SWAP_ON_BE(bext->MaxTruePeakLevel));
			if (bext->MaxMomentaryLoudness)	bextstr += S_FMT("Highest Momentary Loudness Level: %d\n", wxUINT16_SWAP_ON_BE(bext->MaxMomentaryLoudness));
			if (bext->MaxShortTermLoudness)	bextstr += S_FMT("Highest Short-Term Loudness Level: %d\n", wxUINT16_SWAP_ON_BE(bext->MaxShortTermLoudness));
			if (tempsize > 602 && bext->CodingHistory[0])
											bextstr += S_FMT("History: %s\n", string::From8BitData(bext->CodingHistory, tempsize - 602));

			ret += S_FMT("%s\n", bextstr);
		}
		// ID3 tag (yes, they may happen in a WAV, Audacity embeds them in an 'id3 ' chunk so they're still valid RIFF files)
		if (temp->id[0] == 'i' && temp->id[1] == 'd' && temp->id[2] == '3' && temp->id[3] == ' ' && tempsize > 14)
		{
			if (mc[offset] == 'T' && mc[offset+1] == 'A' && mc[offset+2] == 'G')
				ret += ParseID3v1Tag(mc, offset);
			else if (mc[offset] == 'I' && mc[offset+1] == 'D' && mc[offset+2] == '3')
				ret += ParseID3v2Tag(mc, offset);
		}
		// RIFF Data Listing
		if (temp->id[0] == 'L' && temp->id[1] == 'I' && temp->id[2] == 'S' && temp->id[3] == 'T' && tempsize > 4)
		{
			// LIST INFO chunk
			if (mc[offset] == 'I' && mc[offset+1] == 'N' && mc[offset+2] == 'F' && mc[offset+3] == 'O')
			{
				string liststr = "Information:\n";
				offset += 4;
				while (offset + 8 < end)
				{
					const wav_chunk_t* chunk = (const wav_chunk_t*)(data + offset);
					size_t chsz = wxUINT32_SWAP_ON_BE(chunk->size);
					string tagname = S_FMT("%s: ", string::From8BitData(chunk->id, 4));
					offset += 8;
					if (offset + chsz > end)
						break;
					else if (chunk->id[0] == 'I')
					{
						if (chunk->id[1] == 'A' && chunk->id[2] == 'R')
						{
							if (chunk->id[3] == 'L')			tagname = "Archival Location: ";
							else if (chunk->id[3] == 'T')		tagname = "Artist: ";
						}
						else if (chunk->id[1] == 'C')
						{
							if (chunk->id[2] == 'M' && chunk->id[3] == 'S')			tagname = "Commissioned: ";
							else if (chunk->id[2] == 'M' && chunk->id[3] == 'T')	tagname = "Comment: ";
							else if (chunk->id[2] == 'O' && chunk->id[3] == 'P')	tagname = "Copyright: ";
							else if (chunk->id[2] == 'R' && chunk->id[3] == 'D')	tagname = "Date Created: ";
							else if (chunk->id[2] == 'R' && chunk->id[3] == 'P')	tagname = "Cropped: ";
						}
						else if (chunk->id[1] == 'E' && chunk->id[2] == 'N' && chunk->id[3] == 'G')	tagname = "Engineer: ";
						else if (chunk->id[1] == 'G' && chunk->id[2] == 'N' && chunk->id[3] == 'R')	tagname = "Genre: ";
						else if (chunk->id[1] == 'K' && chunk->id[2] == 'E' && chunk->id[3] == 'Y')	tagname = "Keywords: ";
						else if (chunk->id[1] == 'M' && chunk->id[2] == 'E' && chunk->id[3] == 'D')	tagname = "Medium: ";
						else if (chunk->id[1] == 'N' && chunk->id[2] == 'A' && chunk->id[3] == 'M')	tagname = "Title: ";
						else if (chunk->id[1] == 'P' && chunk->id[2] == 'R' && chunk->id[3] == 'D')	tagname = "Product: ";
						else if (chunk->id[1] == 'S' && chunk->id[2] == 'B' && chunk->id[3] == 'J')	tagname = "Subject: ";
						else if (chunk->id[1] == 'S' && chunk->id[2] == 'F' && chunk->id[3] == 'T')	tagname = "Software: ";
						else if (chunk->id[1] == 'S' && chunk->id[2] == 'R' && chunk->id[3] == 'C')	tagname = "Source: ";
						else if (chunk->id[1] == 'S' && chunk->id[2] == 'R' && chunk->id[3] == 'F')	tagname = "Source Form: ";
						else if (chunk->id[1] == 'T' && chunk->id[2] == 'C' && chunk->id[3] == 'H')	tagname = "Technician: ";
					}
					liststr += S_FMT("%s%s\n", tagname, string::From8BitData(data + offset, chsz));
					offset += chsz;
					if (offset % 2)
						offset++;
				}
				ret += S_FMT("%s\n", liststr);
			}
			// LIST adtl chunk
			else if (mc[offset] == 'a' && mc[offset+1] == 'd' && mc[offset+2] == 't' && mc[offset+3] == 'l')
			{
				// We need to have a cue chunk, wav specs say there can be only one at most
				if (cue && wxUINT32_SWAP_ON_BE(cue->size) >= 4)
				{
					size_t cueofs = 8 + (const char *)cue - data;
					size_t cuesize = wxUINT32_SWAP_ON_BE(cue->size);
					size_t numcuepoints = READ_L32(udata, cueofs);
					bool * alreadylisted = new bool[numcuepoints];
					memset(alreadylisted, false, numcuepoints * sizeof(bool));
					if (cuesize >= 4 + numcuepoints * sizeof(wav_cue_t))
					{
						string liststr = S_FMT("Associated Data List:\n%d cue points\n", numcuepoints);
						const wav_cue_t * cuepoints = (const wav_cue_t *)(data + cueofs + 4);
						size_t ioffset = offset + 4;
						while (ioffset < end)
						{
							const wav_chunk_t * note = (const wav_chunk_t *)(data + ioffset);
							size_t isize = wxUINT32_SWAP_ON_BE(note->size);
							ioffset += 8;
							size_t cuepoint = READ_L32(udata, ioffset);
							int cpindex = -1;
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
								liststr += S_FMT("Cue point %d: sample %d from %s, offset %d, block offset %d, chunk %d\n",
									cuepoint, wxUINT32_SWAP_ON_BE(cuepoints[cpindex].dwPosition),
									string::From8BitData(cuepoints[cpindex].fccChunk, 4), 
									wxUINT32_SWAP_ON_BE(cuepoints[cpindex].dwSampleOffset), 
									wxUINT32_SWAP_ON_BE(cuepoints[cpindex].dwBlockStart), 
									wxUINT32_SWAP_ON_BE(cuepoints[cpindex].dwChunkStart));
								alreadylisted[cpindex] = true;
							}
							if (note->id[0] == 'l' && note->id[1] == 'a' && note->id[2] == 'b' && note->id[3] == 'l')
							{
								string content = string::From8BitData(data+ioffset+4, isize-4); content.Trim();
								liststr += S_FMT("Cue point %d label: %s\n", cuepoint, content);
							}
							else if (note->id[0] == 'l' && note->id[1] == 't' && note->id[2] == 'x' && note->id[3] == 't')
							{
								liststr += S_FMT("Cue point %d: sample length %d, purpose %s\n", cuepoint, READ_L32(udata, (ioffset+4)),
									string::From8BitData(data+ioffset+8, 4));
							}
							else if (note->id[0] == 'n' && note->id[1] == 'o' && note->id[2] == 't' && note->id[3] == 'e')
							{
								string content = string::From8BitData(data+ioffset+4, isize-4); content.Trim();
								liststr += S_FMT("Cue point %d note: %s\n", cuepoint, content);
							}

							ioffset += isize;
						}
						ret += S_FMT("%s\n", liststr);
					}
				}
			}
		}
		// Other ASCII metadata, if they aren't too big
		else if (tempsize > 4 && tempsize < 8192)
		{
			bool pure_ascii = true;
			bool zerochar = false; // true if previous character was '\0'
			for (size_t i = 0; pure_ascii && i < tempsize; ++i)
			{
				// Allow them to have several substrings separated by single null bytes
				// This is found notably in afsp chunks.
				if (udata[offset+i] == 0)
				{
					// Two nulls in a row? Then break.
					if (zerochar == true)
					{
						pure_ascii = false;
						break;
					}
					zerochar = true;
				}
				// Only accept CR, LF, tabs, and printable characters
				else if ((udata[offset+i] < 0x20 && udata[offset+i] != 9 && udata[offset+i] != 10 && udata[offset+i] != 13) || udata[offset+i] > 0x7E)
				{
					pure_ascii = false;
					break;
				}
				else zerochar = false; // Reset zero status of previous character
			}
			if (pure_ascii)
			{
				char * asciidata = new char[tempsize];
				memcpy(asciidata, data + offset, tempsize);
				for (size_t i = 0; i < tempsize - 1; ++i)
					if (asciidata[i] == 0)
						asciidata[i] = '\n';

				ret += S_FMT("%s chunk:\n%s\n\n", string::From8BitData(temp->id, 4), string::From8BitData(asciidata, tempsize));
				delete[] asciidata;
			}
		}
	}
	return ret;
}


string Audio::getID3Tag(MemChunk& mc)
{
	// We actually identify RIFF-WAVE files as MP3 if they are encoded with
	// the MP3 codec, but that means the metadata format is different, so
	// call the RIFF-WAVE metadata function instead. We might end up finding
	// an ID3 tag anyway, provided it's nicely embedded in an "id3 " chunk.
	if (mc.getSize() > 64 && mc[0] == 'R' && mc[1] == 'I' && mc[2] == 'F' && mc[3] == 'F' &&
		mc[8] == 'W' && mc[9] == 'A' && mc[10] == 'V' && mc[11] == 'E')
		return getWavInfo(mc);

	string ret;
	// Check for empty wasted space at the beginning, since it's apparently
	// quite popular in MP3s to start with a useless blank frame.
	size_t s = 0;
	if (mc[0] == 0)
	{
		// Completely arbitrary limit to how long to seek for data.
		size_t limit = MIN(1200, mc.getSize()/16);
		while ((s < limit) && (mc[s] == 0))
			++s;
	}

	if (mc.getSize() > s+14)
	{
		// Check for ID3 header (ID3v2). Version and revision numbers cannot be FF.
		// Only the four upper flags are valid.
		while (mc.getSize() > s+14 && mc[s+0] == 'I' && mc[s+1] == 'D' && mc[s+2] == '3' &&
		        mc[s+3] != 0xFF && mc[s+4] != 0xFF && ((mc[s+5] & 0x0F) == 0) &&
		        mc[s+6] < 0x80 && mc[s+7] < 0x80 && mc[s+8] < 0x80 && mc[s+9] < 0x80)
		{
			ret += ParseID3v2Tag(mc, s);

			// Compute size. It is stored as a "synchsafe integer", that is to say,
			// a big-endian value where the highest bit of each byte is not used.
			size_t size = (mc[s+6] << 21) + (mc[s+7] << 14) + (mc[s+8] << 7) + mc[s+9] + 10;
			// If there is a footer, then add 10 more to the size
			if (mc[s+5] & 0x10) size += 10;
			// Needs to be at least that big
			if (mc.getSize() >= size + 4)
			s += size;
		}
	}
	// It's also possible to get an ID3v1 (or v1.1) tag.
	// Though normally they're at the end of the file.
	if (mc.getSize() > s+132)
	{
		// Check for ID3 header (ID3v1).
		if (mc[s+0] == 'T' && mc[s+1] == 'A' && mc[s+2] == 'G')
		{
			ret += ParseID3v1Tag(mc, s);
		}
	}
	// Look for ID3v1 tag at end of file.
	if (mc.getSize() > 132)
	{
		s = mc.getSize() - 128;
		// Check for ID3 header (ID3v1).
		if (mc[s+0] == 'T' && mc[s+1] == 'A' && mc[s+2] == 'G')
		{
			ret += ParseID3v1Tag(mc, s);
		}
	}
	return ret;
}

string Audio::getOggComments(MemChunk& mc)
{
	oggpageheader_t ogg;
	vorbisheader_t vorb;
	size_t pagestart = 58;
	size_t end = mc.getSize();
	string ret = "";

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
				if (vorb.packettype == 3 && vorb.tag[0] == 'v' && vorb.tag[1] == 'o' &&
					vorb.tag[2] == 'r' && vorb.tag[3] == 'b' && vorb.tag[4] == 'i' && vorb.tag[5] == 's')
				{
					ret += ParseVorbisComment(mc, datastart + 7);

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

string Audio::getFlacComments(MemChunk& mc)
{
	string ret = "";
	// FLAC files begin with identifier "fLaC"; skip them
	size_t s = 4;
	// FLAC metadata blocks have a 4-byte header
	while (s + 4 < mc.getSize())
	{
		// Last three bytes are big-endian value for size of metadata
		size_t blocksize = READ_B24(mc, s+1);

		// First byte contains block type and "last block" flag (128)
		// Type 4 is the VORBIS_COMMENT type
		if ((mc[s] & 0x7F) == 4)
			ret += ParseVorbisComment(mc, s+4);

		// If this was the last block, no need to keep processing
		if (mc[s] & 0x80)
			break;
		// Otherwise, keep on trucking to next block
		s+=4+blocksize;
	}
	return ret;
}

string Audio::getITComments(MemChunk& mc)
{
	const char* data = (const char*)mc.getData();
	const itheader_t* head = (const itheader_t*) data;
	size_t s = sizeof(itheader_t);

	// Get song name
	string ret = S_FMT("%s\n", string::From8BitData(head->songname, 26));

	// Get song comment, if any
	if ((wxUINT16_SWAP_ON_BE(head->special) & 1) && (wxUINT16_SWAP_ON_BE(head->msglength) > 0))
	{
		// To keep only valid strings, we trim whitespace and then print the string into itself.
		// The second step gets rid of strings full of invalid characters where length() does not
		// report the actual printable length correctly.
		string comment = string::From8BitData(data + wxUINT16_SWAP_ON_BE(head->msgoffset), wxUINT16_SWAP_ON_BE(head->msglength));
		comment.Trim(); comment = S_FMT("%s", comment);
		if (comment.length())
			ret += S_FMT("%s\n", comment);
	}

	// Get instrument comments
	size_t offset = s + wxUINT16_SWAP_ON_BE(head->ordnum);
	if (head->insnum)
		ret += S_FMT("\n%d instruments:\n", wxUINT16_SWAP_ON_BE(head->insnum));
	for (size_t i = 0; i < wxUINT16_SWAP_ON_BE(head->insnum); ++i)
	{
		size_t ofs = READ_L32(data, (offset + (i<<2)));
		if (ofs > offset && ofs + 60 < mc.getSize() && data[ofs] == 'I' && data[ofs+1] == 'M' && data[ofs+2] == 'P' && data[ofs+3] == 'I')
		{
			string instrument = string::From8BitData(data+ofs+4, 12);
			instrument.Trim(); instrument = S_FMT("%s", instrument);
			string comment = string::From8BitData(data+ofs+32, 26);
			comment.Trim(); comment = S_FMT("%s", comment);
			if (instrument.length() && comment.length())
				ret += S_FMT("%i: %s - %s\n", i, instrument, comment);
			else if (instrument.length())
				ret += S_FMT("%i: %s\n", i, instrument);
			else if (comment.length())
				ret += S_FMT("%i - %s\n", i, comment);
		}
	}
	
	// Get sample comments
	offset += wxUINT16_SWAP_ON_BE(head->insnum)<<2;
	if (head->smpnum)
		ret += S_FMT("\n%d samples:\n", wxUINT16_SWAP_ON_BE(head->smpnum));
	for (size_t i = 0; i < wxUINT16_SWAP_ON_BE(head->smpnum); ++i)
	{
		size_t pos = offset + (i<<2);
		size_t ofs = READ_L32(mc, pos);
		if (ofs > offset && ofs + 60 < mc.getSize() && data[ofs] == 'I' && data[ofs+1] == 'M' && data[ofs+2] == 'P' && data[ofs+3] == 'S')
		{
			string sample = string::From8BitData(data+ofs+4, 12);
			sample.Trim(); sample = S_FMT("%s", sample);
			string comment = string::From8BitData(data+ofs+20, 26);
			comment.Trim(); comment = S_FMT("%s", comment);
			if (sample.length() && comment.length())
				ret += S_FMT("%i: %s - %s\n", i, sample, comment);
			else if (sample.length())
				ret += S_FMT("%i: %s\n", i, sample);
			else if (comment.length())
				ret += S_FMT("%i - %s\n", i, comment);
		}
	}

	return ret;
}

string Audio::getModComments(MemChunk& mc)
{
	const char* data = (const char*)mc.getData();
	size_t s = 20;

	// Get song name
	string ret = S_FMT("%s\n", string::From8BitData(data, 20));

	// Get instrument/sample comments
	// We only recognize mods that have their magic identifier at offset 1080 (31 samples),
	// so no need to bother with mods that only have 15 samples (magic at offset 600).
	ret += "\n31 samples:\n";
	for (size_t i = 0; i < 31; ++i)
	{
		string comment = string::From8BitData(data + s, 22);
		comment.Trim(); comment = S_FMT("%s", comment);
		if (comment.length())
			ret += S_FMT("%i - %s\n", i, comment);

		// Move to next offset
		s += 30;
	}
	return ret;
}

string Audio::getS3MComments(MemChunk& mc)
{
	const char* data = (const char*)mc.getData();
	const s3mheader_t* head = (const s3mheader_t*) data;
	size_t s = 96;

	// Get song name
	string ret = S_FMT("%s\n", string::From8BitData(head->songname, 28));

	// Get instrument/sample comments
	if (head->insnum)
		ret += S_FMT("\n%d instruments and samples:\n", wxUINT16_SWAP_ON_BE(head->insnum));
	s += wxUINT16_SWAP_ON_BE(head->ordnum);
	for (size_t i = 0; i < wxUINT16_SWAP_ON_BE(head->insnum); ++i)
	{
		size_t t = (READ_L16(mc, (s+2*i)))<<4;
		if (t + 80 > mc.getSize())
			return ret;
		const s3msample_t* sample = (const s3msample_t*) (data+t);
		string dosname = string::From8BitData(sample->dosname, 12);
		dosname.Trim(); dosname = S_FMT("%s", dosname);
		string comment = string::From8BitData(sample->comment, 28);
		comment.Trim(); comment = S_FMT("%s", comment);
		if (dosname.length() && comment.length())
			ret += S_FMT("%i: %s - %s\n", i, dosname, comment);
		else if (dosname.length())
			ret += S_FMT("%i: %s\n", i, dosname);
		else if (comment.length())
			ret += S_FMT("%i - %s\n", i, comment);
	}
	return ret;
}

string Audio::getXMComments(MemChunk& mc)
{
	const char* data = (const char*)mc.getData();
	const xmheader_t* head = (const xmheader_t*) data;
	size_t s = 60 + wxUINT32_SWAP_ON_BE(head->headersize);

	// Get song name
	string ret = S_FMT("%s\n", string::From8BitData(head->songname, 20));

	// Get tracker name
	ret += S_FMT("Tracked with %s\n", string::From8BitData(head->tracker, 20));

	// Skip over patterns
	if (head->patnum)
		ret += S_FMT("\n%d patterns\n", wxUINT16_SWAP_ON_BE(head->patnum));
	for (size_t i = 0; i < wxUINT16_SWAP_ON_BE(head->patnum); ++i)
	{
		if (s + 9 < mc.getSize())
		{
			size_t patsize = READ_L32(mc, s) + READ_L16(mc, s+7);
			s+=patsize;
		}
		else return ret;
	}

	// Get instrument comments
	if (head->insnum)
		ret += S_FMT("\n%d instruments:\n", wxUINT16_SWAP_ON_BE(head->insnum));
	for (size_t i = 0; i < wxUINT16_SWAP_ON_BE(head->insnum); ++i)
	{
		if (s + 29 < mc.getSize())
		{
			size_t instsize = READ_L32(mc, s);
			if (instsize < 33)
				return ret;
			// To keep only valid strings, we trim whitespace and then print the string into itself.
			// The second step gets rid of strings full of invalid characters where length() does not
			// report the actual printable length correctly.
			string comment = string::From8BitData(data+s+4, 22);
			comment.Trim(); comment = S_FMT("%s", comment);
			if (comment.length())
				ret += S_FMT("%i: %s\n", i, comment);
			size_t samples = READ_L16(mc, s+27);

			if (samples > 0 && s + instsize < mc.getSize())
			{
				size_t shsz = READ_L32(mc, s+29);
				if (shsz < 40)
					return ret;
				s += instsize;
				size_t samplesize = 0;
				for (size_t j = 0; j < samples && s + shsz < mc.getSize(); ++j)
				{
					size_t smsz = READ_L32(mc, s);
					comment = string::From8BitData(data+s+18, 22);
					comment.Trim(); comment = S_FMT("%s", comment);
					if (comment.length())
						ret += S_FMT("%i-%i: %s\n", i, j, comment);
					s += shsz;
					samplesize += smsz;
				}
				s += samplesize;
			}
			else s+=instsize;
		}
		else return ret;
	}

	return ret;
}

string Audio::getSunInfo(MemChunk& mc)
{
	size_t datasize = READ_B32(mc, 8);
	size_t codec = READ_B32(mc, 12);
	size_t samplerate = READ_B32(mc, 16);
	size_t channels = READ_B32(mc, 20);

	string format = "Format: ";
	switch (codec)
	{
	case 1:		format += wxString::FromUTF8("\xCE\xBC-Law");	break;
	case 2:	
	case 3:	
	case 4:	
	case 5:		format += S_FMT("PCM (signed)");				break;
	case 6:	
	case 7:		format += S_FMT("PCM (float)");					break;
	case 27:	format += S_FMT("a-Law");						break;
	default:	format += S_FMT("Unknown (%u)", codec);			break;
	}
	string ret = "Mono";
	if (channels == 2)
		ret = "Stereo";
	else if (channels > 2)
		ret = S_FMT("%u channels", channels);
	int bps = 1;
	if (codec > 1 && codec < 6) bps = codec - 1;
	else if (codec == 6 || codec == 7) bps = codec - 2;
	size_t samples = datasize / bps;
	ret += S_FMT(" %u-bit", bps * 8);
	ret += S_FMT(" sound with %u samples at %u Hz\n%s\n", samples, samplerate, format);

	return ret;
}

string Audio::getVocInfo(MemChunk& mc)
{
	int codec = -1;
	int blockcount = 0;
	size_t datasize = 0;
	size_t i = 26, e = mc.getSize();
	bool gotextra = false;
	wav_fmtchunk_t fmtchunk;
	while (i < e)
	{
		// Parses through blocks
		uint8_t blocktype = mc[i];
		size_t blocksize = READ_L24(mc, i+1);
		i+=4;
		if (i + blocksize > e && blocktype != 0)
		{
			return S_FMT("Invalid sound: VOC file cut abruptly in block %i (offset %i)", blockcount, i-4);
		}
		blockcount++;
		switch (blocktype)
		{
		case 0: // Terminator, the rest should be ignored
			i = e; break;
		case 1: // Sound data
			if (!gotextra && codec >= 0 && codec != mc[i+1])
				return "Invalid sound: VOC files with different codecs are not supported";
			else if (codec == -1)
			{
				fmtchunk.samplerate = 1000000/(256 - mc[i]);
				fmtchunk.channels = 1;
				fmtchunk.tag = 1;
				codec = mc[i+1];
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
				fmtchunk.samplerate = 256000000/((mc[i+3] + 1) * (65536 - READ_L16(mc, i)));
				fmtchunk.channels = mc[i+3] + 1;
				fmtchunk.tag = 1;
				codec = mc[i+2];
			}
			break;
		case 9: // Sound data in new format
			if (codec >= 0 && codec != READ_L16(mc, i+6))
				return "Invalid sound: VOC files with different codecs are not supported";
			else if (codec == -1)
			{
				fmtchunk.samplerate = READ_L32(mc, i);
				fmtchunk.bps = mc[i+4];
				fmtchunk.channels = mc[i+5];
				fmtchunk.tag = 1;
				codec = READ_L16(mc, i+6);
			}
			datasize += blocksize - 12;
			break;
		}
		i += blocksize;
	}
	string format = "Format: ";
	switch (codec)
	{
	case 0:		format += S_FMT("PCM (unsigned)"); 				break;
	case 1:		format += S_FMT("4-to-8 ADPCM");				break;
	case 2:		format += S_FMT("3-to-8 ADPCM");				break;
	case 3:		format += S_FMT("2-to-8 ADPCM");				break;
	case 4:		format += S_FMT("PCM (signed)");				break;
	case 6:		format += S_FMT("a-Law");						break;
	case 7:		format += wxString::FromUTF8("\xCE\xBC-Law");	break;
	case 0x200:	format += S_FMT("4to-16 ADPCM");				break;
	default:	format += S_FMT("Unknown (%u)", codec);			break;
	}
	string ret = "Mono";
	if (fmtchunk.channels == 2)
		ret = "Stereo";
	else if (fmtchunk.channels > 2)
		ret = S_FMT("%u channels", fmtchunk.channels);
	size_t samples = datasize / (codec == 4 ? 2 : 1);
	ret += S_FMT(" %u-bit", codec == 4 ? 16 : 8);
	ret += S_FMT(" sound with %u samples at %u Hz\n%s\n", samples, fmtchunk.samplerate, format);
	ret += S_FMT("%d blocks\n", blockcount);
	return ret;
}

string Audio::getWavInfo(MemChunk& mc)
{
	const char* data = (const char*)mc.getData();
	const uint8_t* udata = (const uint8_t*)data;
	const wav_chunk_t* head = (const wav_chunk_t*)data;
	const wav_chunk_t* temp = NULL;
	const wav_chunk_t* wdat = NULL;
	const wav_chunk_t* fact = NULL;
	const wav_chunk_t* cue  = NULL;
	const wav_fmtchunk_t* fmt = NULL;
	size_t s = 12;

	string chunksfound = "Chunks: ";

	// Find data chunks
	while (s + 8 < mc.getSize())
	{
		temp = (const wav_chunk_t*) (data + s);
		if (temp->id[0] == 'L' && temp->id[1] == 'I' && temp->id[2] == 'S' && temp->id[3] == 'T')
			chunksfound += S_FMT("%s_%c%c%c%c, ", string::From8BitData(temp->id, 4), data[s+8], data[s+9], data[s+10], data[s+11]);
		else chunksfound += S_FMT("%s, ", string::From8BitData(temp->id, 4));
		
		if (temp->id[0] == 'f' && temp->id[1] == 'm' && temp->id[2] == 't' && temp->id[3] == ' ')
			fmt = (const wav_fmtchunk_t*) temp;
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

	if (wdat == NULL || fmt == NULL)
		return S_FMT("Invalid RIFF-WAVE file, %s", (fmt == NULL) ? (wdat == NULL) ? "no format or data" : "no format" : "no data");

	string format = "Format: ";
	size_t tag = wxUINT16_SWAP_ON_BE(fmt->tag);
	size_t formnum = tag;
	if (formnum == 65534)
	{
		format = "Format: Extensible - ";
		formnum = wxUINT32_SWAP_ON_BE(fmt->guid[0]);
	}
	switch (formnum)
	{
	case 1:		format += S_FMT("PCM"); 									break;
	case 2:		format += S_FMT("Microsoft ADPCM");							break;
	case 3:		format += S_FMT("IEEE754");									break;
	case 6:		format += S_FMT("ITU G.711 a-Law");							break;
	case 7:		format += wxString::FromUTF8("ITU G.711 \xCE\xBC-Law");		break;
	case 17:	format += S_FMT("IMA ADPCM");								break;
	case 20:	format += S_FMT("ITU G.723 ADPCM");							break;
	case 49:	format += S_FMT("GSM 6.10");								break;
	case 64:	format += S_FMT("ITU G.721 ADPCM");							break;
	case 85:	format += S_FMT("MPEG Layer 3");							break;
	default:	format += S_FMT("Unknown (%u)", fmt->tag);					break;
	}
	string ret = "Mono";
	size_t channels = wxUINT16_SWAP_ON_BE(fmt->channels);
	if (channels == 2)
		ret = "Stereo";
	else if (channels > 2)
		ret = S_FMT("%u channels", channels);
	size_t smplsize = fmt->blocksize;
	size_t datasize = wdat->size;
	size_t samples = datasize / (smplsize > 0 ? smplsize : 1);
	if (fact && wxUINT32_SWAP_ON_BE(fact->size) >= 4 && tag != 1)
	{
		size_t offset = (const uint8_t*)fact - udata + 8;
		samples = READ_L32(udata, offset);
	}
	size_t bps = wxUINT16_SWAP_ON_BE(fmt->bps);
	if (tag == 65534 && bps != 0)
		bps = wxUINT16_SWAP_ON_BE(fmt->vbps);
	if (bps == 0)
		ret += S_FMT(" variable bit rate");
	else ret += S_FMT(" %u-bit", bps);
	size_t samplerate = wxUINT32_SWAP_ON_BE(fmt->samplerate);
	ret += S_FMT(" sound with %u samples at %u Hz\n%s\n", samples, samplerate, format);
	if (tag == 65534 && fmt->channelmask > 0)
	{
		size_t channelmask = wxUINT32_SWAP_ON_BE(fmt->channelmask);
		string channelstr = "Channels: ";
		for (size_t i = 0; i < 18; ++i)
		{
			size_t mask = 1<<i;
			if (channelmask & mask)
			{
				channelstr += S_FMT("%s", speaker_pos[i]);
				channelmask &= ~mask;
				if (channelmask && i < 17)
					channelstr += S_FMT(", ");
			}
		}
		ret += S_FMT("%s\n", channelstr);
	}

	// Parse metadata chunks
	ret += S_FMT("\n%s%s\n", parseIFFChunks(mc, 12, samplerate, cue), chunksfound);
	return ret;
}

string Audio::getRmidInfo(MemChunk& mc)
{
	const char* data = (const char*)mc.getData();
	const uint8_t* udata = (const uint8_t*)data;
	const wav_chunk_t* head = (const wav_chunk_t*)data;
	const wav_chunk_t* temp = NULL;
	const wav_chunk_t* cue  = NULL;
	size_t s = 12;

	string chunksfound = "Chunks: ";
	string ret = "\n";

	// Find data chunks
	while (s + 8 < mc.getSize())
	{
		temp = (const wav_chunk_t*) (data + s);
		if (temp->id[0] == 'L' && temp->id[1] == 'I' && temp->id[2] == 'S' && temp->id[3] == 'T')
			chunksfound += S_FMT("%s_%c%c%c%c, ", string::From8BitData(temp->id, 4), data[s+8], data[s+9], data[s+10], data[s+11]);
		else chunksfound += S_FMT("%s, ", string::From8BitData(temp->id, 4));
		
		if (temp->id[0] == 'c' && temp->id[1] == 'u' && temp->id[2] == 'e' && temp->id[3] == ' ')
			cue = temp;
		size_t offset = 8 + wxUINT32_SWAP_ON_BE(temp->size);
		if (offset % 2)
			++offset;
		s += offset;
	}
	chunksfound.RemoveLast(2);

	// Find data chunks
	ret += S_FMT("%s%s\n", parseIFFChunks(mc, 12, 1, cue), chunksfound);
	return ret;
}

string Audio::getAiffInfo(MemChunk& mc)
{
	const char* data = (const char*)mc.getData();
	const uint8_t* udata = (const uint8_t*)data;
	const wav_chunk_t* head = (const wav_chunk_t*)data;
	const aiff_comm_t* comm = NULL;
	const wav_chunk_t* temp = NULL;
	const wav_chunk_t* cue  = NULL;
	size_t s = 12;

	string chunksfound = "Chunks: ";

	// Find data chunks
	while (s + 8 < mc.getSize())
	{
		temp = (const wav_chunk_t*) (data + s);
		if (temp->id[0] == 'L' && temp->id[1] == 'I' && temp->id[2] == 'S' && temp->id[3] == 'T')
			chunksfound += S_FMT("%s_%c%c%c%c, ", string::From8BitData(temp->id, 4), data[s+8], data[s+9], data[s+10], data[s+11]);
		else chunksfound += S_FMT("%s, ", string::From8BitData(temp->id, 4));
		
		if (temp->id[0] == 'C' && temp->id[1] == 'O' && temp->id[2] == 'M' && temp->id[3] == 'M')
			comm = (const aiff_comm_t*)temp;
		else if (temp->id[0] == 'c' && temp->id[1] == 'u' && temp->id[2] == 'e' && temp->id[3] == ' ')
			cue = temp;
		size_t offset = 8 + wxUINT32_SWAP_ON_LE(temp->size);
		if (offset % 2)
			++offset;
		s += offset;
	}
	chunksfound.RemoveLast(2);

	if (comm == NULL)
		return S_FMT("Invalid AIFF file, no common chunk");

	size_t samplerate = 1;
	// Frame rate calculations adapted from libsndfile
	 // Negative or less than 1
	if (comm->xsr[0] & 0x80 || comm->xsr[0] <= 0x3F)	samplerate = 1;
	// Too big
	else if (comm->xsr[0] > 0x40 || (comm->xsr[0] == 0x40 && comm->xsr[1] > 0x1C)) samplerate = 800000000;
	// Sane value for a frame rate
	else samplerate = ((comm->xsr[2] << 23) | (comm->xsr[3] << 15) | (comm->xsr[4] << 7) | (comm->xsr[5] >> 1)) >> (29 - comm->xsr[1]);

	string format = "Format: ";
	if (mc[11] == 'C' && comm->size > 22) // AIFC has larger COMMon chunk
	{
		const char* pos = ((const char*)comm) + 26;
		format += wxString::From8BitData(pos + 5, *(pos+4));
		format += S_FMT(" (%s)", wxString::From8BitData(pos, 4));
	}
	else format += "PCM (none)";
	string ret = "Mono";
	size_t channels = wxUINT16_SWAP_ON_LE(comm->channels);
	if (channels == 2)
		ret = "Stereo";
	else if (channels > 2)
		ret = S_FMT("%u channels", channels);
	size_t frames = wxUINT32_SWAP_ON_LE(comm->frames);
	size_t samples = frames * channels;
	size_t bps = wxUINT16_SWAP_ON_LE(comm->bitsize);
	ret += S_FMT(" %u-bit", bps);
	if (channels > 1)
		ret += S_FMT(" sound with %u samples in %u frames at %u Hz\n%s\n", samples, frames, samplerate, format);
	else 	ret += S_FMT(" sound with %u samples at %u Hz\n%s\n", samples, samplerate, format);


	// Find data chunks
	ret += S_FMT("%s%s\n", parseIFFChunks(mc, 12, samplerate, cue, true), chunksfound);
	return ret;
}
