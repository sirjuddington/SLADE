/* Extended Module Player
 * Copyright (C) 1996-2021 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * OctaMED v1.00b: ftp://ftp.funet.fi/pub/amiga/fish/501-600/ff579
 */

#include "med.h"
#include "loader.h"
#include "../med_extras.h"

static int mmd1_test(HIO_HANDLE *, char *, const int);
static int mmd1_load (struct module_data *, HIO_HANDLE *, const int);

const struct format_loader libxmp_loader_mmd1 = {
	"MED 2.10/OctaMED",
	mmd1_test,
	mmd1_load
};

static int mmd1_test(HIO_HANDLE *f, char *t, const int start)
{
	char id[4];
	uint32 offset, len;

	if (hio_read(id, 1, 4, f) < 4)
		return -1;

	if (memcmp(id, "MMD0", 4) && memcmp(id, "MMD1", 4))
		return -1;

	hio_seek(f, 28, SEEK_CUR);
	offset = hio_read32b(f);		/* expdata_offset */

	if (offset) {
		hio_seek(f, start + offset + 44, SEEK_SET);
		offset = hio_read32b(f);
		len = hio_read32b(f);
		hio_seek(f, start + offset, SEEK_SET);
		libxmp_read_title(f, t, len);
	} else {
		libxmp_read_title(f, t, 0);
	}

	return 0;
}

/* Number of octaves in IFFOCT samples */
static const int num_oct[6] = { 5, 3, 2, 4, 6, 7 };

static int mmd1_load(struct module_data *m, HIO_HANDLE *f, const int start)
{
	struct xmp_module *mod = &m->mod;
	int i, j, k;
	struct MMD0 header;
	struct MMD0song song;
	struct MMD1Block block;
	struct InstrHdr instr;
	struct SynthInstr synth;
	struct InstrExt exp_smp;
	struct MMD0exp expdata;
	struct xmp_event *event;
	int ver = 0;
	int smp_idx = 0;
	uint8 e[4];
	int song_offset;
	int blockarr_offset;
	int smplarr_offset;
	int expdata_offset;
	int expsmp_offset;
	int songname_offset;
	int iinfo_offset;
	int annotxt_offset;
	int pos;
	int bpm_on, bpmlen, med_8ch, hexvol;
	char name[40];

	LOAD_INIT();

	hio_read(&header.id, 4, 1, f);

	ver = *((char *)&header.id + 3) - '1' + 1;

	D_(D_WARN "load header");
	header.modlen = hio_read32b(f);
	song_offset = hio_read32b(f);
	D_(D_INFO "song_offset = 0x%08x", song_offset);
	hio_read16b(f);
	hio_read16b(f);
	blockarr_offset = hio_read32b(f);
	D_(D_INFO "blockarr_offset = 0x%08x", blockarr_offset);
	hio_read32b(f);
	smplarr_offset = hio_read32b(f);
	D_(D_INFO "smplarr_offset = 0x%08x", smplarr_offset);
	hio_read32b(f);
	expdata_offset = hio_read32b(f);
	D_(D_INFO "expdata_offset = 0x%08x", expdata_offset);
	hio_read32b(f);
	header.pstate = hio_read16b(f);
	header.pblock = hio_read16b(f);
	header.pline = hio_read16b(f);
	header.pseqnum = hio_read16b(f);
	header.actplayline = hio_read16b(f);
	header.counter = hio_read8(f);
	header.extra_songs = hio_read8(f);

	/*
	 * song structure
	 */
	D_(D_WARN "load song");
	if (hio_seek(f, start + song_offset, SEEK_SET) != 0)
	  return -1;
	for (i = 0; i < 63; i++) {
		song.sample[i].rep = hio_read16b(f);
		song.sample[i].replen = hio_read16b(f);
		song.sample[i].midich = hio_read8(f);
		song.sample[i].midipreset = hio_read8(f);
		song.sample[i].svol = hio_read8(f);
		song.sample[i].strans = hio_read8s(f);
	}
	song.numblocks = hio_read16b(f);
	song.songlen = hio_read16b(f);

	/* Sanity check */
	if (song.numblocks > 255 || song.songlen > 256) {
		return -1;
	}

	D_(D_INFO "song.songlen = %d", song.songlen);
	for (i = 0; i < 256; i++)
		song.playseq[i] = hio_read8(f);
	song.deftempo = hio_read16b(f);
	song.playtransp = hio_read8(f);
	song.flags = hio_read8(f);
	song.flags2 = hio_read8(f);
	song.tempo2 = hio_read8(f);
	for (i = 0; i < 16; i++)
		song.trkvol[i] = hio_read8(f);
	song.mastervol = hio_read8(f);
	song.numsamples = hio_read8(f);

	/* Sanity check */
	if (song.numsamples > 63) {
		return -1;
	}

	/*
	 * convert header
	 */
	m->c4rate = C4_NTSC_RATE;
	m->quirk |= song.flags & FLAG_STSLIDE ? 0 : QUIRK_VSALL | QUIRK_PBALL;
	hexvol = song.flags & FLAG_VOLHEX;
	med_8ch = song.flags & FLAG_8CHANNEL;
	bpm_on = song.flags2 & FLAG2_BPM;
	bpmlen = 1 + (song.flags2 & FLAG2_BMASK);
	m->time_factor = MED_TIME_FACTOR;

	mmd_set_bpm(m, med_8ch, song.deftempo, bpm_on, bpmlen);

        mod->spd = song.tempo2;
	mod->pat = song.numblocks;
	mod->ins = song.numsamples;
	mod->len = song.songlen;
	mod->rst = 0;
	mod->chn = 0;
	memcpy(mod->xxo, song.playseq, mod->len);
	mod->name[0] = 0;

	/*
	 * Obtain number of samples from each instrument
	 */
	mod->smp = 0;
	for (i = 0; i < mod->ins; i++) {
		uint32 smpl_offset;
		int16 type;
		if (hio_seek(f, start + smplarr_offset + i * 4, SEEK_SET) != 0)
		  return -1;
		smpl_offset = hio_read32b(f);
		if (smpl_offset == 0)
			continue;
		if (hio_seek(f, start + smpl_offset, SEEK_SET) != 0)
		  return -1;
		hio_read32b(f);				/* length */
		type = hio_read16b(f);
		if (type == -1) {			/* type is synth? */
			int wforms;
			hio_seek(f, 14, SEEK_CUR);
			wforms = hio_read16b(f);

			/* Sanity check */
			if (wforms > 256)
				return -1;

			mod->smp += wforms;
		} else if (type >= 1 && type <= 6) {
			mod->smp += num_oct[type - 1];
		} else {
			mod->smp++;
		}
	}

	/*
	 * expdata
	 */
	D_(D_WARN "load expdata");
	expdata.s_ext_entries = 0;
	expdata.s_ext_entrsz = 0;
	expdata.i_ext_entries = 0;
	expdata.i_ext_entrsz = 0;
	expsmp_offset = 0;
	iinfo_offset = 0;
	if (expdata_offset) {
		if (hio_seek(f, start + expdata_offset, SEEK_SET) != 0)
		  return -1;
		hio_read32b(f);
		expsmp_offset = hio_read32b(f);
		D_(D_INFO "expsmp_offset = 0x%08x", expsmp_offset);
		expdata.s_ext_entries = hio_read16b(f);
		expdata.s_ext_entrsz = hio_read16b(f);
		annotxt_offset = hio_read32b(f);
		expdata.annolen = hio_read32b(f);
		iinfo_offset = hio_read32b(f);
		D_(D_INFO "iinfo_offset = 0x%08x", iinfo_offset);
		expdata.i_ext_entries = hio_read16b(f);
		expdata.i_ext_entrsz = hio_read16b(f);

		/* Sanity check */
		if (expsmp_offset < 0 ||
		    annotxt_offset < 0 ||
                    expdata.annolen > 0x10000 ||
		    iinfo_offset < 0) {
			return -1;
		}

		hio_read32b(f);
		hio_read32b(f);
		hio_read32b(f);
		hio_read32b(f);
		songname_offset = hio_read32b(f);
		expdata.songnamelen = hio_read32b(f);
		D_(D_INFO "songname_offset = 0x%08x", songname_offset);
		D_(D_INFO "expdata.songnamelen = %d", expdata.songnamelen);

		hio_seek(f, start + songname_offset, SEEK_SET);
		for (i = 0; i < expdata.songnamelen; i++) {
			if (i >= XMP_NAME_SIZE)
				break;
			mod->name[i] = hio_read8(f);
		}

		/* Read annotation */
		if (annotxt_offset != 0 && expdata.annolen != 0) {
			D_(D_INFO "annotxt_offset = 0x%08x", annotxt_offset);
			m->comment = malloc(expdata.annolen + 1);
			if (m->comment != NULL) {
				hio_seek(f, start + annotxt_offset, SEEK_SET);
				hio_read(m->comment, 1, expdata.annolen, f);
				m->comment[expdata.annolen] = 0;
			}
		}
	}

	/*
	 * Quickly scan patterns to check the number of channels
	 */
	D_(D_WARN "find number of channels");

	for (i = 0; i < mod->pat; i++) {
		int block_offset;

		if (hio_seek(f, start + blockarr_offset + i * 4, SEEK_SET) != 0)
			return -1;
		block_offset = hio_read32b(f);
		D_(D_INFO "block %d block_offset = 0x%08x", i, block_offset);
		if (block_offset == 0)
			continue;
		if (hio_seek(f, start + block_offset, SEEK_SET) != 0)
			return -1;

		if (ver > 0) {
			block.numtracks = hio_read16b(f);
			/* block.lines = */ hio_read16b(f);
		} else {
			block.numtracks = hio_read8(f);
			/* block.lines = */ hio_read8(f);
		}

		if (block.numtracks > mod->chn) {
			mod->chn = block.numtracks;
		}
	}

	/* Sanity check */
	/* MMD0/MMD1 can't have more than 16 channels... */
	if (mod->chn > MIN(16, XMP_MAX_CHANNELS)) {
		return -1;
	}

	mod->trk = mod->pat * mod->chn;

	libxmp_set_type(m, ver == 0 ? mod->chn > 4 ? "OctaMED 2.00 MMD0" :
				"MED 2.10 MMD0" : "OctaMED 4.00 MMD1");

	MODULE_INFO();

	D_(D_INFO "BPM mode: %s (length = %d)", bpm_on ? "on" : "off", bpmlen);
	D_(D_INFO "Song transpose: %d", song.playtransp);
	D_(D_INFO "Stored patterns: %d", mod->pat);

	/*
	 * Read and convert patterns
	 */
	D_(D_WARN "read patterns");
	if (libxmp_init_pattern(mod) < 0)
		return -1;

	for (i = 0; i < mod->pat; i++) {
		int block_offset;

		if (hio_seek(f, start + blockarr_offset + i * 4, SEEK_SET) != 0)
		  return -1;
		block_offset = hio_read32b(f);
		if (block_offset == 0)
			continue;
		if (hio_seek(f, start + block_offset, SEEK_SET) != 0)
		  return -1;

		if (ver > 0) {
			block.numtracks = hio_read16b(f);
			block.lines = hio_read16b(f);
			hio_read32b(f);
		} else {
			block.numtracks = hio_read8(f);
			block.lines = hio_read8(f);
		}

		/* Sanity check--Amiga OctaMED files have an upper bound of 3200 lines per block. */
		if (block.lines + 1 > 3200)
			return -1;

		if (libxmp_alloc_pattern_tracks_long(mod, i, block.lines + 1) < 0)
			return -1;

		if (ver > 0) {		/* MMD1 */
			for (j = 0; j < mod->xxp[i]->rows; j++) {
				for (k = 0; k < block.numtracks; k++) {
					e[0] = hio_read8(f);
					e[1] = hio_read8(f);
					e[2] = hio_read8(f);
					e[3] = hio_read8(f);

					event = &EVENT(i, k, j);
					event->note = e[0] & 0x7f;
					if (event->note)
						event->note +=
						    12 + song.playtransp;

					if (event->note >= XMP_MAX_KEYS)
						event->note = 0;

					event->ins = e[1] & 0x3f;

					/* Decay */
					if (event->ins && !event->note) {
						event->f2t = FX_MED_HOLD;
					}

					event->fxt = e[2];
					event->fxp = e[3];
					mmd_xlat_fx(event, bpm_on, bpmlen,
							med_8ch, hexvol);
				}
			}
		} else {		/* MMD0 */
			for (j = 0; j < mod->xxp[i]->rows; j++) {
				for (k = 0; k < block.numtracks; k++) {
					e[0] = hio_read8(f);
					e[1] = hio_read8(f);
					e[2] = hio_read8(f);

					event = &EVENT(i, k, j);
					event->note = e[0] & 0x3f;
					if (event->note)
						event->note += 12;

					if (event->note >= XMP_MAX_KEYS)
						event->note = 0;

					event->ins =
					    (e[1] >> 4) | ((e[0] & 0x80) >> 3)
					    | ((e[0] & 0x40) >> 1);

					/* Decay */
					if (event->ins && !event->note) {
						event->f2t = FX_MED_HOLD;
					}

					event->fxt = e[1] & 0x0f;
					event->fxp = e[2];
					mmd_xlat_fx(event, bpm_on, bpmlen,
							med_8ch, hexvol);
				}
			}
		}
	}

	if (libxmp_med_new_module_extras(m))
		return -1;

	/*
	 * Read and convert instruments and samples
	 */
	D_(D_WARN "read instruments");
	if (libxmp_init_instrument(m) < 0)
		return -1;

	D_(D_INFO "Instruments: %d", mod->ins);

	for (smp_idx = i = 0; i < mod->ins; i++) {
		int smpl_offset;

		if (hio_seek(f, start + smplarr_offset + i * 4, SEEK_SET) != 0)
			return -1;
		smpl_offset = hio_read32b(f);

		D_(D_INFO "sample %d smpl_offset = 0x%08x", i, smpl_offset);

		if (smpl_offset == 0) {
			continue;
		}
		if (hio_seek(f, start + smpl_offset, SEEK_SET) < 0) {
			return -1;
		}
		instr.length = hio_read32b(f);
		instr.type = hio_read16b(f);

		if ((pos = hio_tell(f)) < 0) {
			return -1;
		}

		if (expdata_offset && i < expdata.i_ext_entries) {
			struct xmp_instrument *xxi = &mod->xxi[i];
			int offset = iinfo_offset + i * expdata.i_ext_entrsz;
			D_(D_INFO "sample %d iinfo_offset = 0x%08x", i, offset);

			if (offset < 0 || hio_seek(f, start + offset, SEEK_SET) < 0) {
				return -1;
			}
			if (hio_read(name, 40, 1, f) < 1) {
				D_(D_CRIT "read error at iinfo %d", i);
				return -1;
			}
			strncpy(xxi->name, name, 32);
			xxi->name[31] = '\0';
		}

		D_(D_INFO "[%2x] %-40.40s %d", i, mod->xxi[i].name, instr.type);

		exp_smp.finetune = 0;
		if (expdata_offset && i < expdata.s_ext_entries) {
			int offset = expsmp_offset + i * expdata.s_ext_entrsz;
			D_(D_INFO "sample %d expsmp_offset = 0x%08x", i, offset);

			if (offset < 0 || hio_seek(f, start + offset, SEEK_SET) < 0) {
				return -1;
			}
			exp_smp.hold = hio_read8(f);
			exp_smp.decay = hio_read8(f);
			exp_smp.suppress_midi_off = hio_read8(f);
			exp_smp.finetune = hio_read8(f);
		}

		hio_seek(f, pos, SEEK_SET);

		if (instr.type == -2) {			/* Hybrid */
			int ret = mmd_load_hybrid_instrument(f, m, i, smp_idx,
				&synth, &exp_smp, &song.sample[i]);

			smp_idx++;

			if (ret < 0)
				return -1;

			if (mmd_alloc_tables(m, i, &synth) != 0)
				return -1;

			continue;
		} else if (instr.type == -1) {			/* Synthetic */
			int ret = mmd_load_synth_instrument(f, m, i, smp_idx,
				&synth, &exp_smp, &song.sample[i]);

			if (ret > 0)
				continue;

			if (ret < 0)
				return -1;

			smp_idx += synth.wforms;

			if (mmd_alloc_tables(m, i, &synth) != 0)
				return -1;

			continue;
		} else if (instr.type >= 1 && instr.type <= 6) { /* IFFOCT */
			int ret;
			const int oct = num_oct[instr.type - 1];

			hio_seek(f, start + smpl_offset + 6, SEEK_SET);

			ret = mmd_load_iffoct_instrument(f, m, i, smp_idx,
				&instr, oct, &exp_smp, &song.sample[i]);

			if (ret < 0)
				return -1;

			smp_idx += oct;

			continue;
		} else if (instr.type == 0) {			/* Sample */
			int ret;

			hio_seek(f, start + smpl_offset + 6, SEEK_SET);

			ret = mmd_load_sampled_instrument(f, m, i, smp_idx,
				&instr, &expdata, &exp_smp, &song.sample[i],
				ver);

			if (ret < 0)
				return -1;

			smp_idx++;

			continue;
		} else {
			/* Invalid instrument type */
			return -1;
		}
	}

	for (i = 0; i < mod->chn; i++) {
		mod->xxc[i].vol = song.trkvol[i];
		mod->xxc[i].pan = DEFPAN((((i + 1) / 2) % 2) * 0xff);
	}

	m->read_event_type = READ_EVENT_MED;

	return 0;
}
