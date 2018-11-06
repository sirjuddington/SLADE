#ifndef LUMPFORMATS_H
#define LUMPFORMATS_H

class TextureXDataFormat : public EntryDataFormat
{
public:
	TextureXDataFormat() : EntryDataFormat("texturex") {};
	~TextureXDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() < 4)
			return EDF_FALSE;

		// Not the best test in the world. But a text-based texture lump ought
		// to fail it every time; as it would be interpreted as too high a number.
		uint32_t ntex = READ_L32(mc, 0);
		if ((int32_t) ntex < 0)
			return EDF_FALSE;
		if (mc.getSize() < (ntex * 24))
			return EDF_FALSE;
		return EDF_TRUE;
	}
};

class PNamesDataFormat : public EntryDataFormat
{
public:
	PNamesDataFormat() : EntryDataFormat("pnames") {};
	~PNamesDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// It's a pretty simple format alright
		uint32_t number = READ_L32(mc, 0);
		if ((int32_t) number < 0)
			return EDF_FALSE;
		if (mc.getSize() != (4 + number * 8))
			return EDF_FALSE;
		return EDF_TRUE;
	}
};

class BoomAnimatedDataFormat : public EntryDataFormat
{
public:
	BoomAnimatedDataFormat() : EntryDataFormat("animated") {};
	~BoomAnimatedDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		if (mc.getSize() > sizeof(animated_t))
		{
			size_t numentries = mc.getSize()/sizeof(animated_t);
			// The last entry can be incomplete, as it may stop right
			// after the declaration of its type. So if the size is not
			// a perfect multiple, then the last entry is incomplete.
			size_t lastentry = ((mc.getSize()%numentries) ? numentries : numentries - 1);

			// Check that the last entry ends on an ANIM_STOP type
			if (mc[lastentry*sizeof(animated_t)] == ANIM_STOP)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class BoomSwitchesDataFormat : public EntryDataFormat
{
public:
	BoomSwitchesDataFormat() : EntryDataFormat("switches") {};
	~BoomSwitchesDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		if (mc.getSize() > sizeof(switches_t))
		{
			size_t numentries = mc.getSize()/sizeof(switches_t);

			// Check that the last entry ends on a SWCH_STOP type
			if (READ_L16(mc, (numentries*sizeof(switches_t) -2)) == SWCH_STOP)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class ZNodesDataFormat : public EntryDataFormat
{
public:
	ZNodesDataFormat() : EntryDataFormat("znod") {};
	~ZNodesDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for ZNOD header
			if (mc[0] == 'Z' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == 'N')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class ZGLNodesDataFormat : public EntryDataFormat
{
public:
	ZGLNodesDataFormat() : EntryDataFormat("zgln") {};
	~ZGLNodesDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for ZGLN header
			if (mc[0] == 'Z' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == 'N')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class ZGLNodes2DataFormat : public EntryDataFormat
{
public:
	ZGLNodes2DataFormat() : EntryDataFormat("zgl2") {};
	~ZGLNodes2DataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for ZGL2 header
			if (mc[0] == 'Z' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == '2')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class XNodesDataFormat : public EntryDataFormat
{
public:
	XNodesDataFormat() : EntryDataFormat("xnod") {};
	~XNodesDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for XNOD header
			if (mc[0] == 'X' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == 'N')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class XGLNodesDataFormat : public EntryDataFormat
{
public:
	XGLNodesDataFormat() : EntryDataFormat("xgln") {};
	~XGLNodesDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for XGLN header
			if (mc[0] == 'X' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == 'N')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class XGLNodes2DataFormat : public EntryDataFormat
{
public:
	XGLNodes2DataFormat() : EntryDataFormat("xgl2") {};
	~XGLNodes2DataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for XGL2 header
			if (mc[0] == 'X' && mc[1] == 'G' && mc[2] == 'L' && mc[3] == '2')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class ACS0DataFormat : public EntryDataFormat
{
public:
	ACS0DataFormat() : EntryDataFormat("acs0") {};
	~ACS0DataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 15)
		{
			// Check for ACS header
			if (mc[0] == 'A' && mc[1] == 'C' && mc[2] == 'S' && mc[3] == 0)
			{
				uint32_t diroffs = READ_L32(mc, 4);
				if (diroffs > mc.getSize())
					return EDF_FALSE;
				else if (mc[diroffs - 4] == 'A' && mc[diroffs - 3] == 'C' &&
				         mc[diroffs - 2] == 'S' && mc[diroffs - 1] != 0)
				{
					return EDF_FALSE;
				}
				return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

class ACSeDataFormat : public EntryDataFormat
{
public:
	ACSeDataFormat() : EntryDataFormat("acsl") {};
	~ACSeDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 32)
		{
			// Check for ACS header
			if (mc[0] == 'A' && mc[1] == 'C' && mc[2] == 'S')
			{
				if (mc[3] == 'e')
				{
					return EDF_TRUE;
				}
				else if (mc[3] == 0)
				{
					uint32_t diroffs = READ_L32(mc, 4);
					if (diroffs > mc.getSize())
						return EDF_FALSE;
					else if (mc[diroffs - 4] == 'A' && mc[diroffs - 3] == 'C' &&
					         mc[diroffs - 2] == 'S' && mc[diroffs - 1] == 'e')
					{
						return EDF_TRUE;
					}
					return EDF_FALSE;
				}
			}
		}
		return EDF_FALSE;
	}
};

class ACSEDataFormat : public EntryDataFormat
{
public:
	ACSEDataFormat() : EntryDataFormat("acse") {};
	~ACSEDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 32)
		{
			// Check for ACS header
			if (mc[0] == 'A' && mc[1] == 'C' && mc[2] == 'S')
			{
				if (mc[3] == 'E')
				{
					return EDF_TRUE;
				}
				else if (mc[3] == 0)
				{
					uint32_t diroffs = READ_L32(mc, 4);
					if (diroffs > mc.getSize())
						return EDF_FALSE;
					else if (mc[diroffs - 4] == 'A' && mc[diroffs - 3] == 'C' &&
					         mc[diroffs - 2] == 'S' && mc[diroffs - 1] == 'E')
					{
						return EDF_TRUE;
					}
					return EDF_FALSE;
				}
			}
		}
		return EDF_FALSE;
	}
};

#endif //LUMPFORMATS_H
