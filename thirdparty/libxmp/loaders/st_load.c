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

/* Ultimate Soundtracker support based on the module format description
 * written by Michael Schwendt <sidplay@geocities.com>
 */

#include "loader.h"
#include "mod.h"
#include "../period.h"

static int st_test(HIO_HANDLE *, char *, const int);
static int st_load(struct module_data *, HIO_HANDLE *, const int);

const struct format_loader libxmp_loader_st = {
	"Soundtracker",
	st_test,
	st_load
};

static const int period[] = {
	856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
	428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
	-1
};

static int st_test(HIO_HANDLE *f, char *t, const int start)
{
	int i, j, k;
	int pat, ins, smp_size;
	struct st_header mh;
	uint8 mod_event[4];
	long size;

	size = hio_size(f);

	if (size < 600) {
		return -1;
	}

	smp_size = 0;

	hio_seek(f, start, SEEK_SET);
	hio_read(mh.name, 1, 20, f);
	if (libxmp_test_name(mh.name, 20) < 0) {
		return -1;
	}

	for (i = 0; i < 15; i++) {
		hio_read(mh.ins[i].name, 1, 22, f);
		mh.ins[i].size = hio_read16b(f);
		mh.ins[i].finetune = hio_read8(f);
		mh.ins[i].volume = hio_read8(f);
		mh.ins[i].loop_start = hio_read16b(f);
		mh.ins[i].loop_size = hio_read16b(f);
	}
	mh.len = hio_read8(f);
	mh.restart = hio_read8(f);
	hio_read(mh.order, 1, 128, f);

	for (pat = i = 0; i < 128; i++) {
		if (mh.order[i] > 0x7f)
			return -1;
		if (mh.order[i] > pat)
			pat = mh.order[i];
	}
	pat++;

	if (pat > 0x7f || mh.len == 0 || mh.len > 0x7f)
		return -1;

	for (i = 0; i < 15; i++) {
		/* Crepequs.mod has random values in first byte */
		mh.ins[i].name[0] = 'X';

		if (libxmp_test_name(mh.ins[i].name, 22) < 0)
			return -1;

		if (mh.ins[i].volume > 0x40)
			return -1;

		if (mh.ins[i].finetune > 0x0f)
			return -1;

		if (mh.ins[i].size > 0x8000)
			return -1;

		/* This test is always false, disable it
		 *
		 * if ((mh.ins[i].loop_start >> 1) > 0x8000)
		 *    return -1;
		 */

		if (mh.ins[i].loop_size > 0x8000)
			return -1;

		/* This test fails in atmosfer.mod, disable it
		 *
		 * if (mh.ins[i].loop_size > 1 && mh.ins[i].loop_size > mh.ins[i].size)
		 *    return -1;
		 */

		if ((mh.ins[i].loop_start >> 1) > mh.ins[i].size)
			return -1;

		if (mh.ins[i].size
		    && (mh.ins[i].loop_start >> 1) == mh.ins[i].size)
			return -1;

		if (mh.ins[i].size == 0 && mh.ins[i].loop_start > 0)
			return -1;

		smp_size += 2 * mh.ins[i].size;
	}

	if (smp_size < 8) {
		return -1;
	}

	for (ins = i = 0; i < pat; i++) {
		for (j = 0; j < (64 * 4); j++) {
			int p, s;

			if (hio_read(mod_event, 1, 4, f) < 4) {
				return -1;
			}

			s = (mod_event[0] & 0xf0) | MSN(mod_event[2]);

			if (s > 15) {	/* sample number > 15 */
				/* cant.mod has this invalid sample number */
				if (!(s == 64 && i == 3 && j == 183)) {
					D_(D_CRIT "%d/%d/%d: invalid sample number: %d", i, j / 4, j % 4, s);
					return -1;
				}
			}

			if (s > ins) {	/* find highest used sample */
				ins = s;
			}

			p = 256 * LSN(mod_event[0]) + mod_event[1];

			if (p == 0) {
				continue;
			}

			/* another special check for cant.mod */
			if (p == 3792 && i == 3 && j == 183) {
				continue;
			}

			/* used in Karsten Obarski's blueberry.mod */
			if (p == 162) {
				continue;
			}

			for (k = 0; period[k] >= 0; k++) {
				if (p == period[k])
					break;
			}
			if (period[k] < 0) {
				D_(D_CRIT "%d/%d/%d: invalid period", i, j / 4, j % 4);
				return -1;
			}
		}
	}

	/* Check if file was cut before any unused samples */
	if (size < 600 + pat * 1024 + smp_size) {
		int ss;
		for (ss = i = 0; i < 15 && i < ins; i++) {
			ss += 2 * mh.ins[i].size;
		}

		if (size < 600 + pat * 1024 + ss) {
			return -1;
		}
	}

	hio_seek(f, start, SEEK_SET);
	libxmp_read_title(f, t, 20);

	return 0;
}

static int st_load(struct module_data *m, HIO_HANDLE *f, const int start)
{
	struct xmp_module *mod = &m->mod;
	int i, j;
	int smp_size;
	struct xmp_event ev, *event;
	struct st_header mh;
	uint8 mod_event[4];
	int ust = 1;
	/* int lps_mult = m->fetch & XMP_CTL_FIXLOOP ? 1 : 2; */
	const char *modtype;
	int fxused;
	int pos;
	int used_ins;		/* Number of samples actually used */
	long size;

	LOAD_INIT();

	size = hio_size(f);

	mod->chn = 4;
	mod->ins = 15;
	mod->smp = mod->ins;
	smp_size = 0;

	hio_read(mh.name, 1, 20, f);
	for (i = 0; i < 15; i++) {
		hio_read(mh.ins[i].name, 1, 22, f);
		mh.ins[i].size = hio_read16b(f);
		mh.ins[i].finetune = hio_read8(f);
		mh.ins[i].volume = hio_read8(f);
		mh.ins[i].loop_start = hio_read16b(f);
		mh.ins[i].loop_size = hio_read16b(f);
	}
	mh.len = hio_read8(f);
	mh.restart = hio_read8(f);
	hio_read(mh.order, 1, 128, f);

	mod->len = mh.len;
	mod->rst = mh.restart;

	/* UST: The byte at module offset 471 is BPM, not the song restart
	 *      The default for UST modules is 0x78 = 120 BPM = 48 Hz.
	 */
	if (mod->rst < 0x40)	/* should be 0x20 */
		ust = 0;

	memcpy(mod->xxo, mh.order, 128);

	for (i = 0; i < 128; i++) {
		if (mod->xxo[i] > mod->pat)
			mod->pat = mod->xxo[i];
	}
	mod->pat++;

	for (i = 0; i < mod->ins; i++) {
		/* UST: Volume word does not contain a "Finetuning" value in its
		 * high-byte.
		 */
		if (mh.ins[i].finetune)
			ust = 0;

		/* if (mh.ins[i].size == 0 && mh.ins[i].loop_size == 1)
		   nt = 1; */

		/* UST: Maximum sample length is 9999 bytes decimal, but 1387
		 * words hexadecimal. Longest samples on original sample disk
		 * ST-01 were 9900 bytes.
		 */
		if (mh.ins[i].size > 0x1387 || mh.ins[i].loop_start > 9999
		    || mh.ins[i].loop_size > 0x1387) {
			ust = 0;
		}

		smp_size += 2 * mh.ins[i].size;
	}

	if (libxmp_init_instrument(m) < 0) {
		return -1;
	}

	for (i = 0; i < mod->ins; i++) {
		struct xmp_instrument *xxi = &mod->xxi[i];
		struct xmp_sample *xxs = &mod->xxs[i];
		struct xmp_subinstrument *sub;

		if (libxmp_alloc_subinstrument(mod, i, 1) < 0)
			return -1;

		sub = &xxi->sub[0];

		xxs->len = 2 * mh.ins[i].size - mh.ins[i].loop_start;
		xxs->lps = 0;
		xxs->lpe = xxs->lps + 2 * mh.ins[i].loop_size;
		xxs->flg = mh.ins[i].loop_size > 1 ? XMP_SAMPLE_LOOP : 0;
		sub->fin = (int8) (mh.ins[i].finetune << 4);
		sub->vol = mh.ins[i].volume;
		sub->pan = 0x80;
		sub->sid = i;
		strncpy((char *)xxi->name, (char *)mh.ins[i].name, 22);

		if (xxs->len > 0) {
			xxi->nsm = 1;
		}
	}

	mod->trk = mod->chn * mod->pat;

	strncpy(mod->name, (char *)mh.name, 20);

	/* Scan patterns for tracker detection */
	fxused = 0;
	pos = hio_tell(f);

	for (i = 0; i < mod->pat; i++) {
		for (j = 0; j < (64 * mod->chn); j++) {
			hio_read(mod_event, 1, 4, f);

			libxmp_decode_protracker_event(&ev, mod_event);

			if (ev.fxt)
				fxused |= 1 << ev.fxt;
			else if (ev.fxp)
				fxused |= 1;

			/* UST: Only effects 1 (arpeggio) and 2 (pitchbend) are
			 * available.
			 */
			if (ev.fxt && ev.fxt != 1 && ev.fxt != 2)
				ust = 0;

			/* Karsten Obarski's sleepwalk uses arpeggio 30 and 40 */
			if (ev.fxt == 1) {	/* unlikely arpeggio */
				if (ev.fxp == 0x00)
					ust = 0;
				/*if ((ev.fxp & 0x0f) == 0 || (ev.fxp & 0xf0) == 0)
				   ust = 0; */
			}

			if (ev.fxt == 2) {  /* bend up and down at same time? */
				if ((ev.fxp & 0x0f) != 0 && (ev.fxp & 0xf0) != 0)
					ust = 0;
			}
		}
	}

	if (fxused & ~0x0006) {
		ust = 0;
	}

	if (ust) {
		modtype = "Ultimate Soundtracker";
	} else if ((fxused & ~0xd007) == 0) {
		modtype = "Soundtracker IX";	/* or MasterSoundtracker? */
	} else if ((fxused & ~0xf807) == 0) {
		modtype = "D.O.C Soundtracker 2.0";
	} else {
		modtype = "unknown tracker 15 instrument";
	}

	snprintf(mod->type, XMP_NAME_SIZE, "%s", modtype);

	MODULE_INFO();

	if (hio_seek(f, start + pos, SEEK_SET) < 0) {
		return -1;
	}

	if (libxmp_init_pattern(mod) < 0) {
		return -1;
	}

	/* Load and convert patterns */

	D_(D_INFO "Stored patterns: %d", mod->pat);

	used_ins = 0;
	for (i = 0; i < mod->pat; i++) {
		if (libxmp_alloc_pattern_tracks(mod, i, 64) < 0)
			return -1;

		for (j = 0; j < (64 * mod->chn); j++) {
			event = &EVENT(i, j % mod->chn, j / mod->chn);
			hio_read(mod_event, 1, 4, f);

			libxmp_decode_protracker_event(event, mod_event);

			if (ev.ins > used_ins)
				used_ins = ev.ins;
		}
	}

	for (i = 0; i < mod->ins; i++) {
		D_(D_INFO "[%2X] %-22.22s %04x %04x %04x %c V%02x %+d",
		   i, mod->xxi[i].name, mod->xxs[i].len, mod->xxs[i].lps,
		   mod->xxs[i].lpe, mh.ins[i].loop_size > 1 ? 'L' : ' ',
		   mod->xxi[i].sub[0].vol, mod->xxi[i].sub[0].fin >> 4);
	}

	m->quirk |= QUIRK_NOBPM;
	m->period_type = PERIOD_MODRNG;

	/* Perform the necessary conversions for Ultimate Soundtracker */
	if (ust) {
		/* Fix restart & bpm */
		mod->bpm = mod->rst;
		mod->rst = 0;

		/* Fix sample loops */
		for (i = 0; i < mod->ins; i++) {
			/* FIXME */
		}

		/* Fix effects (arpeggio and pitchbending) */
		for (i = 0; i < mod->pat; i++) {
			for (j = 0; j < (64 * mod->chn); j++) {
				event = &EVENT(i, j % mod->chn, j / mod->chn);
				if (event->fxt == 1)
					event->fxt = 0;
				else if (event->fxt == 2
					 && (event->fxp & 0xf0) == 0)
					event->fxt = 1;
				else if (event->fxt == 2
					 && (event->fxp & 0x0f) == 0)
					event->fxp >>= 4;
			}
		}
	} else {
		if (mod->rst >= mod->len)
			mod->rst = 0;
	}

	/* Load samples */

	D_(D_INFO "Stored samples: %d", mod->smp);

	for (i = 0; i < mod->ins; i++) {
		if (!mod->xxs[i].len)
			continue;

		/* Skip transient part of sample.
		 *
		 * Dennis Lindroos <denafcm@gmail.com> reports: One main thing is
		 * sample-looping which on all trackers up to Noisetracker 1 (i
		 * think it was Mahoney who actually changed the loopstart to be
		 * in WORDS) never play looped samples from the beginning, i.e.
		 * only plays the looped part. This can be heard in old modules
		 * especially with "analogstring", "strings2" or "strings3"
		 * samples because these have "slow attack" that is not part of
		 * the loop and thus they sound "sharper"..
		 */
		hio_seek(f, mh.ins[i].loop_start, SEEK_CUR);

		if (libxmp_load_sample(m, f, 0, &mod->xxs[i], NULL) < 0) {
			return -1;
		}
	}

	return 0;
}
