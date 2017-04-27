#ifndef AUDIOFORMATS_H
#define AUDIOFORMATS_H

/* CheckForTags
 * Looks whether the memory chunk starts with an ID3 tag, and if
 * there is one, returns the index at which the true audio data
 * begins. Returns 0 if there is no tag before audio data.
 *******************************************************************/
size_t checkForTags(MemChunk& mc)
{
	// Check for empty wasted space at the beginning, since it's apparently
	// quite popular in MP3s to start with a useless blank frame.
	size_t s = 0;
	// Completely arbitrary limit to how long to seek for data.
	size_t limit = MIN(1200, mc.getSize()/16);
	if (mc[0] == 0)
	{
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
			// Compute size. It is stored as a "synchsafe integer", that is to say,
			// a big-endian value where the highest bit of each byte is not used.
			size_t size = (mc[s+6] << 21) + (mc[s+7] << 14) + (mc[s+8] << 7) + mc[s+9] + 10;
			// If there is a footer, then add 10 more to the size
			if (mc[s+5] & 0x10) size += 10;
			// Needs to be at least that big
			if (mc.getSize() >= size + 4)
				s += size;
			return size;
		}
		// Blank frame after ID3 tag, because MP3 is awful.
		while (mc[s] == 0 && s < limit)
			++s;
		// Sometimes, the frame start is off by one for some reason.
		if ((s + 4 < limit) && (mc[s] != 0xFF) && (mc[s+1] == 0xFF))
			++s;
	}
	// It's also possible to get an ID3v1 (or v1.1) tag.
	// Though normally they're at the end of the file.
	if (mc.getSize() > s+132)
	{
		// Check for ID3 header (ID3v1).
		if (mc[s+0] == 'T' && mc[s+1] == 'A' && mc[s+2] == 'G')
		{
			return s+128;
		}
	}
	return s;
}

class MUSDataFormat : public EntryDataFormat
{
public:
	MUSDataFormat() : EntryDataFormat("midi_mus") {};
	~MUSDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 16)
		{
			// Check for MUS header
			if (mc[0] == 'M' && mc[1] == 'U' && mc[2] == 'S' && mc[3] == 0x1A)
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class MIDIDataFormat : public EntryDataFormat
{
public:
	MIDIDataFormat() : EntryDataFormat("midi_smf") {};
	~MIDIDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 16)
		{
			// Check for MIDI header
			if (mc[0] == 'M' && mc[1] == 'T' && mc[2] == 'h' && mc[3] == 'd')
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class XMIDataFormat : public EntryDataFormat
{
public:
	XMIDataFormat() : EntryDataFormat("midi_xmi") {};
	~XMIDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 50)
		{
			// Check for XMI header: FORM, XDIRINFO, CAT, XMIDFORM, XMID
			if (mc[0] == 'F' && mc[1] == 'O' && mc[2] == 'R' && mc[3] == 'M' &&
			        mc[4] == 0 && mc[5] == 0 && mc [6] == 0 &&
			        mc[8] == 'X' && mc [9] == 'D' && mc[10] == 'I' && mc[11] == 'R' &&
			        mc[12] == 'I' && mc[13] == 'N' && mc[14] == 'F' && mc[15] == 'O' &&
			        mc[16] == 0 && mc[17] == 0 && mc [18] == 0 &&
			        mc[22] == 'C' && mc[23] == 'A' && mc[24] == 'T' &&
			        mc[30] == 'X' && mc[31] == 'M' && mc[32] == 'I' && mc[33] == 'D' &&
			        mc[34] == 'F' && mc[35] == 'O' && mc[36] == 'R' && mc[37] == 'M' &&
			        mc[42] == 'X' && mc[43] == 'M' && mc[44] == 'I' && mc[45] == 'D')
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class HMIDataFormat : public EntryDataFormat
{
public:
	HMIDataFormat() : EntryDataFormat("midi_hmi") {};
	~HMIDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 50)
		{
			// Check for HMI header: HMI-MIDI
			if (mc[0] == 'H' && mc[1] == 'M' && mc[2] == 'I' && mc[3] == '-' &&
			        mc[4] == 'M' && mc[5] == 'I' && mc[6] == 'D' && mc[7] == 'I')
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class HMPDataFormat : public EntryDataFormat
{
public:
	HMPDataFormat() : EntryDataFormat("midi_hmp") {};
	~HMPDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 50)
		{
			// Check for HMI header: HMIMIDIP
			if (mc[0] == 'H' && mc[1] == 'M' && mc[2] == 'I' && mc[3] == 'M' &&
			        mc[4] == 'I' && mc[5] == 'D' && mc[6] == 'I' && mc[7] == 'P')
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class GMIDDataFormat : public EntryDataFormat
{
public:
	GMIDDataFormat() : EntryDataFormat("midi_gmid") {};
	~GMIDDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 8)
		{
			// Check for GMID header: "MIDI", "GMD ", "ADL " or "ROL " followed by BE data size
			if (((mc[0] == 'M' && mc[1] == 'I' && mc[2] == 'D' && mc[3] == 'I') ||
				(mc[0] == 'G' && mc[1] == 'M' && mc[2] == 'D' && mc[3] == ' ') ||
				(mc[0] == 'A' && mc[1] == 'D' && mc[2] == 'L' && mc[3] == ' ') ||
				(mc[0] == 'R' && mc[1] == 'O' && mc[2] == 'L' && mc[3] == ' ')) &&
			        ((READ_B32(mc, 4) + 8) == mc.getSize()))
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class RMIDDataFormat : public EntryDataFormat
{
public:
	RMIDDataFormat() : EntryDataFormat("midi_rmid") {};
	~RMIDDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 36)
		{
			// Check for RIFF RMID header
			if (mc[0] == 'R' && mc[1] == 'I' && mc[2] == 'F' && mc[3] == 'F' &&
				mc[8] == 'R' && mc[9] == 'M' && mc[10] == 'I' && mc[11] == 'D' &&
				mc[12] == 'd' && mc[13] == 'a' && mc[14] == 't' && mc[15] == 'a' &&
				mc[20] == 'M' && mc[21] == 'T' && mc[22] == 'h' && mc[23] == 'd' &&
			        (READ_L32(mc, 4) + 8 == mc.getSize()) && 
					(READ_L32(mc, 16) < READ_L32(mc, 4)))
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class ITModuleDataFormat : public EntryDataFormat
{
public:
	ITModuleDataFormat() : EntryDataFormat("mod_it") {};
	~ITModuleDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 32)
		{
			// Check for IT header
			if (mc[0] == 'I' && mc[1] == 'M' && mc[2] == 'P' && mc[3] == 'M')
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class XMModuleDataFormat : public EntryDataFormat
{
public:
	XMModuleDataFormat() : EntryDataFormat("mod_xm") {};
	~XMModuleDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 80)
		{
			// Check for mod header
			char temp[18] = "";
			memcpy(temp, mc.getData(), 18);
			temp[17] = 0;
			if (temp[9] == 'M')
				temp[9] = 'm';
			if (!S_FMT("%s", temp).Cmp("Extended module: "))
			{
				if (mc[37] == 0x1a)
				{
					return EDF_TRUE;
				}
			}
		}

		return EDF_FALSE;
	}
};

class S3MModuleDataFormat : public EntryDataFormat
{
public:
	S3MModuleDataFormat() : EntryDataFormat("mod_s3m") {};
	~S3MModuleDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 60)
		{
			// Check for s3m header
			if (mc[44] == 'S' && mc[45] == 'C' && mc[46] == 'R' && mc[47] == 'M')
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class MODModuleDataFormat : public EntryDataFormat
{
public:
	MODModuleDataFormat() : EntryDataFormat("mod_mod") {};
	~MODModuleDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 1084)
		{
			// Check format; note: byte 951 is used by NoiseTracker to indicate restart byte so it can be any value
			if (mc[950] >= 0 && mc[950] <= 129 /*&& (mc[951] & 127) == 127*/)
			{
				if ((mc[1080] == 'M' && mc[1081] == '.' && mc[1082] == 'K' && mc[1083] == '.') ||
				        (mc[1080] == 'M' && mc[1081] == '!' && mc[1082] == 'K' && mc[1083] == '!') ||
				        (mc[1080] == 'F' && mc[1081] == 'L' && mc[1082] == 'T' && mc[1083] == '4') ||
				        (mc[1080] == 'F' && mc[1081] == 'L' && mc[1082] == 'T' && mc[1083] == '8') ||
						(mc[1080] >= '1' && mc[1080] <= '9' && mc[1081] >= '0' && mc[1081] <= '9' && mc[1082] == 'C' && mc[1083] == 'H') ||
				        (mc[1080] >= '2' && mc[1080] <= '8' && mc[1081] == 'C' && mc[1082] == 'H' && mc[1083] == 'N'))
					return EDF_TRUE;
			}
		}

		return EDF_FALSE;
	}
};

class OKTModuleDataFormat : public EntryDataFormat
{
public:
	OKTModuleDataFormat() : EntryDataFormat("mod_okt") {};
	~OKTModuleDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 1360)
		{
			// Check for mod header
			if (mc[ 0] == 'O' && mc[ 1] == 'K' && mc[ 2] == 'T' && mc[ 3] == 'A' &&
				mc[ 4] == 'S' && mc[ 5] == 'O' && mc[ 6] == 'N' && mc[ 7] == 'G' &&
				mc[ 8] == 'C' && mc[ 9] == 'M' && mc[10] == 'O' && mc[11] == 'D' &&
				mc[24] == 'S' && mc[25] == 'A' && mc[26] == 'M' && mc[27] == 'P')
			{
				return EDF_TRUE;
			}
		}

		return EDF_FALSE;
	}
};

class IMFDataFormat : public EntryDataFormat
{
public:
	IMFDataFormat() : EntryDataFormat("opl_imf") {};
	~IMFDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 13)
		{
			// Check format
			if (mc[0] == 'A' && mc[1] == 'D' && mc[2] == 'L' &&
			        mc[3] == 'I' && mc[4] == 'B' &&	mc[5] == 1 &&
			        mc[6] == 0 && mc[7] == 0 && mc[8] == 1)
			{
				return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

class IMFRawDataFormat : public EntryDataFormat
{
public:
	IMFRawDataFormat() : EntryDataFormat("opl_imf_raw") {};
	~IMFRawDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		// Check size
		if (size > 94 && size < 65535)
		{
			int ret = EDF_MAYBE;

			// Check data size info
			size_t datasize = READ_L16(mc, 0);
			if (datasize > size || (datasize != 0 && datasize + 92 < size))
				return EDF_FALSE;

			// So-called type 1 begins with data size, type 0 doesn't.
			// So we have a type-dependent offset here
			uint8_t tofs = datasize ? 2 : 0;
			size_t enough = datasize ? datasize : size;
			enough = MIN(enough, 160u+tofs);

			// First index command is usually writing 0 on register 0
			if (READ_L16(mc, tofs) != 0)
				ret = EDF_UNLIKELY;

			// Check data: uint8_t register, uint8_t data, uint16_t delay
			for (size_t i = 4+tofs; i < enough; i+=4)
			{
				uint8_t reg = mc[i];
				uint8_t rega = reg & 0xE0, regb = reg & 0x1F, regc = reg & 0x0F;
				if (reg == (i - tofs)/4)
				{
					// Hack for titlermx.imf
				}
				else if (reg == 0 && i > 0)
					return EDF_FALSE;
				else if (rega >= 0xA0 && rega <= 0xC0)
				{
					if (regc > 8 && reg != 0xBD)
						return EDF_FALSE;
				}
				else if ((rega >= 0x20 && rega <= 0x80) || rega == 0xE0)
				{
					if (regb > 15)
						return EDF_FALSE;
				}
				else if (rega == 0)
				{
					if (regb != 0 && regb != 4 && regb != 5 && regb != 8)
						return EDF_FALSE;
				}
			}
			// Figure that's probably good enough
			return ret;
		}
		return EDF_FALSE;
	}
};

class DRODataFormat : public EntryDataFormat
{
public:
	DRODataFormat() : EntryDataFormat("opl_dro") {};
	~DRODataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 20)
		{
			// Check format
			if (mc[0] == 'D' && mc[1] == 'B' && mc[2] == 'R' &&
			        mc[3] == 'A' && mc[4] == 'W' &&	mc[5] == 'O' &&
			        mc[6] == 'P' && mc[7] == 'L')
			{
				uint16_t v1 = READ_L16(mc, 8);
				uint16_t v2 = READ_L16(mc, 10);
				if ((v1 == 2 && v2 == 0) || v2 == 1)
					return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

class RAWDataFormat : public EntryDataFormat
{
public:
	RAWDataFormat() : EntryDataFormat("opl_raw") {};
	~RAWDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 10)
		{
			// Check format
			if (mc[0] == 'R' && mc[1] == 'A' && mc[2] == 'W' &&
			        mc[3] == 'A' && mc[4] == 'D' &&	mc[5] == 'A' &&
			        mc[6] == 'T' && mc[7] == 'A')
			{
				return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

class DoomSoundDataFormat : public EntryDataFormat
{
public:
	DoomSoundDataFormat() : EntryDataFormat("snd_doom") {};
	~DoomSoundDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 8)
		{
			// Check header
			uint16_t head, samplerate;
			uint32_t samples;
			mc.seek(0, SEEK_SET);
			mc.read(&head, 2);
			mc.read(&samplerate, 2);
			mc.read(&samples, 4);

			if (head == 3 && samples <= (mc.getSize() - 8) && samples > 4 && samplerate >= 8000)
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class DoomMacSoundDataFormat : public EntryDataFormat
{
public:
	DoomMacSoundDataFormat() : EntryDataFormat("snd_doom_mac") {};
	~DoomMacSoundDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 8)
		{
			// Check header
			uint16_t head, samplerate;
			uint32_t samples;
			mc.seek(0, SEEK_SET);
			mc.read(&head, 2);
			mc.read(&samplerate, 2);
			mc.read(&samples, 4);

			head = wxUINT16_SWAP_ON_BE(head);
			samples = wxUINT32_SWAP_ON_BE(samples);

			if (head == 0x300 && samples <= (mc.getSize() - 8) && samples > 4 && samplerate >= 8000)
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class JaguarDoomSoundDataFormat : public EntryDataFormat
{
public:
	JaguarDoomSoundDataFormat() : EntryDataFormat("snd_jaguar") {};
	~JaguarDoomSoundDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 28)
		{
			// Check header
			uint32_t samples = READ_B32(mc, 0);
			uint32_t loopstr = READ_B32(mc, 4);
			uint32_t loopend = READ_B32(mc, 8);

			if ((samples == (mc.getSize() - 28) && samples > 4) &&
			        // Normal sounds typically have loopstart = 0, loopend = samples
			        ((loopstr < samples && loopend <= samples && loopstr <= loopend)
			        // Percussion instruments have deliberately invalid loop start/loop end values
			        || (loopstr == 0xFFFFFFFF && (loopend == samples + 1 || loopend == 0))))


				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class DoomPCSpeakerDataFormat : public EntryDataFormat
{
public:
	DoomPCSpeakerDataFormat() : EntryDataFormat("snd_speaker") {};
	~DoomPCSpeakerDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check header: the first two bytes must always be null
			if (mc[0] | mc[1])
				return EDF_FALSE;
			// Next is the number of samples (LE uint16_t), and
			// each sample is a single byte, so the size can be
			// checked easily.
			if (mc.getSize() == 4 + READ_L16(mc, 2))
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};


// WAV format values
// A more complete list can be found in mmreg.h,
// under the "WAVE form wFormatTag IDs" comment.
// There are dozens upon dozens of them, most of
// which are not usually seen in practice.
#define WAVE_FMT_UNK	0x0000
#define WAVE_FMT_PCM	0x0001
#define WAVE_FMT_ADPCM	0x0002
#define WAVE_FMT_ALAW	0x0006
#define WAVE_FMT_MULAW	0x0007
#define WAVE_FMT_MP3	0x0055
#define WAVE_FMT_XTNSBL	0xFFFE

int RiffWavFormat(MemChunk& mc)
{
	// Check size
	size_t size = mc.getSize();
	int format = WAVE_FMT_UNK;
	if (size > 44)
	{
		// Check for wav header
		if (mc[0] != 'R' || mc[1] != 'I' || mc[2] != 'F' || mc[3] != 'F' ||
		        mc[8] != 'W' || mc[9] != 'A' || mc[10] != 'V' || mc[11] != 'E')
			// Not a RIFF-WAV file
			return format;
		// Verify existence of "fmt " and "data" chunks
		size_t fmts = 0, data = 0;
		size_t ncoffs = 12; // next chunk offset
		while (ncoffs + 16 < size)
		{
			if (mc[ncoffs] == 'f' && mc[ncoffs + 1] == 'm' && mc[ncoffs + 2] == 't' && mc[ncoffs + 3] == ' ')
			{
				if (fmts) // already found, there can be only one
					return -1;
				format = READ_L16(mc, (ncoffs+8));
				fmts = ncoffs;
			}
			else if (mc[ncoffs + 0] == 'd' && mc[ncoffs + 1] == 'a' && mc[ncoffs + 2] == 't' && mc[ncoffs + 3] == 'a')
			{
				if (data) // already found, there can be only one
					return -1;
				data = ncoffs;
				// All of them are found, no need to keep looking
				if (fmts)
					break;
			}
			ncoffs += 8 + READ_L32(mc, (ncoffs + 4));
			if (ncoffs % 2)
				ncoffs++;
		}
		if (fmts && data)
			return format;
	}
	return format;
}

class WAVDataFormat : public EntryDataFormat
{
public:
	WAVDataFormat() : EntryDataFormat("snd_wav") {};
	~WAVDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		int fmt = RiffWavFormat(mc);
		if (fmt == WAVE_FMT_UNK || fmt == WAVE_FMT_MP3)
			return EDF_FALSE;
		if (fmt <= WAVE_FMT_MULAW || fmt == WAVE_FMT_XTNSBL)
			return EDF_TRUE;
		return EDF_MAYBE;
	}
};

class OggDataFormat : public EntryDataFormat
{
public:
	OggDataFormat() : EntryDataFormat("snd_ogg") {};
	~OggDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 40)
		{
			// Check for Ogg Vorbis header.
			if (mc[0] == 'O' && mc[1] == 'g' && mc[2] == 'g' && mc[3] == 'S')
			{
				// An Ogg header is 28 byte long. If it's an Ogg Vorbis file, it's followed 
				// by a Vorbis start header. Don't get confused by Ogg FLAC or Ogg Speex.
				if (mc[28] == 1 && mc[29] == 'v' && mc[30] == 'o' && 
					mc[31] == 'r' && mc[32] == 'b' && mc[33] == 'i' && mc[34] == 's')
					return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

class FLACDataFormat : public EntryDataFormat
{
public:
	FLACDataFormat() : EntryDataFormat("snd_flac") {};
	~FLACDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for FLAC header.
			if (mc[0] == 'f' && mc[1] == 'L' && mc[2] == 'a' && mc[3] == 'C')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

// This function was written using the following page as reference:
// http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm
int validMPEG(MemChunk& mc, uint8_t layer, size_t start)
{
	// Check size
	if (mc.getSize() > 4+start)
	{
		// Check for MP3 frame header. Warning, it is a very weak signature.
		uint16_t framesync = ((mc[0+start]<<4) + (mc[1+start]>>4)) & 0xFFE;
		// Check for presence of the sync word (the first eleven bits, all set)
		if (framesync == 0xFFE)
		{
			uint8_t version = (mc[1+start]>>3) & 3;
			uint8_t mylayer = (mc[1+start]>>1) & 3;
			// Version: 0 MPEG v2.5 (unofficial), 1 invalid, 2 MPEG v2, 3 MPEG v3
			// Layer: 0 invalid, 1 III, 2 II, 3 I (this sure makes sense :p)
			if (version != 1 && mylayer == (4 - layer))
			{
				// The bitrate index has values that depend on version and layer,
				// but 1111b is invalid across the board. Same for sample rate,
				// 11b is invalid. Finally, an emphasis setting of 10b is bad, too.
				uint8_t rates = (mc[2+start]>>2);
				uint8_t emphasis = mc[3+start] & 3;
				if (rates != 0x3F && emphasis != 2)
				{
					// More checks could be done here, notably to compute frame length
					// and check that it corresponds to either another frame or EOF...
					return EDF_MAYBE;
				}
			}
		}
	}
	return EDF_FALSE;
}

class MP2DataFormat : public EntryDataFormat
{
public:
	MP2DataFormat() : EntryDataFormat("snd_mp2") {};
	~MP2DataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		return validMPEG(mc, 2, checkForTags(mc));
	}
};

class MP3DataFormat : public EntryDataFormat
{
public:
	MP3DataFormat() : EntryDataFormat("snd_mp3") {};
	~MP3DataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// MP3 data might be contained in RIFF-WAV files.
		// Officially, they are legit .WAV files, just using MP3 instead of PCM.
		// In practice, a simple PCM WAV player will abort, while MP3 players will
		// usually work; so it's probably better to identify them as MP3.
		int rwfmt = RiffWavFormat(mc);
		if (rwfmt == WAVE_FMT_MP3)
			return EDF_TRUE;

		return validMPEG(mc, 3, checkForTags(mc));
	}
};

class VocDataFormat : public EntryDataFormat
{
public:
	VocDataFormat() : EntryDataFormat("snd_voc") {};
	~VocDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 26)
		{
			// Check for header, see specs at http://wiki.multimedia.cx/index.php?title=Creative_Voice
			if (mc[19] == 26 && mc [20] == 26 && mc[21] == 0)
			{
				uint16_t version = READ_L16(mc, 22);
				uint16_t validity = ~version + 0x1234;
				if ((mc[24] + (mc[25] << 8)) == validity)
				{
					// Lastly, check for header text
					string header(wxString::FromAscii(mc.getData(), 19));
					if (header == "Creative Voice File")
						return EDF_TRUE;
				}
			}
		}
		return EDF_FALSE;
	}
};

class WolfSoundDataFormat : public EntryDataFormat
{
public:
	WolfSoundDataFormat() : EntryDataFormat("snd_wolf") {};
	~WolfSoundDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		return (mc.getSize() > 0 ? EDF_MAYBE : EDF_FALSE);
	}
};

class AudioTPCSoundDataFormat : public EntryDataFormat
{
public:
	AudioTPCSoundDataFormat() : EntryDataFormat("snd_audiot") {};
	~AudioTPCSoundDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size > 8)
		{
			size_t nsamples = READ_L32(mc, 0);
			if (size < (nsamples + 9)
				&& size < 1024
				&& size > (nsamples + 6)
				&& mc[nsamples+6] == 0)
				return EDF_TRUE;
			// Hack #1: last PC sound in Wolf3D/Spear carries a Muse end marker
			else if (size == (nsamples + 11)
				&& (mc[nsamples+7] == '!')
				&& (mc[nsamples+8] == 'I')
				&& (mc[nsamples+9] == 'D')
				&& (mc[nsamples+10] =='!'))
				return EDF_TRUE;
			// Hack #2: Rise of the Triad's PCSP53
			else if (size == 150 && nsamples == 142 && 
				mc[147] == 156 && mc[148] == 157 && mc[149] == 97)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class AudioTAdlibSoundDataFormat : public EntryDataFormat
{
public:
	AudioTAdlibSoundDataFormat() : EntryDataFormat("opl_audiot") {};
	~AudioTAdlibSoundDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size > 24 && size < 1024)
		{
			// Octave block value must be less than 8, sustain shouldn't be null
			if (mc[22] > 7 || (mc[12]|mc[13])==0)
				return EDF_FALSE;
			size_t nsamples = READ_L32(mc, 0);
			if (size >= (nsamples + 24) && (mc[size-1] == 0))
				return EDF_TRUE;
			// Hack #1: last Adlib sound in Wolf3D/Spear carries a Muse end marker
			else if (size >= nsamples + 28 &&
				mc[size-1] == '!' && mc[size-2] == 'D' && 
				mc[size-3] == 'I' && mc[size-4] == '!')
				return EDF_TRUE;
			// Hack #2: Rise of the Triad's ADLB53
			else if (size == 166 && nsamples == 142 &&
				mc[163] == 7 && mc[164] == 7 && mc[165] == 6)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

// Blood SFX+RAW format
class BloodSFXDataFormat : public EntryDataFormat
{
public:
	BloodSFXDataFormat() : EntryDataFormat("snd_bloodsfx") {};
	~BloodSFXDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size, must be between 22 and 29 included
		if (mc.getSize() > 21 && mc.getSize() < 30)
		{
			if (mc[12] == 1 || mc[12] == 5)
			{
				size_t i = 20;
				for (; i < mc.getSize() - 1; ++i)
				{
					// Check that the entry does give a purely alphanumeric ASCII name
					if ((mc[i] < '0' || (mc[i] > '9' && mc[i] < 'A') ||
					        (mc[i] > 'Z' && mc[i] < 'a') || mc[i] > 'z') && mc[i] != '_')
						return EDF_FALSE;
				}
				// And check that it null-terminates
				if (mc[i] == 0)
					return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

class SunSoundDataFormat : public EntryDataFormat
{
public:
	SunSoundDataFormat() : EntryDataFormat("snd_sun") {};
	~SunSoundDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 32)
		{
			// Check for signature
			if (mc[0] != '.' || mc[1] != 's' || mc[2] != 'n' || mc[3] != 'd')
				return EDF_FALSE;
			size_t offset = READ_B32(mc, 4);
			size_t datasize = READ_B32(mc, 8);
			if (offset < 24 || offset + datasize > mc.getSize())
				return EDF_FALSE;
			size_t format = READ_B32(mc, 12);
			if (format < 2 || format > 7)
				return EDF_FALSE;
			size_t samplerate = READ_B32(mc, 16);
			if (samplerate < 8000 || samplerate > 96000)
				return EDF_FALSE;
			size_t channels = READ_B32(mc, 20);
			if (channels == 0 || channels > 2)
				return EDF_FALSE;
			return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

CVAR(Bool, debugaiff, false, 0)
class AIFFSoundDataFormat : public EntryDataFormat
{
public:
	AIFFSoundDataFormat() : EntryDataFormat("snd_aiff") {};
	~AIFFSoundDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 50)
		{
			// An AIFF file should start with a FORM-AIFF groupID, then contain a COMM and an SSND chunks.
			// GroupID size 12. COMM size 26. SSND size 12 + samples. So minimum size is greater than 50.
			// Chunk sizes are in big-endian notation.
			if (mc[0] == 'F' && mc[1] == 'O' && mc[2] == 'R' && mc[3] == 'M' &&
				mc[8] == 'A' && mc[9] == 'I' && mc[10] == 'F' && (mc[11] == 'F' || mc[11] == 'C'))
			{
				size_t size = READ_B32(mc, 4) + 8;
				if (debugaiff) LOG_MESSAGE(1, "size %d", size);
				if (size > mc.getSize())
				{
					if (debugaiff) LOG_MESSAGE(1, "%d <= %d fails", size, mc.getSize());
					return EDF_FALSE;
				}
				size_t s = 12;
				bool comm_found = false;
				bool ssnd_found = false;
				while (s < size && !(comm_found && ssnd_found))
				{
					if (debugaiff) LOG_MESSAGE(1, "%d/%d", s, size);
					if (mc[s+0] == 'C' && mc[s+1] == 'O' && mc[s+2] == 'M' && mc[s+3] == 'M')
						comm_found = true;
					else if (mc[s+0] == 'S' && mc[s+1] == 'S' && mc[s+2] == 'N' && mc[s+3] == 'D')
						ssnd_found = true;
					s += 8 + READ_B32(mc, s+4);
					if (s%2) ++s;
					if (debugaiff) LOG_MESSAGE(1, "looking now at offset %d", s);
				}
				if (comm_found && ssnd_found)
					return EDF_TRUE;
				if (debugaiff) LOG_MESSAGE(1, "COMM was %sfound and SSND was %sfound", comm_found ? "" : "not ", ssnd_found ? "" : "not ");
			}
		}
		return EDF_FALSE;
	}
};

class AYDataFormat : public EntryDataFormat
{
public:
	AYDataFormat() : EntryDataFormat("gme_ay") {};
	~AYDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 20)
		{
			// Check for header text using official signature string
			if (memcmp(mc.getData(), "ZXAYEMUL", 8) == 0)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class GBSDataFormat : public EntryDataFormat
{
public:
	GBSDataFormat() : EntryDataFormat("gme_gbs") {};
	~GBSDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 112)
		{
			// Talk about a weak signature...
			if (memcmp(mc.getData(), "GBS\x01", 4) == 0)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class GYMDataFormat : public EntryDataFormat
{
public:
	GYMDataFormat() : EntryDataFormat("gme_gym") {};
	~GYMDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 428)
		{
			// Talk about a weak signature... And some GYM files don't even have that...
			if (memcmp(mc.getData(), "GYMX", 4) == 0)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class HESDataFormat : public EntryDataFormat
{
public:
	HESDataFormat() : EntryDataFormat("gme_hes") {};
	~HESDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 32)
		{
			// Another weak signature
			if (memcmp(mc.getData(), "HESM", 4) == 0)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class KSSDataFormat : public EntryDataFormat
{
public:
	KSSDataFormat() : EntryDataFormat("gme_kss") {};
	~KSSDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 16)
		{
			// Weak signatures for the weak signature god!
			// Unreliable identifications for his throne!
			if (memcmp(mc.getData(), "KSCC", 4) == 0 ||
				memcmp(mc.getData(), "KSSX", 4) == 0)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class NSFDataFormat : public EntryDataFormat
{
public:
	NSFDataFormat() : EntryDataFormat("gme_nsf") {};
	~NSFDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 128)
		{
			// Check for header text using official signature string
			if (memcmp(mc.getData(), "NESM\x1A", 5) == 0)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class NSFEDataFormat : public EntryDataFormat
{
public:
	NSFEDataFormat() : EntryDataFormat("gme_nsfe") {};
	~NSFEDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 5)
		{
			// Check for header text using official signature string
			if (memcmp(mc.getData(), "NESM\x1A", 5) == 0)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class SAPDataFormat : public EntryDataFormat
{
public:
	SAPDataFormat() : EntryDataFormat("gme_sap") {};
	~SAPDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 16)
		{
			// Check for header text using official signature string
			if (memcmp(mc.getData(), "SAP\x0D\x0A", 5) == 0)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

// SNES SPC format, supported by ZDoom and Eternity
class SPCDataFormat : public EntryDataFormat
{
public:
	SPCDataFormat() : EntryDataFormat("gme_spc") {};
	~SPCDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 256)
		{
			// Check for header text using official signature string
			if (memcmp(mc.getData(), "SNES-SPC700 Sound File Data", 27) == 0)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class VGMDataFormat : public EntryDataFormat
{
public:
	VGMDataFormat() : EntryDataFormat("gme_vgm") {};
	~VGMDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 64)
		{
			// Check for header text (kind of a weak test)
			if (memcmp(mc.getData(), "Vgm ", 4) == 0)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

#define GZIP_SIGNATURE 0x1F8B0800
#include "Utility/Compression.h"
class VGZDataFormat : public EntryDataFormat
{
public:
	VGZDataFormat() : EntryDataFormat("gme_vgz") {};
	~VGZDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 64)
		{
			// Check for GZip header first
			if (READ_B32(mc, 0) == GZIP_SIGNATURE)
			{
				// Extract, then check for vgm signature
				MemChunk tmp;
				if (Compression::GZipInflate(mc, tmp) &&
					tmp.getSize() > 64 && memcmp(tmp.getData(), "Vgm ", 4) == 0)
					return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};
#endif //AUDIOFORMATS_H
