#ifndef MODELFORMATS_H
#define MODELFORMATS_H

class DMDModelDataFormat : public EntryDataFormat
{
public:
	DMDModelDataFormat() : EntryDataFormat("mesh_dmd") {};
	~DMDModelDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for DMDM header
			if (mc[0] == 'D' && mc[1] == 'M' && mc[2] == 'D' && mc[3] == 'M')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class MDLModelDataFormat : public EntryDataFormat
{
public:
	MDLModelDataFormat() : EntryDataFormat("mesh_mdl") {};
	~MDLModelDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for IDPO header
			if (mc[0] == 'I' && mc[1] == 'D' && mc[2] == 'P' && mc[3] == 'O')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class MD2ModelDataFormat : public EntryDataFormat
{
public:
	MD2ModelDataFormat() : EntryDataFormat("mesh_md2") {};
	~MD2ModelDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for IDP2 header
			if (mc[0] == 'I' && mc[1] == 'D' && mc[2] == 'P' && mc[3] == '2')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class MD3ModelDataFormat : public EntryDataFormat
{
public:
	MD3ModelDataFormat() : EntryDataFormat("mesh_md3") {};
	~MD3ModelDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for IDP3 header
			if (mc[0] == 'I' && mc[1] == 'D' && mc[2] == 'P' && mc[3] == '3')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class VOXVoxelDataFormat : public EntryDataFormat
{
public:
	VOXVoxelDataFormat() : EntryDataFormat("voxel_vox") {};
	~VOXVoxelDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size: 12 bytes for dimensions and 768 for palette,
		// so 780 bytes for an empty voxel object.
		if (mc.getSize() > 780)
		{
			uint32_t x, y, z;
			mc.seek(0, SEEK_SET);
			mc.read(&x, 4); x = wxINT32_SWAP_ON_BE(x);
			mc.read(&y, 4); y = wxINT32_SWAP_ON_BE(y);
			mc.read(&z, 4); z = wxINT32_SWAP_ON_BE(z);
			if (mc.getSize() == 780 + (x * y * z))
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class KVXVoxelDataFormat : public EntryDataFormat
{
public:
	KVXVoxelDataFormat() : EntryDataFormat("voxel_kvx") {};
	~KVXVoxelDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size: 28 bytes for dimensions and pivot,
		// 4 minimum for offset info, and 768 for palette,
		// so 800 bytes for an empty voxel object.
		if (mc.getSize() > 800)
		{
			// We'll need a certain number of variables
			uint32_t szd, szx, szy, szz;
			int32_t szofx, szofxy, szvxd;
			uint32_t dummy;
			size_t endofvox, parsed;
			// Take palette info into account
			endofvox = mc.getSize() - 768;
			parsed = 0;
			mc.seek(0, SEEK_SET);

			// Start validation loop
			for (int miplevel = 0; miplevel < 5; miplevel++)
			{
				mc.read(&szd, 4); szd = wxINT32_SWAP_ON_BE(szd); parsed += 4;
				// Check that data doesn't run out of bounds
				if (parsed + szd > endofvox)
					return EDF_FALSE;
				mc.read(&szx, 4); szx = wxINT32_SWAP_ON_BE(szx);
				mc.read(&szy, 4); szy = wxINT32_SWAP_ON_BE(szy);
				mc.read(&szz, 4); szz = wxINT32_SWAP_ON_BE(szz);
				// Compute size of the different data segments to do some checks
				szofx = (szx+1)<<2; szofxy = szx*((szy+1)<<1);
				szvxd = szd - (szofx + szofxy);
				if (szvxd < 0)
					return EDF_FALSE;
				// Those are the coordinates of the pivot point.
				// We don't care about it for this test.
				mc.read(&dummy, 4); mc.read(&dummy, 4); mc.read(&dummy, 4);
				// X offsets of the voxel. The first can be used for a check.
				mc.read(&dummy, 4); dummy = wxINT32_SWAP_ON_BE(dummy);
				if (dummy != ((szx+1)*4 + 2*szx*(szy+1)))
					return EDF_FALSE;

				// Update the parse count
				parsed += szd;
				mc.seek(parsed, SEEK_SET);

				// We're at the end of a mip level,
				// have we reached the palette yet?
				if (parsed == endofvox)
					return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

#endif //MODELFORMATS_H
