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

#include "loader.h"
#include "../period.h"

#define MAGIC_PSM_	MAGIC4('P','S','M',0xfe)


static int psm_test (HIO_HANDLE *, char *, const int);
static int psm_load (struct module_data *, HIO_HANDLE *, const int);

const struct format_loader libxmp_loader_psm = {
	"Protracker Studio",
	psm_test,
	psm_load
};

static int psm_test(HIO_HANDLE *f, char *t, const int start)
{
	if (hio_read32b(f) != MAGIC_PSM_)
		return -1;

	libxmp_read_title(f, t, 60);

	return 0;
}


/* FIXME: effects translation */

static int psm_load(struct module_data *m, HIO_HANDLE *f, const int start)
{
	struct xmp_module *mod = &m->mod;
	int c, r, i;
	struct xmp_event *event;
	uint8 buf[1024];
	uint32 p_ord, p_chn, p_pat, p_ins;
	uint32 p_smp[256];
	int type, ver /*, mode*/;

	LOAD_INIT();

	hio_read32b(f);

	hio_read(buf, 1, 60, f);
	memcpy(mod->name, (char *)buf, 59);
	mod->name[59] = '\0';

	type = hio_read8(f);		/* song type */
	ver = hio_read8(f);		/* song version */
	/*mode =*/ hio_read8(f);	/* pattern version */

	if (type & 0x01)		/* song mode not supported */
		return -1;

	libxmp_set_type(m, "Protracker Studio PSM %d.%02d", MSN(ver), LSN(ver));

	mod->spd = hio_read8(f);
	mod->bpm = hio_read8(f);
	hio_read8(f);			/* master volume */
	hio_read16l(f);			/* song length */
	mod->len = hio_read16l(f);
	mod->pat = hio_read16l(f);
	mod->ins = hio_read16l(f);
	hio_read16l(f);			/* ignore channels to play */
	mod->chn = hio_read16l(f);	/* use channels to proceed */
	mod->smp = mod->ins;
	mod->trk = mod->pat * mod->chn;

	/* Sanity check */
	if (mod->len > 256 || mod->pat > 256 || mod->ins > 255 ||
	    mod->chn > XMP_MAX_CHANNELS) {
		return -1;
        }

	p_ord = hio_read32l(f);
	p_chn = hio_read32l(f);
	p_pat = hio_read32l(f);
	p_ins = hio_read32l(f);

	/* should be this way but fails with Silverball song 6 */
	//mod->flg |= ~type & 0x02 ? XXM_FLG_MODRNG : 0;

	m->c4rate = C4_NTSC_RATE;

	MODULE_INFO();

	hio_seek(f, start + p_ord, SEEK_SET);
	hio_read(mod->xxo, 1, mod->len, f);

	hio_seek(f, start + p_chn, SEEK_SET);
	hio_read(buf, 1, 16, f);

	if (libxmp_init_instrument(m) < 0)
		return -1;

	hio_seek(f, start + p_ins, SEEK_SET);
	for (i = 0; i < mod->ins; i++) {
		struct xmp_instrument *xxi = &mod->xxi[i];
		uint16 flags, c2spd;
		int finetune;

		if (libxmp_alloc_subinstrument(mod, i, 1) < 0)
			return -1;

		hio_read(buf, 1, 13, f);	/* sample filename */
		hio_read(buf, 1, 24, f);	/* sample description */
		buf[24] = 0;			/* add string termination */
		strncpy(xxi->name, (char *)buf, 24);
		xxi->name[24] = '\0';
		p_smp[i] = hio_read32l(f);
		hio_read32l(f);			/* memory location */
		hio_read16l(f);			/* sample number */
		flags = hio_read8(f);		/* sample type */
		mod->xxs[i].len = hio_read32l(f); 
		mod->xxs[i].lps = hio_read32l(f);
		mod->xxs[i].lpe = hio_read32l(f);
		finetune = (int8)(hio_read8(f) << 4);
		mod->xxi[i].sub[0].vol = hio_read8(f);
		c2spd = hio_read16l(f);
		mod->xxi[i].sub[0].pan = 0x80;
		mod->xxi[i].sub[0].sid = i;
		mod->xxs[i].flg = flags & 0x80 ? XMP_SAMPLE_LOOP : 0;
		mod->xxs[i].flg |= flags & 0x20 ? XMP_SAMPLE_LOOP_BIDIR : 0;

		libxmp_c2spd_to_note(c2spd, &mod->xxi[i].sub[0].xpo,
						&mod->xxi[i].sub[0].fin);
		mod->xxi[i].sub[0].fin += finetune;

		if (mod->xxs[i].len > 0)
			mod->xxi[i].nsm = 1;

		D_(D_INFO "[%2X] %-22.22s %04x %04x %04x %c V%02x %5d",
			i, mod->xxi[i].name, mod->xxs[i].len, mod->xxs[i].lps,
			mod->xxs[i].lpe, mod->xxs[i].flg & XMP_SAMPLE_LOOP ?
			'L' : ' ', mod->xxi[i].sub[0].vol, c2spd);
	}
	
	if (libxmp_init_pattern(mod) < 0)
		return -1;

	D_(D_INFO "Stored patterns: %d", mod->pat);

	hio_seek(f, start + p_pat, SEEK_SET);
	for (i = 0; i < mod->pat; i++) {
		int len;
		uint8 b, rows, chan;

		len = hio_read16l(f) - 4;
		rows = hio_read8(f);
		if (rows > 64) {
			return -1;
		}
		chan = hio_read8(f);
		if (chan > 32) {
			return -1;
		}

		if (libxmp_alloc_pattern_tracks(mod, i, rows) < 0)
			return -1;

		for (r = 0; r < rows; r++) {
			while (len > 0) {
				b = hio_read8(f);
				len--;

				if (b == 0)
					break;
	
				c = b & 0x0f;
				if (c >= mod->chn)
					return -1;
				event = &EVENT(i, c, r);
	
				if (b & 0x80) {
					event->note = hio_read8(f) + 36 + 1;
					event->ins = hio_read8(f);
					len -= 2;
				}
	
				if (b & 0x40) {
					event->vol = hio_read8(f) + 1;
					len--;
				}
	
				if (b & 0x20) {
					event->fxt = hio_read8(f);
					event->fxp = hio_read8(f);
					len -= 2;
				}
			}
		}

		if (len > 0)
			hio_seek(f, len, SEEK_CUR);
	}

	/* Read samples */

	D_(D_INFO "Stored samples: %d", mod->smp);

	for (i = 0; i < mod->ins; i++) {
		hio_seek(f, start + p_smp[i], SEEK_SET);
		if (libxmp_load_sample(m, f, SAMPLE_FLAG_DIFF,
				&mod->xxs[mod->xxi[i].sub[0].sid], NULL) < 0)
			return -1;
	}

	return 0;
}
