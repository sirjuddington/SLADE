#ifndef MISCFORMATS_H
#define MISCFORMATS_H

class RLE0DataFormat : public EntryDataFormat
{
public:
	RLE0DataFormat() : EntryDataFormat("misc_rle0") {};
	~RLE0DataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 6)
		{
			// Check for RLE0 header
			if (mc[0] == 'R' && mc[1] == 'L' && mc[2] == 'E' && mc[3] == '0')
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

#endif //MISCFORMATS_H
