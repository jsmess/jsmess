#include <string.h>
#include "imgtoolx.h"

typedef struct {
	imgtool_image base;
	imgtool_stream *f;
	int basepos;
	int length;
	int curpos;

	int channels;
	int resolution;
	int frequency;
	INT16 lastsample;
} waveimage;

typedef struct {
	imgtool_directory base;
	waveimage *wimg;
	int pos;
} waveimageenum;

static int find_wavtag(imgtool_stream *f, int filelen, const char *tag, int *offset, UINT32 *blocklen)
{
	char buf[4];

	while(1) {
		*offset += stream_read(f, buf, sizeof(buf));
		*offset += stream_read(f, blocklen, sizeof(*blocklen));
		*blocklen = LITTLE_ENDIANIZE_INT32(*blocklen);

		if (!memcmp(buf, tag, 4))
			break;

		stream_seek(f, *blocklen, SEEK_CUR);
		*offset += *blocklen;

		if (*offset >= filelen)
			return IMGTOOLERR_CORRUPTIMAGE;
	}
	return 0;
}

int imgwave_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
{
	int err;
	waveimage *wimg;
	struct WaveExtra *extra;
	char buf[4];
	UINT16 format, channels, resolution;
	UINT32 filelen, blocklen, frequency;
	int offset = 0;

	offset += stream_read(f, buf, sizeof(buf));
	if (offset < 4)
		goto initalt;
	if (memcmp(buf, "RIFF", 4))
		goto initalt;

	offset += stream_read(f, &filelen, sizeof(filelen));
	if (offset < 8)
		goto initalt;
	filelen = LITTLE_ENDIANIZE_INT32(filelen);

	offset += stream_read(f, buf, sizeof(buf));
	if (offset < 12)
		goto initalt;
	if (memcmp(buf, "WAVE", 4))
		goto initalt;

	err = find_wavtag(f, filelen, "fmt ", &offset, &blocklen);
	if (err)
		return err;

	offset += stream_read(f, &format, sizeof(format));
	format = LITTLE_ENDIANIZE_INT16(format);
	if (format != 1)
		return IMGTOOLERR_CORRUPTIMAGE;

	offset += stream_read(f, &channels, sizeof(channels));
	channels = LITTLE_ENDIANIZE_INT16(channels);
	if (channels != 1 && channels != 2)
		return IMGTOOLERR_CORRUPTIMAGE;

	offset += stream_read(f, &frequency, sizeof(frequency));
	frequency = LITTLE_ENDIANIZE_INT32(frequency);

	stream_seek(f, 6, SEEK_CUR);

	offset += stream_read(f, &resolution, sizeof(resolution));
	resolution = LITTLE_ENDIANIZE_INT16(resolution);
	if ((resolution != 8) && (resolution != 16))
		return IMGTOOLERR_CORRUPTIMAGE;

	stream_seek(f, blocklen - 16, SEEK_CUR);

	err = find_wavtag(f, filelen, "data", &offset, &blocklen);
	if (err)
		return err;

	wimg = malloc(sizeof(waveimage));
	if (!wimg)
		return IMGTOOLERR_OUTOFMEMORY;

	wimg->base.module = mod;
	wimg->f = f;
	wimg->basepos = wimg->curpos = offset;
	wimg->length = blocklen;
	wimg->channels = channels;
	wimg->frequency = frequency;
	wimg->resolution = resolution;
	wimg->lastsample = 0;
	*outimg = &wimg->base;
	return 0;

initalt:
	extra = (struct WaveExtra *) mod->extra;
	if (!extra->initalt)
		return IMGTOOLERR_CORRUPTIMAGE;

	stream_seek(f, 0, SEEK_SET);

	wimg = malloc(sizeof(waveimage));
	if (!wimg)
		return IMGTOOLERR_OUTOFMEMORY;


	err = extra->initalt(f, &wimg->f, &wimg->basepos, &wimg->length,
		&wimg->channels, &wimg->frequency, &wimg->resolution);
	if (err) {
		free(wimg);
		return err;
	}

	wimg->base.module = mod;
	wimg->curpos = wimg->basepos;
	wimg->lastsample = 0;
	stream_seek(wimg->f, wimg->basepos, SEEK_SET);
	*outimg = &wimg->base;

	if (wimg->f != f)
		stream_close(f);

	return 0;
}

void imgwave_exit(imgtool_image *img)
{
	waveimage *wimg = (waveimage *) img;
	stream_close(wimg->f);
	free(wimg);
}

int imgwave_seek(imgtool_image *img, int pos)
{
	waveimage *wimg = (waveimage *) img;

	if (wimg->curpos != pos) {
		stream_seek(wimg->f, pos, SEEK_SET);
		wimg->curpos = pos;
	}
	return 0;
}

static int imgwave_readsample(imgtool_image *img, INT16 *sample)
{
	int len;
	INT16 s = 0;
	waveimage *wimg = (waveimage *) img;

	union {
		UINT8 buf8[2];
		INT16 buf16[2];
	} u;

	if (wimg->curpos >= wimg->basepos + wimg->length) {
		*sample = 0;
		return IMGTOOLERR_INPUTPASTEND;
	}

	if (wimg->resolution == 8) {
		len = stream_read(wimg->f, u.buf8, wimg->channels);
		wimg->curpos += len;
		if (len < wimg->channels)
			return IMGTOOLERR_CORRUPTIMAGE;

		switch(wimg->channels) {
		case 1:
			s = ((INT16) u.buf8[0]) - 0x80;
			s *= 0x100;
			break;
		case 2:
			s = ((INT16) u.buf8[0]) - 0x80;
			s += ((INT16) u.buf8[1]) - 0x80;
			s *= 0x80;
			break;
		}
	}
	else {
		len = stream_read(wimg->f, u.buf16, wimg->channels * 2);
		wimg->curpos += len;
		if (len < (wimg->channels * 2))
			return IMGTOOLERR_CORRUPTIMAGE;

		switch(wimg->channels) {
		case 1:
			s = u.buf16[0];
			break;
		case 2:
			s = (u.buf16[0] / 2) + (u.buf16[1] / 2);
			break;
		}
	}
	*sample = s;
	return 0;
}

static int imgwave_readtransition(imgtool_image *img, int *frequency)
{
	int err;
	int count = 0;
	int transitioned;
	INT16 sample;
	waveimage *wimg;

	wimg = (waveimage *) img;

	do {
		err = imgwave_readsample(img, &sample);
		if (err)
			return err;

		/* Problem - I should be checking for both types of transitions, somehow */
#if 0
		transitioned = (wimg->lastsample >= 0) && (sample < 0);
#else
		transitioned = (wimg->lastsample < 0) && (sample >= 0);
#endif
		wimg->lastsample = sample;
		count++;
	}
	while(!transitioned);

	*frequency = wimg->frequency / count;
	return 0;
}

static int imgwave_readbit(imgtool_image *img, UINT8 *bit)
{
	int err, freq;
	waveimage *wimg;
	struct WaveExtra *extra;

	wimg = (waveimage *) img;
	extra = (struct WaveExtra *) wimg->base.module->extra;

	err = imgwave_readtransition(img, &freq);
	if (err)
		return err;

	if (extra->zeropulse > extra->onepulse)
		*bit = extra->threshpulse > freq;
	else
		*bit = extra->threshpulse <= freq;
	return 0;
}

int imgwave_forward(imgtool_image *img)
{
	int err;
	int i;
	UINT8 carry, newcarry;
	UINT8 *buffer;
	waveimage *wimg;
	struct WaveExtra *extra;

	wimg = (waveimage *) img;
	extra = (struct WaveExtra *) wimg->base.module->extra;

	buffer = malloc(extra->blockheadersize);
	if (!buffer)
		return IMGTOOLERR_OUTOFMEMORY;

	memset(buffer, 0, extra->blockheadersize);

	do {
		err = imgwave_readbit(img, &carry);
		if (err) {
			free(buffer);
			return err;
		}

		for (i = extra->blockheadersize-1; i >= 0; i--) {
			if (extra->waveflags & WAVEIMAGE_MSB_FIRST) {
				/* Most significant bit first */
				newcarry = buffer[i] & 0x80;
				buffer[i] <<= 1;
				if (carry)
					buffer[i] |= 1;
			}
			else {
				/* Least significant bit first */
				newcarry = buffer[i] & 0x01;
				buffer[i] >>= 1;
				if (carry)
					buffer[i] |= 0x80;
			}
			carry = newcarry;
		}
	}
	while(memcmp(buffer, extra->blockheader, extra->blockheadersize));

	free(buffer);
	return 0;
}

int imgwave_read(imgtool_image *img, UINT8 *buf, int bufsize)
{
	int err;
	int i, j;
	UINT8 b, bit;
	waveimage *wimg;
	struct WaveExtra *extra;

	wimg = (waveimage *) img;
	extra = (struct WaveExtra *) wimg->base.module->extra;

	for (i = 0; i < bufsize; i++) {
		b = 0;
		for (j = 0; j < 8; j++) {
			err = imgwave_readbit(img, &bit);
			if (err)
				return err;
			if (extra->waveflags & WAVEIMAGE_MSB_FIRST) {
				/* Most significant bit first */
				b <<= 1;
				if (bit)
					b |= 1;
			}
			else {
				/* Least significant bit first */
				b >>= 1;
				if (bit)
					b |= 0x80;
			}
		}
		*(buf++) = b;
	}
	return 0;
}

int imgwave_beginenum(imgtool_image *img, imgtool_directory **outenum)
{
	/* TODO - Enumerating wave images should be cached */

	waveimage *wimg = (waveimage *) img;
	waveimageenum *wenum;

	wenum = malloc(sizeof(waveimageenum));
	if (!wenum)
		return IMGTOOLERR_OUTOFMEMORY;

	wenum->base.module = img->module;
	wenum->wimg = wimg;
	wenum->pos = wimg->basepos;
	*outenum = (imgtool_directory *) wenum;
	return 0;
}

int imgwave_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
{
	int err;
	waveimageenum *wenum;

	wenum = (waveimageenum *) enumeration;

	err = imgwave_seek((imgtool_image *) wenum->wimg, wenum->pos);
	if (err)
		return err;

	err = ((struct WaveExtra *) wenum->wimg->base.module->extra)->nextfile((imgtool_image *) wenum->wimg, ent);
	if (err)
		return err;

	wenum->pos = wenum->wimg->curpos;
	return 0;
}

void imgwave_closeenum(imgtool_directory *enumeration)
{
	waveimageenum *wenum = (waveimageenum *) enumeration;
	free(wenum);
}

int imgwave_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
{
	int err, pos;
	imgtool_dirent ent;
	imgtool_directory *enumeration;
	waveimageenum *wenum;
	waveimage *wimg;
	struct WaveExtra *extra;
	char fnamebuf[256];

	wimg = (waveimage *) img;
	extra = (struct WaveExtra *) wimg->base.module->extra;

	err = imgwave_beginenum(img, &enumeration);
	if (err)
		return err;
	wenum = (waveimageenum *) enumeration;

	memset(&ent, 0, sizeof(ent));
	ent.fname = fnamebuf;
	ent.fname_len = sizeof(fnamebuf);

	do {
		pos = wenum->pos;
		err = imgwave_nextenum(enumeration, &ent);
		if (ent.eof)
			err = IMGTOOLERR_FILENOTFOUND;
		if (err) {
			imgwave_closeenum(enumeration);
			return err;
		}
	}
	while(strcmp(ent.fname, fname));
	
	imgwave_closeenum(enumeration);
	enumeration = NULL;

	err = imgwave_seek(img, pos);
	if (err)
		return err;

	err = extra->readfile(img, destf);
	if (err)
		return err;

	return 0;
}


