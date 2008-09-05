/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**********************************************************
*      This software is part of the graphviz package      *
*                http://www.graphviz.org/                 *
*                                                         *
*            Copyright (c) 1994-2004 AT&T Corp.           *
*                and is licensed under the                *
*            Common Public License, Version 1.0           *
*                      by AT&T Corp.                      *
*                                                         *
*        Information and Software Systems Research        *
*              AT&T Research, Florham Park NJ             *
**********************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gvplugin_device.h"

#ifdef HAVE_LIBGD
#include "gd.h"

int gvdevice_gd_putBuf (gdIOCtx *context, const void *buffer, int len)
{
    return gvdevice_write((GVJ_t *)(context->tell), buffer, len);
}

/* used by gif output */
void gvdevice_gd_putC (gdIOCtx *context, int C)
{
    char c = C;

    gvdevice_write((GVJ_t *)(context->tell), &c, 1);
}

#ifdef HAVE_PANGOCAIRO
typedef enum {
	FORMAT_GIF,
	FORMAT_JPEG,
	FORMAT_PNG,
	FORMAT_WBMP,
	FORMAT_GD,
	FORMAT_GD2,
	FORMAT_XBM,
} format_type;

static void gd_format(GVJ_t * job)
{
    gdImagePtr im;
    unsigned int x, y, color, alpha;
    unsigned int *data = (unsigned int*)(job->imagedata);
    unsigned int width = job->width;
    unsigned int height = job->height;
    gdIOCtx ctx;

    ctx.putBuf = gvdevice_gd_putBuf;
    ctx.putC = gvdevice_gd_putC;
    ctx.tell = (void*)job;    /* hide *job here */

    im = gdImageCreateTrueColor(width, height);
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
	    color = *data++;
	    /* gd's max alpha is 127 */
	    if ((alpha = (color >> 25) & 0x7f))
	        /* gd's alpha is transparency instead of opacity */
	    	color = (color & 0xffffff) | ((127 - alpha) << 24);
	    else
		color = im->transparent;
	    gdImageSetPixel (im, x, y, color);
	}
    }

    switch (job->device.id) {
#ifdef HAVE_GD_GIF
    case FORMAT_GIF:
	gdImageTrueColorToPalette(im, 0, 256);
	gdImageGifCtx(im, &ctx);
        break;
#endif

#ifdef HAVE_GD_JPEG
    case FORMAT_JPEG:
	/*
	 * Write IM to OUTFILE as a JFIF-formatted JPEG image, using
	 * quality JPEG_QUALITY.  If JPEG_QUALITY is in the range
	 * 0-100, increasing values represent higher quality but also
	 * larger image size.  If JPEG_QUALITY is negative, the
	 * IJG JPEG library's default quality is used (which should
	 * be near optimal for many applications).  See the IJG JPEG
	 * library documentation for more details.
	 */ 
#define JPEG_QUALITY -1
	gdImageJpegCtx(im, &ctx, JPEG_QUALITY);
	break;
#endif

#ifdef HAVE_GD_PNG
    case FORMAT_PNG:
	gdImagePngCtx(im, &ctx);
        break;
#endif

    case FORMAT_GD:
	gdImageGd(im, job->output_file);
	break;

    case FORMAT_GD2:
#define GD2_CHUNKSIZE 128
#define GD2_RAW 1
#define GD2_COMPRESSED 2
	gdImageGd2(im, job->output_file, GD2_CHUNKSIZE, GD2_COMPRESSED);
	break;

#ifdef HAVE_GD_GIF
    case FORMAT_WBMP:
	{
	    /* Use black for the foreground color for the B&W wbmp image. */
            int black = gdImageColorResolveAlpha(im, 0, 0, 0, gdAlphaOpaque);
	    gdImageWBMPCtx(im, black, &ctx);
	}
	break;
#endif

#if 0
/* libgd only supports reading of xpm files */
#ifdef HAVE_GD_XPM
    case FORMAT_XBM:
	gdImageXbm(im, job->output_file);
#endif
#endif
	break;
    default:
	break;
    }

    gdImageDestroy(im);
}

static gvdevice_engine_t gd_engine = {
    NULL,		/* gd_initialize */
    gd_format,
    NULL,		/* gd_finalize */
};

static gvdevice_features_t device_features_gd = {
    GVDEVICE_BINARY_FORMAT
      | GVDEVICE_DOES_TRUECOLOR,/* flags */
    {0.,0.},                    /* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},                  /* dpi */
};

static gvdevice_features_t device_features_gd_no_writer = {
    GVDEVICE_BINARY_FORMAT
      | GVDEVICE_NO_WRITER
      | GVDEVICE_DOES_TRUECOLOR,/* flags */
    {0.,0.},                    /* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},                  /* dpi */
};
#endif
#endif

gvplugin_installed_t gvdevice_gd_types[] = {
#ifdef HAVE_LIBGD
#ifdef HAVE_PANGOCAIRO

#ifdef HAVE_GD_GIF
    {FORMAT_GIF, "gif:cairo", 10, &gd_engine, &device_features_gd},
    {FORMAT_WBMP, "wbmp:cairo", 5, &gd_engine, &device_features_gd},
#endif

#ifdef HAVE_GD_JPEG
    {FORMAT_JPEG, "jpe:cairo", 5, &gd_engine, &device_features_gd},
    {FORMAT_JPEG, "jpeg:cairo", 5, &gd_engine, &device_features_gd},
    {FORMAT_JPEG, "jpg:cairo", 5, &gd_engine, &device_features_gd},
#endif

#ifdef HAVE_GD_PNG
    {FORMAT_PNG, "png:cairo", 5, &gd_engine, &device_features_gd},
#endif

    {FORMAT_GD, "gd:cairo", 5, &gd_engine, &device_features_gd_no_writer},
    {FORMAT_GD2, "gd2:cairo", 5, &gd_engine, &device_features_gd_no_writer},

#if 0
/* libgd only supports reading of xpm files */
#ifdef HAVE_GD_XPM
    {FORMAT_XBM, "xbm:cairo", 5, &gd_engine, &device_features_gd},
#endif
#endif

#endif
#endif
    {0, NULL, 0, NULL, NULL}
};
