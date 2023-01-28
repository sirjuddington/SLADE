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

#include "med.h"
#include "loader.h"
#include "../med_extras.h"

static int mmd3_test (HIO_HANDLE *, char *, const int);
static int mmd3_load (struct module_data *, HIO_HANDLE *, const int);

const struct format_loader libxmp_loader_mmd3 = {
	"OctaMED",
	mmd3_test,
	mmd3_load
};

static int mmd3_test(HIO_HANDLE *f, char *t, const int start)
{
	char id[4];
	uint32 offset, len;

	if (hio_read(id, 1, 4, f) < 4)
		return -1;

	if (memcmp(id, "MMD2", 4) && memcmp(id, "MMD3", 4))
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

static int mmd3_load(struct module_data *m, HIO_HANDLE *f, const int start)
{
	struct xmp_module *mod = &m->mod;
	int i, j, k;
	struct MMD0 header;
	struct MMD2song song;
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
	int seqtable_offset;
	int trackvols_offset;
	int trackpans_offset;
	int blockarr_offset;
	int smplarr_offset;
	int expdata_offset;
	int expsmp_offset;
	int songname_offset;
	int iinfo_offset;
	int mmdinfo_offset;
	int playseq_offset;
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
	hio_seek(f, start + song_offset, SEEK_SET);
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
	D_(D_INFO "song.songlen = %d", song.songlen);
	seqtable_offset = hio_read32b(f);
	hio_read32b(f);
	trackvols_offset = hio_read32b(f);
	song.numtracks = hio_read16b(f);
	song.numpseqs = hio_read16b(f);
	trackpans_offset = hio_read32b(f);
	song.flags3 = hio_read32b(f);
	song.voladj = hio_read16b(f);
	song.channels = hio_read16b(f);
	song.mix_echotype = hio_read8(f);
	song.mix_echodepth = hio_read8(f);
	song.mix_echolen = hio_read16b(f);
	song.mix_stereosep = hio_read8(f);

	hio_seek(f, 223, SEEK_CUR);

	song.deftempo = hio_read16b(f);
	song.playtransp = hio_read8(f);
	song.flags = hio_read8(f);
	song.flags2 = hio_read8(f);
	song.tempo2 = hio_read8(f);
	for (i = 0; i < 16; i++)
		hio_read8(f);		/* reserved */
	song.mastervol = hio_read8(f);
	song.numsamples = hio_read8(f);

	/* Sanity check */
	if (song.numsamples > 63) {
		return -1;
	}

	/*
	 * read sequence
	 */
	hio_seek(f, start + seqtable_offset, SEEK_SET);
	playseq_offset = hio_read32b(f);
	hio_seek(f, start + playseq_offset, SEEK_SET);
	hio_seek(f, 32, SEEK_CUR);	/* skip name */
	hio_read32b(f);
	hio_read32b(f);
	mod->len = hio_read16b(f);

	/* Sanity check */
	if (mod->len > 255) {
		return -1;
	}

	for (i = 0; i < mod->len; i++) {
		mod->xxo[i] = hio_read16b(f);
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
	mod->rst = 0;
	mod->chn = 0;
	mod->name[0] = 0;

	/*
	 * Obtain number of samples from each instrument
	 */
	mod->smp = 0;
	for (i = 0; i < mod->ins; i++) {
		uint32 smpl_offset;
		int16 type;
		hio_seek(f, start + smplarr_offset + i * 4, SEEK_SET);
		smpl_offset = hio_read32b(f);
		if (smpl_offset == 0)
			continue;
		hio_seek(f, start + smpl_offset, SEEK_SET);
		hio_read32b(f);				/* length */
		type = hio_read16b(f);
		if (type == -1) {			/* type is synth? */
			hio_seek(f, 14, SEEK_CUR);
			mod->smp += hio_read16b(f);	/* wforms */
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
		hio_seek(f, start + expdata_offset, SEEK_SET);
		hio_read32b(f);
		expsmp_offset = hio_read32b(f);
		D_(D_INFO "expsmp_offset = 0x%08x", expsmp_offset);
		expdata.s_ext_entries = hio_read16b(f);
		expdata.s_ext_entrsz = hio_read16b(f);
		hio_read32b(f);		/* annotxt */
		hio_read32b(f);		/* annolen */
		iinfo_offset = hio_read32b(f);
		D_(D_INFO "iinfo_offset = 0x%08x", iinfo_offset);
		expdata.i_ext_entries = hio_read16b(f);
		expdata.i_ext_entrsz = hio_read16b(f);

		/* Sanity check */
		if (expsmp_offset < 0 || iinfo_offset < 0) {
			return -1;
		}

		hio_read32b(f);
		hio_read32b(f);
		hio_read32b(f);
		hio_read32b(f);
		songname_offset = hio_read32b(f);
		D_(D_INFO "songname_offset = 0x%08x", songname_offset);
		expdata.songnamelen = hio_read32b(f);
		hio_read32b(f);		/* dumps */
		mmdinfo_offset = hio_read32b(f);

		if (hio_error(f)) {
			return -1;
		}

		hio_seek(f, start + songname_offset, SEEK_SET);
		D_(D_INFO "expdata.songnamelen = %d", expdata.songnamelen);
		for (i = 0; i < expdata.songnamelen; i++) {
			if (i >= XMP_NAME_SIZE)
				break;
			mod->name[i] = hio_read8(f);
		}

		if (mmdinfo_offset != 0) {
			D_(D_INFO "mmdinfo_offset = 0x%08x", mmdinfo_offset);
			hio_seek(f, start + mmdinfo_offset, SEEK_SET);
			mmd_info_text(f, m, mmdinfo_offset);
		}
	}

	/*
	 * Quickly scan patterns to check the number of channels
	 */
	D_(D_WARN "find number of channels");

	for (i = 0; i < mod->pat; i++) {
		int block_offset;

		hio_seek(f, start + blockarr_offset + i * 4, SEEK_SET);
		block_offset = hio_read32b(f);
		D_(D_INFO "block %d block_offset = 0x%08x", i, block_offset);
		if (hio_error(f)) {
			return -1;
		}
		if (block_offset == 0)
			continue;
		hio_seek(f, start + block_offset, SEEK_SET);

		block.numtracks = hio_read16b(f);
		/* block.lines = */ hio_read16b(f);
		if (hio_error(f)) {
			return -1;
		}

		if (block.numtracks > mod->chn) {
			mod->chn = block.numtracks;
		}
	}

	/* Sanity check */
	if (mod->chn <= 0 || mod->chn > XMP_MAX_CHANNELS)
		return -1;

	mod->trk = mod->pat * mod->chn;

	if (ver == 2)
		libxmp_set_type(m, "OctaMED v5 MMD2");
	else
		libxmp_set_type(m, "OctaMED Soundstudio MMD%c", '0' + ver);

	MODULE_INFO();

	D_(D_INFO "BPM mode: %s (length = %d)", bpm_on ? "on" : "off", bpmlen);
	D_(D_INFO "Song transpose : %d", song.playtransp);
	D_(D_INFO "Stored patterns: %d", mod->pat);

	/*
	 * Read and convert patterns
	 */
	D_(D_WARN "read patterns");

	if (libxmp_init_pattern(mod) < 0)
		return -1;

	for (i = 0; i < mod->pat; i++) {
		int block_offset;

		hio_seek(f, start + blockarr_offset + i * 4, SEEK_SET);
		block_offset = hio_read32b(f);
		if (block_offset == 0)
			continue;
		hio_seek(f, start + block_offset, SEEK_SET);

		block.numtracks = hio_read16b(f);
		block.lines = hio_read16b(f);
		hio_read32b(f);

		/* Sanity check--Amiga OctaMED files have an upper bound of 3200 lines per block,
		 * but MED Soundstudio for Windows allows up to 9999 lines.
		  */
		if (block.lines + 1 > 9999)
			return -1;

		if (libxmp_alloc_pattern_tracks_long(mod, i, block.lines + 1) < 0)
			return -1;

		for (j = 0; j < mod->xxp[i]->rows; j++) {
			for (k = 0; k < block.numtracks; k++) {
				e[0] = hio_read8(f);
				e[1] = hio_read8(f);
				e[2] = hio_read8(f);
				e[3] = hio_read8(f);

				event = &EVENT(i, k, j);
				event->note = e[0] & 0x7f;
				if (event->note) {
					event->note += song.playtransp;
					if (ver == 2)
						event->note += 12;
					else
						event->note -= 12;
				};

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
	}

	if (libxmp_med_new_module_extras(m) != 0)
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

		hio_seek(f, start + smplarr_offset + i * 4, SEEK_SET);
		smpl_offset = hio_read32b(f);

		D_(D_INFO "sample %d smpl_offset = 0x%08x", i, smpl_offset);

		if (smpl_offset == 0) {
			continue;
		}

		hio_seek(f, start + smpl_offset, SEEK_SET);
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
			D_(D_INFO "[%2x] %-40.40s %d", i, mod->xxi[i].name, instr.type);
		}

		memset(&exp_smp, 0, sizeof(struct InstrExt));

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

			if (expdata.s_ext_entrsz > 4) {	/* Octamed V5 */
				exp_smp.default_pitch = hio_read8(f);
				exp_smp.instr_flags = hio_read8(f);
			}
		}

		hio_seek(f, pos, SEEK_SET);

		if (instr.type == -2) {			/* Hybrid */
			int ret = mmd_load_hybrid_instrument(f, m, i,
				smp_idx, &synth, &exp_smp, &song.sample[i]);

			if (ret < 0)
				return -1;

			smp_idx++;

			if (mmd_alloc_tables(m, i, &synth) != 0)
				return -1;

			continue;
		} else if (instr.type == -1) {		/* Synthetic */
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
		} else if ((instr.type & STEREO) != 0) {
			D_(D_WARN "stereo sample unsupported");
			mod->xxi[i].nsm = 0;
			continue;
		} else if ((instr.type & S_16) == 0) {	/* Sample */
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
			D_(D_CRIT "invalid instrument type: %d", instr.type);
			return -1;
		}
	}

	hio_seek(f, start + trackvols_offset, SEEK_SET);
	for (i = 0; i < mod->chn; i++)
		mod->xxc[i].vol = hio_read8(f);;

	if (trackpans_offset) {
		hio_seek(f, start + trackpans_offset, SEEK_SET);
		for (i = 0; i < mod->chn; i++) {
			int p = 8 * hio_read8s(f);
			mod->xxc[i].pan = 0x80 + (p > 127 ? 127 : p);
		}
	} else {
		for (i = 0; i < mod->chn; i++)
			mod->xxc[i].pan = 0x80;
	}

	m->read_event_type = READ_EVENT_MED;

	return 0;
}
