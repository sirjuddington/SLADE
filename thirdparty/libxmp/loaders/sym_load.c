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
#ifndef LIBXMP_NO_DEPACKERS
#include "depackers/readlzw.h"
#else
#include "../readrle.c"
#include "../readlzw.c"
#endif


static int sym_test(HIO_HANDLE *, char *, const int);
static int sym_load (struct module_data *, HIO_HANDLE *, const int);

const struct format_loader libxmp_loader_sym = {
	"Digital Symphony",
	sym_test,
	sym_load
};

static int sym_test(HIO_HANDLE *f, char *t, const int start)
{
	uint32 a, b;
	int i, ver;

	/* Load from memory not supported until we handle sample depacking */
	if (HIO_HANDLE_TYPE(f) != HIO_HANDLE_TYPE_FILE)
		return -1;

	a = hio_read32b(f);
	b = hio_read32b(f);

	if (a != 0x02011313 || b != 0x1412010B)		/* BASSTRAK */
		return -1;

	ver = hio_read8(f);

	/* v1 files are the same as v0 but may contain strange compression
	 * formats. Deal with that problem later if it arises.
	 */
	if (ver > 1) {
		return -1;
	}

	hio_read8(f);		/* chn */
	hio_read16l(f);		/* pat */
	hio_read16l(f);		/* trk */
	hio_read24l(f);		/* infolen */

	for (i = 0; i < 63; i++) {
		if (~hio_read8(f) & 0x80)
			hio_read24l(f);
	}

	libxmp_read_title(f, t, hio_read8(f));

	return 0;
}



static void fix_effect(struct xmp_event *e, int parm)
{
	switch (e->fxt) {
	case 0x00:	/* 00 xyz Normal play or Arpeggio + Volume Slide Up */
	case 0x01:	/* 01 xyy Slide Up + Volume Slide Up */
	case 0x02:	/* 01 xyy Slide Up + Volume Slide Up */
		e->fxp = parm & 0xff;
		if (parm >> 8) {
			e->f2t = FX_VOLSLIDE_UP;
			e->f2p = parm >> 8;
		}
		break;
	case 0x03:	/* 03 xyy Tone Portamento */
	case 0x04:	/* 04 xyz Vibrato */
	case 0x07:	/* 07 xyz Tremolo */
		e->fxp = parm;
		break;
	case 0x05:	/* 05 xyz Tone Portamento + Volume Slide */
	case 0x06:	/* 06 xyz Vibrato + Volume Slide */
		e->fxp = parm;
		if (!parm)
			e->fxt -= 2;
		break;
	case 0x09:	/* 09 xxx Set Sample Offset */
		e->fxp = parm >> 1;
		break;
	case 0x0a:	/* 0A xyz Volume Slide + Fine Slide Up */
		if (parm & 0xff) {
			e->fxp = parm & 0xff;
		} else {
			e->fxt = 0;
		}
		e->f2t = FX_EXTENDED;
		e->f2p = (EX_F_PORTA_UP << 4) | ((parm & 0xf00) >> 8);
		break;
	case 0x0b:	/* 0B xxx Position Jump */
	case 0x0c:	/* 0C xyy Set Volume */
	case 0x0d:	/* 0D xyy Pattern Break */
	case 0x0f:	/* 0F xxx Set Speed */
		e->fxp = parm;
		break;
	case 0x13:	/* 13 xxy Glissando Control */
		e->fxt = FX_EXTENDED;
		e->fxp = (EX_GLISS << 4) | (parm & 0x0f);
		break;
	case 0x14:	/* 14 xxy Set Vibrato Waveform */
		e->fxt = FX_EXTENDED;
		e->fxp = (EX_VIBRATO_WF << 4) | (parm & 0x0f);
		break;
	case 0x15:	/* 15 xxy Set Fine Tune */
		e->fxt = FX_EXTENDED;
		e->fxp = (EX_FINETUNE << 4) | (parm & 0x0f);
		break;
	case 0x16:	/* 16 xxx Jump to Loop */
		e->fxt = FX_EXTENDED;
		e->fxp = (EX_PATTERN_LOOP << 4) | (parm & 0x0f);
		break;
	case 0x17:	/* 17 xxy Set Tremolo Waveform */
		e->fxt = FX_EXTENDED;
		e->fxp = (EX_TREMOLO_WF << 4) | (parm & 0x0f);
		break;
	case 0x19:	/* 19 xxx Retrig Note */
		if (parm < 0x10) {
			e->fxt = FX_EXTENDED;
			e->fxp = (EX_RETRIG << 4) | (parm & 0x0f);
		} else {
			/* ignore */
			e->fxt = 0;
		}
		break;
	case 0x11:	/* 11 xyy Fine Slide Up + Fine Volume Slide Up */
	case 0x12:	/* 12 xyy Fine Slide Down + Fine Volume Slide Up */
	case 0x1a:	/* 1A xyy Fine Slide Up + Fine Volume Slide Down */
	case 0x1b:	/* 1B xyy Fine Slide Down + Fine Volume Slide Down */
	{
		uint8 pitch_effect = ((e->fxt == 0x11 || e->fxt == 0x1a) ?
				      EX_F_PORTA_UP : EX_F_PORTA_DN);
		uint8 vol_effect = ((e->fxt == 0x11 || e->fxt == 0x12) ?
				      EX_F_VSLIDE_UP : EX_F_VSLIDE_DN);

		if ((parm & 0xff) && ((parm & 0xff) < 0x10)) {
			e->fxt = FX_EXTENDED;
			e->fxp = (pitch_effect << 4) | (parm & 0x0f);
		} else
			e->fxt = 0;
		if (parm >> 8) {
			e->f2t = FX_EXTENDED;
			e->f2p = (vol_effect << 4) | (parm >> 8);
		}
		break;
	}
	case 0x1c:	/* 1C xxx Note Cut */
		e->fxt = FX_EXTENDED;
		e->fxp = (EX_CUT << 4) | (parm & 0x0f);
		break;
	case 0x1d:	/* 1D xxx Note Delay */
		e->fxt = FX_EXTENDED;
		e->fxp = (EX_DELAY << 4) | (parm & 0x0f);
		break;
	case 0x1e:	/* 1E xxx Pattern Delay */
		e->fxt = FX_EXTENDED;
		e->fxp = (EX_PATT_DELAY << 4) | (parm & 0x0f);
		break;
	case 0x1f:	/* 1F xxy Invert Loop */
		e->fxt = 0;
		break;
	case 0x20:	/* 20 xyz Normal play or Arpeggio + Volume Slide Down */
		e->fxt = FX_ARPEGGIO;
		e->fxp = parm & 0xff;
		if (parm >> 8) {
			e->f2t = FX_VOLSLIDE_DN;
			e->f2p = parm >> 8;
		}
		break;
	case 0x21:	/* 21 xyy Slide Up + Volume Slide Down */
		e->fxt = FX_PORTA_UP;
		e->fxp = parm & 0xff;
		if (parm >> 8) {
			e->f2t = FX_VOLSLIDE_DN;
			e->f2p = parm >> 8;
		}
		break;
	case 0x22:	/* 22 xyy Slide Down + Volume Slide Down */
		e->fxt = FX_PORTA_DN;
		e->fxp = parm & 0xff;
		if (parm >> 8) {
			e->f2t = FX_VOLSLIDE_DN;
			e->f2p = parm >> 8;
		}
		break;
	case 0x2f:	/* 2F xxx Set Tempo */
		if (parm >= 0x100 && parm <= 0x800) {
			e->fxt = FX_SPEED;
			e->fxp = (parm+4) >> 3; /* round to nearest */
		} else {
			/* umm... */
		}
		break;
	case 0x2a:	/* 2A xyz Volume Slide + Fine Slide Down */
	case 0x2b:	/* 2B xyy Line Jump */
	case 0x30:	/* 30 xxy Set Stereo */
	case 0x31:	/* 31 xxx Song Upcall */
	case 0x32:	/* 32 xxx Unset Sample Repeat */
	default:
		e->fxt = 0;
	}
}

static uint32 readptr32l(uint8 *p)
{
	uint32 a, b, c, d;

	a = *p++;
	b = *p++;
	c = *p++;
	d = *p++;

	return (d << 24) | (c << 16) | (b << 8) | a;
}

static uint32 readptr16l(uint8 *p)
{
	uint32 a, b;

	a = *p++;
	b = *p++;

	return (b << 8) | a;
}

static int sym_load(struct module_data *m, HIO_HANDLE *f, const int start)
{
	struct xmp_module *mod = &m->mod;
	struct xmp_event *event;
	int i, j;
	int /*ver,*/ infolen, sn[64];
	uint32 a, b;
	uint8 *buf;
	int size, ret;
	uint8 allowed_effects[8];

	LOAD_INIT();

	hio_seek(f, 8, SEEK_CUR);	/* BASSTRAK */

	/*ver =*/ hio_read8(f);
	libxmp_set_type(m, "Digital Symphony");

	mod->chn = hio_read8(f);
	mod->len = mod->pat = hio_read16l(f);

	/* Sanity check */
	if (mod->chn > 8 || mod->pat > 256)
		return -1;

	mod->trk = hio_read16l(f);	/* Symphony patterns are actually tracks */
	infolen = hio_read24l(f);

	mod->ins = mod->smp = 63;

	if (libxmp_init_instrument(m) < 0)
		return -1;

	for (i = 0; i < mod->ins; i++) {
		if (libxmp_alloc_subinstrument(mod, i, 1) < 0)
			return -1;

		sn[i] = hio_read8(f);	/* sample name length */

		if (~sn[i] & 0x80) {
			mod->xxs[i].len = hio_read24l(f) << 1;
			mod->xxi[i].nsm = 1;

			/* Sanity check */
			if (mod->xxs[i].len > 0x80000)
				return -1;
		}
	}

	a = hio_read8(f);		/* track name length */
	if (a > 32) {
		hio_read(mod->name, 1, 32, f);
		hio_seek(f, a - 32, SEEK_SET);
	} else {
		hio_read(mod->name, 1, a, f);
	}

	hio_read(allowed_effects, 1, 8, f);

	MODULE_INFO();

	mod->trk++;			/* alloc extra empty track */
	if (libxmp_init_pattern(mod) < 0)
		return -1;

	/* Sequence */
	a = hio_read8(f);		/* packing */

	if (a != 0 && a != 1)
		return -1;

	D_(D_INFO "Packed sequence: %s", a ? "yes" : "no");

	size = mod->len * mod->chn * 2;
	if ((buf = malloc(size)) == NULL)
		return -1;

	if (a) {
		unsigned char *x = libxmp_read_lzw_dynamic(f->handle.file, buf,
					13, 0, size, size, XMP_LZW_QUIRK_DSYM);
		if (x == NULL) {
			free(buf);
			return -1;
		}
	} else {
		if (hio_read(buf, 1, size, f) != size) {
			free(buf);
			return -1;
		}
	}

	for (i = 0; i < mod->len; i++) {	/* len == pat */
		if (libxmp_alloc_pattern(mod, i) < 0) {
			free(buf);
			return -1;
		}
		mod->xxp[i]->rows = 64;

		for (j = 0; j < mod->chn; j++) {
			int idx = 2 * (i * mod->chn + j);
			int t = readptr16l(&buf[idx]);

			if (t == 0x1000) {
				/* empty trk */
				t = mod->trk - 1;
			} else if (t >= mod->trk - 1) {
				/* Sanity check */
				free(buf);
				return -1;
			}

			mod->xxp[i]->index[j] = t;
		}
		mod->xxo[i] = i;
	}
	free(buf);

	/* Read and convert patterns */

	a = hio_read8(f);

	if (a != 0 && a != 1)
		return -1;

	D_(D_INFO "Packed tracks: %s", a ? "yes" : "no");
	D_(D_INFO "Stored tracks: %d", mod->trk - 1);

	size = 64 * (mod->trk - 1) * 4;
	if ((buf = malloc(size)) == NULL)
		return -1;

	if (a) {
		unsigned char *x = libxmp_read_lzw_dynamic(f->handle.file, buf,
					13, 0, size, size, XMP_LZW_QUIRK_DSYM);
		if (x == NULL) {
			free(buf);
			return -1;
		}
	} else {
		if (hio_read(buf, 1, size, f) != size) {
			free(buf);
			return -1;
		}
	}

	for (i = 0; i < mod->trk - 1; i++) {
		if (libxmp_alloc_track(mod, i, 64) < 0) {
			free(buf);
			return -1;
		}

		for (j = 0; j < mod->xxt[i]->rows; j++) {
			int parm;

			event = &mod->xxt[i]->event[j];

			b = readptr32l(&buf[4 * (i * 64 + j)]);
			event->note = b & 0x0000003f;
			if (event->note)
				event->note += 48;
			event->ins = (b & 0x00001fc0) >> 6;
			event->fxt = (b & 0x000fc000) >> 14;
			parm = (b & 0xfff00000) >> 20;

			if (allowed_effects[event->fxt >> 3] & (1 << (event->fxt & 7))) {
				fix_effect(event, parm);
			} else {
				event->fxt = 0;
			}
		}
	}

	free(buf);

	/* Extra track */
	if (libxmp_alloc_track(mod, i, 64) < 0)
		return -1;

	/* Load and convert instruments */

	D_(D_INFO "Instruments: %d", mod->ins);

	for (i = 0; i < mod->ins; i++) {
		uint8 buf[128];

		memset(buf, 0, sizeof(buf));
		hio_read(buf, 1, sn[i] & 0x7f, f);
		libxmp_instrument_name(mod, i, buf, 32);

		if (~sn[i] & 0x80) {
			int looplen;

			mod->xxs[i].lps = hio_read24l(f) << 1;
			looplen = hio_read24l(f) << 1;
			if (looplen > 2)
				mod->xxs[i].flg |= XMP_SAMPLE_LOOP;
			mod->xxs[i].lpe = mod->xxs[i].lps + looplen;
			mod->xxi[i].sub[0].vol = hio_read8(f);
			mod->xxi[i].sub[0].pan = 0x80;
			/* finetune adjusted comparing DSym and S3M versions
			 * of "inside out" */
			mod->xxi[i].sub[0].fin = (int8)(hio_read8(f) << 4);
			mod->xxi[i].sub[0].sid = i;
		}

		D_(D_INFO "[%2X] %-22.22s %05x %05x %05x %c V%02x %+03d",
				i, mod->xxi[i].name, mod->xxs[i].len,
				mod->xxs[i].lps, mod->xxs[i].lpe,
				mod->xxs[i].flg & XMP_SAMPLE_LOOP ? 'L' : ' ',
				mod->xxi[i].sub[0].vol, mod->xxi[i].sub[0].fin);

		if (sn[i] & 0x80 || mod->xxs[i].len == 0)
			continue;

		a = hio_read8(f);

		if (a != 0 && a != 1) {
			D_(D_WARN "libxmp: unsupported sample type %d\n", a);
			//return -1;
		}

		if (a == 1) {
			uint8 *b = malloc(mod->xxs[i].len);
			libxmp_read_lzw_dynamic(f->handle.file, b, 13, 0,
					mod->xxs[i].len, mod->xxs[i].len,
					XMP_LZW_QUIRK_DSYM);
			ret = libxmp_load_sample(m, NULL,
					SAMPLE_FLAG_NOLOAD | SAMPLE_FLAG_DIFF,
					&mod->xxs[i], (char*)b);
			free(b);
		/*} else if (a == 4) {
			ret = libxmp_load_sample(m, f, SAMPLE_FLAG_VIDC,
					&mod->xxs[i], NULL);*/
		} else {
			ret = libxmp_load_sample(m, f, SAMPLE_FLAG_VIDC,
					&mod->xxs[i], NULL);
		}

		if (ret < 0)
			return -1;
	}

	for (i = 0; i < mod->chn; i++) {
		mod->xxc[i].pan = DEFPAN((((i + 3) / 2) % 2) * 0xff);
	}

	return 0;
}
