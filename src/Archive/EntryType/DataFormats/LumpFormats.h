#pragma once

class TextureXDataFormat : public EntryDataFormat
{
public:
	TextureXDataFormat() : EntryDataFormat("texturex"){};
	~TextureXDataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// Check size
		if (mc.size() < 4)
			return MATCH_FALSE;

		// Not the best test in the world. But a text-based texture lump ought
		// to fail it every time; as it would be interpreted as too high a number.
		uint32_t ntex = mc.readL32(0);
		if ((int32_t)ntex < 0)
			return MATCH_FALSE;
		if (mc.size() < (ntex * 24))
			return MATCH_FALSE;
		return MATCH_TRUE;
	}
};

class PNamesDataFormat : public EntryDataFormat
{
public:
	PNamesDataFormat() : EntryDataFormat("pnames"){};
	~PNamesDataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// It's a pretty simple format alright
		uint32_t number = mc.readL32(0);
		if ((int32_t)number < 0)
			return MATCH_FALSE;
		if (mc.size() != (4 + number * 8))
			return MATCH_FALSE;
		return MATCH_TRUE;
	}
};

class BoomAnimatedDataFormat : public EntryDataFormat
{
public:
	BoomAnimatedDataFormat() : EntryDataFormat("animated"){};
	~BoomAnimatedDataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		if (mc.size() > sizeof(AnimatedEntry))
		{
			size_t numentries = mc.size() / sizeof(AnimatedEntry);
			// The last entry can be incomplete, as it may stop right
			// after the declaration of its type. So if the size is not
			// a perfect multiple, then the last entry is incomplete.
			size_t lastentry = ((mc.size() % numentries) ? numentries : numentries - 1);

			// Check that the last entry ends on an ANIM_STOP type
			if (mc[lastentry * sizeof(AnimatedEntry)] == AnimTypes::STOP)
				return MATCH_TRUE;
		}
		return MATCH_FALSE;
	}
};

class BoomSwitchesDataFormat : public EntryDataFormat
{
public:
	BoomSwitchesDataFormat() : EntryDataFormat("switches"){};
	~BoomSwitchesDataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		if (mc.size() > sizeof(SwitchesEntry))
		{
			size_t numentries = mc.size() / sizeof(SwitchesEntry);

			// Check that the last entry ends on a SWCH_STOP type
			if (mc.readL16((numentries * sizeof(SwitchesEntry) - 2)) == SwitchTypes::STOP)
				return MATCH_TRUE;
		}
		return MATCH_FALSE;
	}
};

class ZNodesDataFormat : public EntryDataFormat
{
public:
	ZNodesDataFormat() : EntryDataFormat("znod"){};
	~ZNodesDataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 4)
		{
			// Check for ZNOD header
			if (mc[0] == 'Z' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == 'N')
				return MATCH_TRUE;
		}
		return MATCH_FALSE;
	}
};

class ZGLNodesDataFormat : public EntryDataFormat
{
public:
	ZGLNodesDataFormat() : EntryDataFormat("zgln"){};
	~ZGLNodesDataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 4)
		{
			// Check for ZGLN header
			if (mc[0] == 'Z' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == 'N')
				return MATCH_TRUE;
		}
		return MATCH_FALSE;
	}
};

class ZGLNodes2DataFormat : public EntryDataFormat
{
public:
	ZGLNodes2DataFormat() : EntryDataFormat("zgl2"){};
	~ZGLNodes2DataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 4)
		{
			// Check for ZGL2 header
			if (mc[0] == 'Z' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == '2')
				return MATCH_TRUE;
		}
		return MATCH_FALSE;
	}
};

class XNodesDataFormat : public EntryDataFormat
{
public:
	XNodesDataFormat() : EntryDataFormat("xnod"){};
	~XNodesDataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 4)
		{
			// Check for XNOD header
			if (mc[0] == 'X' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == 'N')
				return MATCH_TRUE;
		}
		return MATCH_FALSE;
	}
};

class XGLNodesDataFormat : public EntryDataFormat
{
public:
	XGLNodesDataFormat() : EntryDataFormat("xgln"){};
	~XGLNodesDataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 4)
		{
			// Check for XGLN header
			if (mc[0] == 'X' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == 'N')
				return MATCH_TRUE;
		}
		return MATCH_FALSE;
	}
};

class XGLNodes2DataFormat : public EntryDataFormat
{
public:
	XGLNodes2DataFormat() : EntryDataFormat("xgl2"){};
	~XGLNodes2DataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 4)
		{
			// Check for XGL2 header
			if (mc[0] == 'X' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == '2')
				return MATCH_TRUE;
		}
		return MATCH_FALSE;
	}
};

class ACS0DataFormat : public EntryDataFormat
{
public:
	ACS0DataFormat() : EntryDataFormat("acs0"){};
	~ACS0DataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 15)
		{
			// Check for ACS header
			if (mc[0] == 'A' && mc[1] == 'C' && mc[2] == 'S' && mc[3] == 0)
			{
				uint32_t diroffs = mc.readL32(4);
				if (diroffs > mc.size())
					return MATCH_FALSE;
				else if (
					mc[diroffs - 4] == 'A' && mc[diroffs - 3] == 'C' && mc[diroffs - 2] == 'S' && mc[diroffs - 1] != 0)
				{
					return MATCH_FALSE;
				}
				return MATCH_TRUE;
			}
		}
		return MATCH_FALSE;
	}
};

class ACSeDataFormat : public EntryDataFormat
{
public:
	ACSeDataFormat() : EntryDataFormat("acsl"){};
	~ACSeDataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 32)
		{
			// Check for ACS header
			if (mc[0] == 'A' && mc[1] == 'C' && mc[2] == 'S')
			{
				if (mc[3] == 'e')
				{
					return MATCH_TRUE;
				}
				else if (mc[3] == 0)
				{
					uint32_t diroffs = mc.readL32(4);
					if (diroffs > mc.size())
						return MATCH_FALSE;
					else if (
						mc[diroffs - 4] == 'A' && mc[diroffs - 3] == 'C' && mc[diroffs - 2] == 'S'
						&& mc[diroffs - 1] == 'e')
					{
						return MATCH_TRUE;
					}
					return MATCH_FALSE;
				}
			}
		}
		return MATCH_FALSE;
	}
};

class ACSEDataFormat : public EntryDataFormat
{
public:
	ACSEDataFormat() : EntryDataFormat("acse"){};
	~ACSEDataFormat() = default;

	int isThisFormat(MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 32)
		{
			// Check for ACS header
			if (mc[0] == 'A' && mc[1] == 'C' && mc[2] == 'S')
			{
				if (mc[3] == 'E')
				{
					return MATCH_TRUE;
				}
				else if (mc[3] == 0)
				{
					uint32_t diroffs = mc.readL32(4);
					if (diroffs > mc.size())
						return MATCH_FALSE;
					else if (
						mc[diroffs - 4] == 'A' && mc[diroffs - 3] == 'C' && mc[diroffs - 2] == 'S'
						&& mc[diroffs - 1] == 'E')
					{
						return MATCH_TRUE;
					}
					return MATCH_FALSE;
				}
			}
		}
		return MATCH_FALSE;
	}
};
