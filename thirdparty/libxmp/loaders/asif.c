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
#include "asif.h"

#define MAGIC_FORM	MAGIC4('F','O','R','M')
#define MAGIC_ASIF	MAGIC4('A','S','I','F')
#define MAGIC_NAME	MAGIC4('N','A','M','E')
#define MAGIC_INST	MAGIC4('I','N','S','T')
#define MAGIC_WAVE	MAGIC4('W','A','V','E')

int asif_load(struct module_data *m, HIO_HANDLE *f, int i)
{
	struct xmp_module *mod = &m->mod;
	int size, pos;
	uint32 id;
	int chunk;
	int j;

	if (f == NULL)
		return -1;

	if (hio_read32b(f) != MAGIC_FORM)
		return -1;
	/*size =*/ hio_read32b(f);

	if (hio_read32b(f) != MAGIC_ASIF)
		return -1;

	for (chunk = 0; chunk < 2; ) {
		id = hio_read32b(f);
		size = hio_read32b(f);
		pos = hio_tell(f) + size;

		switch (id) {
		case MAGIC_WAVE:
			//printf("wave chunk\n");
		
			hio_seek(f, hio_read8(f), SEEK_CUR);	/* skip name */
			mod->xxs[i].len = hio_read16l(f) + 1;
			size = hio_read16l(f);		/* NumSamples */
			
			for (j = 0; j < size; j++) {
				hio_read16l(f);		/* Location */
				mod->xxs[j].len = 256 * hio_read16l(f);
				hio_read16l(f);		/* OrigFreq */
				hio_read16l(f);		/* SampRate */
			}
		
			if (libxmp_load_sample(m, f, SAMPLE_FLAG_UNS,
						&mod->xxs[i], NULL) < 0) {
				return -1;
			}

			chunk++;
			break;

		case MAGIC_INST:
			//printf("inst chunk\n");
		
			hio_seek(f, hio_read8(f), SEEK_CUR);	/* skip name */
		
			hio_read16l(f);			/* SampNum */
			hio_seek(f, 24, SEEK_CUR);	/* skip envelope */
			hio_read8(f);			/* ReleaseSegment */
			hio_read8(f);			/* PriorityIncrement */
			hio_read8(f);			/* PitchBendRange */
			hio_read8(f);			/* VibratoDepth */
			hio_read8(f);			/* VibratoSpeed */
			hio_read8(f);			/* UpdateRate */
		
			mod->xxi[i].nsm = 1;
			mod->xxi[i].sub[0].vol = 0x40;
			mod->xxi[i].sub[0].pan = 0x80;
			mod->xxi[i].sub[0].sid = i;

			chunk++;
		}

		hio_seek(f, pos, SEEK_SET);
	}

	return 0;
}
