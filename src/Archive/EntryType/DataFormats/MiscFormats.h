#pragma once

class RLE0DataFormat : public EntryDataFormat
{
public:
	RLE0DataFormat() : EntryDataFormat("misc_rle0") {}
	~RLE0DataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 6)
		{
			// Check for RLE0 header
			if (mc[0] == 'R' && mc[1] == 'L' && mc[2] == 'E' && mc[3] == '0')
				return MATCH_TRUE;
		}

		return MATCH_FALSE;
	}
};
