/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* Added support for Tiled TIFF output
* Luca Cristelli (luca.cristelli@ies.it) 1/2001
*
* This r.tiff version uses the standard libtiff from your system.
* 8. June 98 Marco Valagussa <marco@duffy.crcc.it>
*
* Original version:
* Portions Copyright (c) 1988, 1990 by Sam Leffler.
* All rights reserved.
*
* This file is provided for unrestricted use provided that this
* legend is included on all tape media and as a part of the
* software program in whole or part. Users may copy, modify or
* distribute this file at will.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <zlib.h>
#include <tiffio.h>

#define XTILESIZ 64
#define YTILESIZ 64
#define TILESIZ XTILESIZ*YTILESIZ

unsigned char tilebuf[TILESIZ], obuf[TILESIZ];
static z_stream zstream;

static int decompress (unsigned char *buf, int len, unsigned char *outbuf,
int outbuf_len);
static void *mapfile (const char *name, size_t * len, FILE ** stream);

#pragma pack(1)
typedef struct
{
int32_t width;
int32_t height;
int16_t xtiles;
int16_t ytiles;
int32_t palette[256];
int32_t offset;
} ozf2_imgEntry;

int
main (int argc, char *argv[])
{

void *scn;

size_t binlen;
FILE *fpbin;
char *binnam;

int *dir;
int *ioff;

ozf2_imgEntry *curr_img;

TIFF *out;
int red, grn, blu;

if (argc != 3)
{
fprintf (stderr, "Usage: %s file.ozf2 file.tif\n", argv[0]);
exit (1);
}

binnam = argv[1];

scn = (void *) mapfile (binnam, &binlen, &fpbin);

dir = (int *) ((off_t) scn + binlen - sizeof (int));
ioff = (int *) ((off_t) scn + (*dir));

curr_img = (ozf2_imgEntry *) ((off_t) scn + ioff[0]);

int ipix, irow, icol, irowtile, icoltile;

out = TIFFOpen (argv[2], "w");
if (!out)
{
fprintf (stderr, "cannot open %s\n", argv[2]);
exit (1);
}

TIFFSetField (out, TIFFTAG_IMAGEWIDTH, curr_img->xtiles * XTILESIZ);
TIFFSetField (out, TIFFTAG_IMAGELENGTH, curr_img->ytiles * YTILESIZ);
TIFFSetField (out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, 1);
TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, 8);
TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
TIFFSetField (out, TIFFTAG_TILEWIDTH, XTILESIZ);
TIFFSetField (out, TIFFTAG_TILELENGTH, YTILESIZ);

/* save the palette */

u_int16_t redp[256], grnp[256], blup[256];
int i;

#define SCALE(x) (((x)*((1L<<16)-1))/255)

for (i = 0; i < 256; i++)
{
unsigned int colour = curr_img->palette[i];
red = ((colour & 0x00ff0000) >> 16) & 0xff;
grn = ((colour & 0x0000ff00) >> 8) & 0xff;
blu = ((colour & 0x000000ff) >> 0) & 0xff;

redp[i] = (u_int16_t) (SCALE (red));
grnp[i] = (u_int16_t) (SCALE (grn));
blup[i] = (u_int16_t) (SCALE (blu));
}

TIFFSetField (out, TIFFTAG_COLORMAP, redp, grnp, blup);
TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
TIFFSetField (out, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

int *offti, itile;
offti = (int *) ((off_t) scn + ioff[0] + sizeof (ozf2_imgEntry) - 4);

for (irowtile = 0; irowtile < curr_img->ytiles; irowtile++)
for (icoltile = 0; icoltile < curr_img->xtiles; icoltile++)
{
unsigned char *compcoff;
int zres;

itile = irowtile * curr_img->xtiles + icoltile;
compcoff = (unsigned char *) ((off_t) scn + offti[itile]);

inflateInit (&zstream);
zres =
decompress (compcoff, offti[itile + 1] - offti[itile], tilebuf,
TILESIZ);
inflateEnd (&zstream);

if (zres != TILESIZ)
{
fprintf (stderr, "ozf2 compression bug: zres=%d != %d\n", zres,
TILESIZ);
exit (1);
}

/* ozf2 tiles are mirrored */
for (irow = 0; irow < YTILESIZ; irow++)
for (icol = 0; icol < XTILESIZ; icol++)
{
ipix = (YTILESIZ - 1 - irow) * XTILESIZ + icol;
obuf[irow * YTILESIZ + icol] = tilebuf[ipix];
}

if (TIFFWriteTile (out, obuf, icoltile * XTILESIZ,
irowtile * YTILESIZ, 0, 0) < 0)
{
fprintf (stderr, "write failed: irowtile= %5d icoltile= %5d\n",
irowtile, icoltile);
exit (1);
}
}

(void) TIFFClose (out);

exit (0);
}

static int
decompress (unsigned char *buf, int len, unsigned char *outbuf,
int outbuf_len)
{
int err;

zstream.next_in = buf;
zstream.avail_in = len;

zstream.next_out = outbuf;
zstream.avail_out = outbuf_len;

inflateReset (&zstream);

err = inflate (&zstream, Z_FINISH);
if (err != Z_STREAM_END)
{
fprintf (stderr, "decompression error %p(%d): %s\n",
buf, len, zError (err));
return -1;
}
return zstream.total_out;
}

void *
mapfile (const char *name, size_t * len, FILE ** stream)
{
void *ptr;
int fd;
FILE *fp = NULL;
struct stat sbuf;


if (access (name, F_OK) == 0)
{
if (access (name, W_OK) == 0)
fp = fopen (name, "r+");
else
{
fp = fopen (name, "r");
}
}
else
{
fprintf (stderr, "no such file %s?\n", name);
exit (1);
}

if (!fp)
{
fprintf (stderr, "no such file %s?\n", name);
exit (1);
}

fd = fileno (fp);
fstat (fd, &sbuf);
*len = sbuf.st_size;

ptr = mmap (NULL, *len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, (off_t) 0);
if (ptr == MAP_FAILED)
{
fprintf (stderr, "FATAL: can't mmap file=%s len=%ld\n", name, *len);
exit (1);
}

*stream = fp;
return ptr;
}