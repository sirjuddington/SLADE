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
 * Common functions for MMD0/1 and MMD2/3 loaders
 * Tempo fixed by Francis Russell
 */

#include "med.h"
#include "loader.h"
#include "../med_extras.h"

#ifdef DEBUG
const char *const mmd_inst_type[] = {
	"HYB",			/* -2 */
	"SYN",			/* -1 */
	"SMP",			/*  0 */
	"I5O",			/*  1 */
	"I3O",			/*  2 */
	"I2O",			/*  3 */
	"I4O",			/*  4 */
	"I6O",			/*  5 */
	"I7O",			/*  6 */
	"EXT",			/*  7 */
};
#endif

static int mmd_convert_tempo(int tempo, int bpm_on, int med_8ch)
{
	const int tempos_compat[10] = {
		195, 97, 65, 49, 39, 32, 28, 24, 22, 20
	};
	const int tempos_8ch[10] = {
		179, 164, 152, 141, 131, 123, 116, 110, 104, 99
	};

	if (tempo > 0) {
		/* From the OctaMEDv4 documentation:
		 *
		 * In 8-channel mode, you can control the playing speed more
		 * accurately (to techies: by changing the size of the mix buffer).
		 * This can be done with the left tempo gadget (values 1-10; the
		 * lower, the faster). Values 11-240 are equivalent to 10.
		 *
		 * NOTE: the tempos used for this are directly from OctaMED
		 * Soundstudio 2, but in older versions the playback speeds
		 * differed slightly between NTSC and PAL. This table seems to
		 * have been intended to be a compromise between the two.
		 */
		if (med_8ch) {
			tempo = tempo > 10 ? 10 : tempo;
			return tempos_8ch[tempo-1];
		}
		/* Tempos 1-10 in tempo mode are compatibility tempos that
		 * approximate Soundtracker speeds.
		 */
		if (tempo <= 10 && !bpm_on) {
			return tempos_compat[tempo-1];
		}
	}
	return tempo;
}

void mmd_xlat_fx(struct xmp_event *event, int bpm_on, int bpmlen, int med_8ch,
		 int hexvol)
{
	switch (event->fxt) {
	case 0x00:
		/* ARPEGGIO 00
		 * Changes the pitch six times between three different
		 * pitches during the duration of the note. It can create a
		 * chord sound or other special effect. Arpeggio works better
		 * with some instruments than others.
		 */
		break;
	case 0x01:
		/* SLIDE UP 01
		 * This slides the pitch of the current track up. It decreases
		 * the period of the note by the amount of the argument on each
		 * timing pulse. OctaMED-Pro can create slides automatically,
		 * but you may want to use this function for special effects.
		 * Note: a param of 0 does nothing and should be ignored.
		 */
		if (!event->fxp)
			event->fxt = 0;
		break;
	case 0x02:
		/* SLIDE DOWN 02
		 * The same as SLIDE UP, but it slides down.
		 * Note: a param of 0 does nothing and should be ignored.
		 */
		if (!event->fxp)
			event->fxt = 0;
		break;
	case 0x03:
		/* PORTAMENTO 03
		 * Makes precise sliding easy.
		 */
		break;
	case 0x04:
		/* VIBRATO 04
		 * The left half of the argument is the vibrato speed, the
		 * right half is the depth. If the numbers are zeros, the
		 * previous speed and depth are used.
		 */
		/* Note: this is twice as deep as the Protracker vibrato */
		event->fxt = FX_VIBRATO2;
		break;
	case 0x05:
		/* SLIDE + FADE 05
		 * ProTracker compatible. This command is the combination of
		 * commands 0300 and 0Dxx. The argument is the fade speed.
		 * The slide will continue during this command.
		 */
		/* fall-through */
	case 0x06:
		/* VIBRATO + FADE 06
		 * ProTracker compatible. Combines commands 0400 and 0Dxx.
		 * The argument is the fade speed. The vibrato will continue
		 * during this command.
		 */
		/* fall-through */
	case 0x07:
		/* TREMOLO 07
		 * ProTracker compatible.
		 * This command is a kind of "volume vibrato". The left
		 * number is the speed of the tremolo, and the right one is
		 * the depth. The depth must be quite high before the effect
		 * is audible.
		 */
		break;
	case 0x08:
		/* HOLD and DECAY 08
		 * This command must be on the same line as the note. The
		 * left half of the argument determines the decay and the
		 * right half the hold.
		 */
		event->fxt = event->fxp = 0;
		break;
	case 0x09:
		/* SECONDARY TEMPO 09
		 * This sets the secondary tempo (the number of timing
		 * pulses per note). The argument must be from 01 to 20 (hex).
		 */
		if (event->fxp >= 0x01 && event->fxp <= 0x20) {
			event->fxt = FX_SPEED;
		} else {
			event->fxt = event->fxp = 0;
		}
		break;
	case 0x0a:
		/* 0A not mentioned but it's Protracker-compatible */
		/* fall-through */
	case 0x0b:
		/* POSITION JUMP 0B
		 * The song plays up to this command and then jumps to
		 * another position in the play sequence. The song then
		 * loops from the point jumped to, to the end of the song
		 * forever. The purpose is to allow for introductions that
		 * play only once.
		 */
		/* fall-through */
	case 0x0c:
		/* SET VOLUME 0C
		 * Overrides the default volume of an instrument.
		 */
		if (!hexvol) {
			int p = event->fxp;
			event->fxp = (p >> 8) * 10 + (p & 0xff);
		}
		break;
	case 0x0d:
		/* VOLUME SLIDE 0D
		 * Smoothly slides the volume up or down. The left half of
		 * the argument increases the volume. The right decreases it.
		 */
		event->fxt = FX_VOLSLIDE;
		break;
	case 0x0e:
		/* SYNTH JUMP 0E
		 * When used with synthetic or hybrid instruments, it
		 * triggers a jump in the Waveform Sequence List. The argument
		 * is the jump destination (line no).
		 */
		event->fxt = event->fxp = 0;
		break;
	case 0x0f:
		/* MISCELLANEOUS 0F
		 * The effect depends upon the value of the argument.
		 */
		if (event->fxp == 0x00) {	/* Jump to next block */
			event->fxt = 0x0d;
			break;
		} else if (event->fxp <= 0xf0) {
			event->fxt = FX_S3M_BPM;
			event->fxp = mmd_convert_tempo(event->fxp, bpm_on, med_8ch);
			break;
		} else switch (event->fxp) {
		case 0xf1:	/* Play note twice */
			event->fxt = FX_EXTENDED;
			event->fxp = (EX_RETRIG << 4) | 3;
			break;
		case 0xf2:	/* Delay note */
			event->fxt = FX_EXTENDED;
			event->fxp = (EX_DELAY << 4) | 3;
			break;
		case 0xf3:	/* Play note three times */
			event->fxt = FX_EXTENDED;
			event->fxp = (EX_RETRIG << 4) | 2;
			break;
		case 0xf8:	/* Turn filter off */
		case 0xf9:	/* Turn filter on */
		case 0xfa:	/* MIDI pedal on */
		case 0xfb:	/* MIDI pedal off */
		case 0xfd:	/* Set pitch */
		case 0xfe:	/* End of song */
			event->fxt = event->fxp = 0;
			break;
		case 0xff:	/* Turn note off */
			event->fxt = event->fxp = 0;
			event->note = XMP_KEY_CUT;
			break;
		default:
			event->fxt = event->fxp = 0;
		}
		break;
	case 0x11:
		/* SLIDE PITCH UP (only once) 11
		 * Equivalent to ProTracker command E1x.
		 * Lets you control the pitch with great accuracy. This
		 * command changes only this occurrence of the note.
		 * Note: a param of 0 does nothing and should be ignored.
		 */
		event->fxt = FX_F_PORTA_UP;
		break;
	case 0x12:
		/* SLIDE DOWN (only once) 12
		 * Equivalent to ProTracker command E2x.
		 * Note: a param of 0 does nothing and should be ignored.
		 */
		event->fxt = FX_F_PORTA_DN;
		break;
	case 0x14:
		/* VIBRATO 14
		 * ProTracker compatible. This is similar to command 04
		 * except the depth is halved, to give greater accuracy.
		 */
		event->fxt = FX_VIBRATO;
		break;
	case 0x15:
		/* SET FINETUNE 15
		 * Set a finetune value for a note, overrides the default
		 * fine tune value of the instrument. Negative numbers must
		 * be entered as follows:
		 *   -1 => FF    -3 => FD    -5 => FB    -7 => F9
		 *   -2 => FE    -4 => FC    -6 => FA    -8 => F8
		 */
		event->fxt = FX_FINETUNE;
		event->fxp = (event->fxp + 8) << 4;
		break;
	case 0x16:
		/* LOOP 16
		 * Creates a loop within a block. 1600 marks the beginning
		 * of the loop.  The next occurrence of the 16 command
		 * designates the number of loops. Same as ProTracker E6x.
		 */
		event->fxt = FX_EXTENDED;
		if (event->fxp > 0x0f)
			event->fxp = 0x0f;
		event->fxp |= 0x60;
		break;
	case 0x18:
		/* STOP NOTE 18
		 * Cuts the note by zeroing the volume at the pulse specified
		 * in the argument value. This is the same as ProTracker
		 * command ECx.
		 */
		event->fxt = FX_EXTENDED;
		if (event->fxp > 0x0f)
			event->fxp = 0x0f;
		event->fxp |= 0xc0;
		break;
	case 0x19:
		/* SET SAMPLE START OFFSET
		 * Same as ProTracker command 9.
		 * When playing a sample, this command sets the starting
		 * offset (at steps of $100 = 256 bytes). Useful for speech
		 * samples.
		 */
		event->fxt = FX_OFFSET;
		break;
	case 0x1a:
		/* SLIDE VOLUME UP ONCE
		 * Only once ProTracker command EAx. Lets volume slide
		 * slowly once per line.
		 * Note: a param of 0 does nothing and should be ignored.
		 */
		event->fxt = event->fxp ? FX_F_VSLIDE_UP : 0;
		break;
	case 0x1b:
		/* SLIDE VOLUME DOWN ONCE
		 * Only once ProTracker command EBx.
		 * Note: a param of 0 does nothing and should be ignored.
		 */
		event->fxt = event->fxp ? FX_F_VSLIDE_DN : 0;
		break;
	case 0x1d:
		/* JUMP TO NEXT BLOCK 1D
		 * Jumps to the next line in the PLAY SEQUENCE LIST at the
		 * specified line. ProTracker command D. This command is
		 * like F00, except that you can specify the line number of
		 * the first line to be played. The line number must be
		 * specified in HEX.
		 */
		event->fxt = FX_BREAK;
		break;
	case 0x1e:
		/* PLAY LINE x TIMES 1E
		 * Plays only commands, notes not replayed. ProTracker
		 * pattern delay.
		 */
		event->fxt = FX_PATT_DELAY;
		break;
	case 0x1f:
		/* Command 1F: NOTE DELAY AND RETRIGGER
		 * (Protracker commands EC and ED)
		 * Gives you accurate control over note playing. You can
		 * delay the note any number of ticks, and initiate fast
		 * retrigger. Level 1 = note delay value, level 2 = retrigger
		 * value.
		 */
		if (MSN(event->fxp)) {
			/* delay */
			event->fxt = FX_EXTENDED;
			event->fxp = 0xd0 | (event->fxp >> 4);
		} else if (LSN(event->fxp)) {
			/* retrig */
			event->fxt = FX_EXTENDED;
			event->fxp = 0x90 | (event->fxp & 0x0f);
		}
		break;
	case 0x2e:
		/* Command 2E: SET TRACK PANNING
		 * Allows track panning to be changed during play. The track
		 * on which the player command appears is the track affected.
		 * The command level is in signed hex: $F0 to $10 = -16 to 16
		 * decimal.
		 */
		if (event->fxp >= 0xf0 || event->fxp <= 0x10) {
			int fxp = (signed char)event->fxp + 16;
			fxp <<= 3;
			if (fxp == 0x100)
				fxp--;
			event->fxt = FX_SETPAN;
			event->fxp = fxp;
		}
		break;
	default:
		event->fxt = event->fxp = 0;
		break;
	}
}


int mmd_alloc_tables(struct module_data *m, int i, struct SynthInstr *synth)
{
	struct med_module_extras *me = (struct med_module_extras *)m->extra;

	me->vol_table[i] = calloc(1, synth->voltbllen);
	if (me->vol_table[i] == NULL)
		goto err;
	memcpy(me->vol_table[i], synth->voltbl, synth->voltbllen);

	me->wav_table[i] = calloc(1, synth->wftbllen);
	if (me->wav_table[i] == NULL)
		goto err1;
	memcpy(me->wav_table[i], synth->wftbl, synth->wftbllen);

	return 0;

    err1:
	free(me->vol_table[i]);
    err:
	return -1;
}

int mmd_load_hybrid_instrument(HIO_HANDLE *f, struct module_data *m, int i,
			int smp_idx, struct SynthInstr *synth,
			struct InstrExt *exp_smp, struct MMD0sample *sample)
{
	struct xmp_module *mod = &m->mod;
	struct xmp_instrument *xxi = &mod->xxi[i];
	struct xmp_subinstrument *sub;
	struct xmp_sample *xxs;
	int length, type;
	int pos = hio_tell(f);

	/* Sanity check */
	if (smp_idx >= mod->smp) {
		return -1;
	}

	synth->defaultdecay = hio_read8(f);
	hio_seek(f, 3, SEEK_CUR);
	synth->rep = hio_read16b(f);
	synth->replen = hio_read16b(f);
	synth->voltbllen = hio_read16b(f);
	synth->wftbllen = hio_read16b(f);
	synth->volspeed = hio_read8(f);
	synth->wfspeed = hio_read8(f);
	synth->wforms = hio_read16b(f);
	hio_read(synth->voltbl, 1, 128, f);;
	hio_read(synth->wftbl, 1, 128, f);;

	/* Sanity check */
	if (synth->voltbllen > 128 || synth->wftbllen > 128) {
		return -1;
	}

	hio_seek(f, pos - 6 + hio_read32b(f), SEEK_SET);
	length = hio_read32b(f);
	type = hio_read16b(f);

	if (libxmp_med_new_instrument_extras(xxi) != 0)
		return -1;

	xxi->nsm = 1;
	if (libxmp_alloc_subinstrument(mod, i, 1) < 0)
		return -1;

	MED_INSTRUMENT_EXTRAS((*xxi))->vts = synth->volspeed;
	MED_INSTRUMENT_EXTRAS((*xxi))->wts = synth->wfspeed;

	sub = &xxi->sub[0];

	sub->pan = 0x80;
	sub->vol = sample->svol;
	sub->xpo = sample->strans + 36;
	sub->sid = smp_idx;
	sub->fin = exp_smp->finetune;

	xxs = &mod->xxs[smp_idx];

	xxs->len = length;
	xxs->lps = 2 * sample->rep;
	xxs->lpe = xxs->lps + 2 * sample->replen;
	xxs->flg = sample->replen > 1 ?  XMP_SAMPLE_LOOP : 0;

	if (libxmp_load_sample(m, f, 0, xxs, NULL) < 0)
		return -1;

	return 0;
}

int mmd_load_synth_instrument(HIO_HANDLE *f, struct module_data *m, int i,
			int smp_idx, struct SynthInstr *synth,
			struct InstrExt *exp_smp, struct MMD0sample *sample)
{
	struct xmp_module *mod = &m->mod;
	struct xmp_instrument *xxi = &mod->xxi[i];
	int pos = hio_tell(f);
	int j;

	synth->defaultdecay = hio_read8(f);
	hio_seek(f, 3, SEEK_CUR);
	synth->rep = hio_read16b(f);
	synth->replen = hio_read16b(f);
	synth->voltbllen = hio_read16b(f);
	synth->wftbllen = hio_read16b(f);
	synth->volspeed = hio_read8(f);
	synth->wfspeed = hio_read8(f);
	synth->wforms = hio_read16b(f);
	hio_read(synth->voltbl, 1, 128, f);;
	hio_read(synth->wftbl, 1, 128, f);;
	for (j = 0; j < 64; j++)
		synth->wf[j] = hio_read32b(f);

	/* Sanity check */
	if (synth->voltbllen > 128 || synth->wftbllen > 128 || synth->wforms > 256) {
		return -1;
	}

	D_(D_INFO "  VS:%02x WS:%02x WF:%02x %02x %+3d %+1d",
			synth->volspeed, synth->wfspeed,
			synth->wforms & 0xff,
			sample->svol,
			sample->strans,
			exp_smp->finetune);

	if (synth->wforms == 0xffff) {
		xxi->nsm = 0;
		return 1;
	}

	if (synth->wforms > 64)
		return -1;

	if (libxmp_med_new_instrument_extras(&mod->xxi[i]) != 0)
		return -1;

	mod->xxi[i].nsm = synth->wforms;
	if (libxmp_alloc_subinstrument(mod, i, synth->wforms) < 0)
		return -1;

	MED_INSTRUMENT_EXTRAS((*xxi))->vts = synth->volspeed;
	MED_INSTRUMENT_EXTRAS((*xxi))->wts = synth->wfspeed;

	for (j = 0; j < synth->wforms; j++) {
		struct xmp_subinstrument *sub = &xxi->sub[j];
		struct xmp_sample *xxs = &mod->xxs[smp_idx];

		/* Sanity check */
		if (j >= xxi->nsm || smp_idx >= mod->smp)
			return -1;

		sub->pan = 0x80;
		sub->vol = 64;
		sub->xpo = 12 + sample->strans;
		sub->sid = smp_idx;
		sub->fin = exp_smp->finetune;

		hio_seek(f, pos - 6 + synth->wf[j], SEEK_SET);

		xxs->len = hio_read16b(f) * 2;
		xxs->lps = 0;
		xxs->lpe = mod->xxs[smp_idx].len;
		xxs->flg = XMP_SAMPLE_LOOP;

		if (libxmp_load_sample(m, f, 0, xxs, NULL) < 0)
			return -1;

		smp_idx++;
	}

	return 0;
}

int mmd_load_sampled_instrument(HIO_HANDLE *f, struct module_data *m, int i,
			int smp_idx, struct InstrHdr *instr,
			struct MMD0exp *expdata, struct InstrExt *exp_smp,
			struct MMD0sample *sample, int ver)
{
	struct xmp_module *mod = &m->mod;
	struct xmp_instrument *xxi = &mod->xxi[i];
	struct xmp_subinstrument *sub;
	struct xmp_sample *xxs;
	int j, k;

	/* Sanity check */
	if (smp_idx >= mod->smp)
		return -1;

	/* hold & decay support */
        if (libxmp_med_new_instrument_extras(xxi) != 0)
                return -1;
	MED_INSTRUMENT_EXTRAS(*xxi)->hold = exp_smp->hold;
	xxi->rls = 0xfff - (exp_smp->decay << 4);

	xxi->nsm = 1;
	if (libxmp_alloc_subinstrument(mod, i, 1) < 0)
		return -1;

	sub = &xxi->sub[0];

	sub->vol = sample->svol;
	sub->pan = 0x80;
	sub->xpo = sample->strans + 36;
	if (ver >= 2 && expdata->s_ext_entrsz > 4) {	/* MMD2+ */
		if (exp_smp->default_pitch) {
			sub->xpo += exp_smp->default_pitch - 25;
		}
	}
	sub->sid = smp_idx;
	sub->fin = exp_smp->finetune << 4;

	xxs = &mod->xxs[smp_idx];

	xxs->len = instr->length;
	xxs->lps = 2 * sample->rep;
	xxs->lpe = xxs->lps + 2 * sample->replen;
	xxs->flg = 0;

	if (sample->replen > 1) {
		xxs->flg |= XMP_SAMPLE_LOOP;
	}

	if (instr->type & S_16) {
		xxs->flg |= XMP_SAMPLE_16BIT;
		xxs->len >>= 1;
		xxs->lps >>= 1;
		xxs->lpe >>= 1;
	}

	/* STEREO means that this is a stereo sample. The sample
	 * is not interleaved. The left channel comes first,
	 * followed by the right channel. Important: Length
	 * specifies the size of one channel only! The actual memory
	 * usage for both samples is length * 2 bytes.
	 */

        /* Restrict sampled instruments to 3 octave range except for MMD3.
         * Checked in MMD0 with med.egypian/med.medieval from Lemmings 2
         * and MED.ParasolStars, MMD1 with med.Lemmings2
         */

	if (ver < 3) {
		for (j = 0; j < 9; j++) {
			for (k = 0; k < 12; k++) {
				int xpo = 0;

				if (j < 1)
					xpo = 12 * (1 - j);
				else if (j > 3)
					xpo = -12 * (j - 3);

				xxi->map[12 * j + k].xpo = xpo;
			}
		}
	}


	if (libxmp_load_sample(m, f, SAMPLE_FLAG_BIGEND, xxs, NULL) < 0) {
		return -1;
	}

	return 0;
}

static const char iffoct_insmap[6][9] = {
	/* 2 */ { 1, 1, 1, 0, 0, 0, 0, 0, 0 },
	/* 3 */ { 2, 2, 2, 2, 2, 2, 1, 1, 0 },
	/* 4 */ { 3, 3, 3, 2, 2, 2, 1, 1, 0 },
	/* 5 */ { 4, 4, 4, 3, 2, 2, 1, 1, 0 },
	/* 6 */ { 5, 5, 5, 5, 4, 3, 2, 1, 0 },
	/* 7 */ { 6, 6, 6, 6, 5, 4, 3, 2, 1 }
};

static const char iffoct_xpomap[6][9] = {
	/* 2 */ { 12, 12, 12,  0,  0,  0,  0,  0,  0 },
	/* 3 */ { 12, 12, 12, 12, 12, 12,  0,  0,-12 },
	/* 4 */ { 12, 12, 12,  0,  0,  0,-12,-12,-24 },
	/* 5 */ { 24, 24, 24, 12,  0,  0,-12,-24,-36 },
	/* 6 */ { 12, 12, 12, 12,  0,-12,-24,-36,-48 },
	/* 7 */ { 12, 12, 12, 12,  0,-12,-24,-36,-48 },
};

int mmd_load_iffoct_instrument(HIO_HANDLE *f, struct module_data *m, int i,
			int smp_idx, struct InstrHdr *instr, int num_oct,
			struct InstrExt *exp_smp, struct MMD0sample *sample)
{
	struct xmp_module *mod = &m->mod;
	struct xmp_instrument *xxi = &mod->xxi[i];
	struct xmp_subinstrument *sub;
	struct xmp_sample *xxs;
	int size, rep, replen, j, k;

	if (num_oct < 2 || num_oct > 7)
		return -1;

	/* Sanity check */
	if (smp_idx + num_oct > mod->smp)
		return -1;

	/* hold & decay support */
	if (libxmp_med_new_instrument_extras(xxi) != 0)
		return -1;

	MED_INSTRUMENT_EXTRAS(*xxi)->hold = exp_smp->hold;
	xxi->rls = 0xfff - (exp_smp->decay << 4);

	xxi->nsm = num_oct;
	if (libxmp_alloc_subinstrument(mod, i, num_oct) < 0)
		return -1;

	/* base octave size */
	size = instr->length / ((1 << num_oct) - 1);
	rep = 2 * sample->rep;
	replen = 2 * sample->replen;

	for (j = 0; j < num_oct; j++) {
		sub = &xxi->sub[j];

		sub->vol = sample->svol;
		sub->pan = 0x80;
		sub->xpo = 24 + sample->strans;
		sub->sid = smp_idx;
		sub->fin = exp_smp->finetune << 4;

		xxs = &mod->xxs[smp_idx];

		xxs->len = size;
		xxs->lps = rep;
		xxs->lpe = rep + replen;
		xxs->flg = 0;

		if (sample->replen > 1) {
			xxs->flg |= XMP_SAMPLE_LOOP;
		}

		if (libxmp_load_sample(m, f, SAMPLE_FLAG_BIGEND, xxs, NULL) < 0) {
			return -1;
		}

		smp_idx++;
		size <<= 1;
		rep <<= 1;
		replen <<= 1;
	}

	/* instrument mapping */

	for (j = 0; j < 9; j++) {
		for (k = 0; k < 12; k++) {
			xxi->map[12 * j + k].ins = iffoct_insmap[num_oct - 2][j];
			xxi->map[12 * j + k].xpo = iffoct_xpomap[num_oct - 2][j];
		}
	}

	return 0;
}


void mmd_set_bpm(struct module_data *m, int med_8ch, int deftempo,
						int bpm_on, int bpmlen)
{
	struct xmp_module *mod = &m->mod;

	mod->bpm = mmd_convert_tempo(deftempo, bpm_on, med_8ch);

	/* 8-channel mode completely overrides regular timing.
	 * See mmd_convert_tempo for more info.
	 */
	if (med_8ch) {
		m->time_factor = DEFAULT_TIME_FACTOR;
	} else if (bpm_on) {
		m->time_factor = DEFAULT_TIME_FACTOR * 4 / bpmlen;
	}
}


void mmd_info_text(HIO_HANDLE *f, struct module_data *m, int offset)
{
	int type, len;

	/* Copy song info text */
	hio_read32b(f);		/* skip next */
	hio_read16b(f);		/* skip reserved */
	type = hio_read16b(f);
	D_(D_INFO "mmdinfo type=%d", type);
	if (type == 1) {	/* 1 = ASCII */
		len = hio_read32b(f);
		D_(D_INFO "mmdinfo length=%d", len);
		if (len > 0 && len < hio_size(f)) {
			m->comment = malloc(len + 1);
			if (m->comment == NULL)
				return;
			hio_read(m->comment, 1, len, f);
			m->comment[len] = 0;
		}
	}
}
