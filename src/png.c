/*
  pygame - Python Game Library
  Copyright (C) 2000-2001 Pete Shinners, 2006 Rene Dudfield

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#ifdef HAVE_PNG

#include <png.h>
#include "pgpng.h"

static int _write_png (char *file_name, png_bytep *rows, int w, int h,
    int colortype, int bitdepth);

static int
_write_png (char *file_name, png_bytep *rows, int w, int h, int colortype,
    int bitdepth)
{
    png_structp png_ptr;
    png_infop info_ptr;
    FILE *fp = NULL;
    char *doing = "open for writing";

    if (!(fp = fopen (file_name, "wb")))
        goto fail;

    doing = "create png write struct";
    if (!(png_ptr = png_create_write_struct
          (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
    {
        fclose (fp);
        goto fail;
    }
    doing = "create png info struct";
    if (!(info_ptr = png_create_info_struct (png_ptr)))
    {
        fclose (fp);
        goto fail;
    }
    if (setjmp (png_jmpbuf (png_ptr)))
    {
        fclose (fp);
        goto fail;
    }
    doing = "init IO";
    png_init_io (png_ptr, fp);

    doing = "write header";
    png_set_IHDR (png_ptr, info_ptr, (png_uint_32)w, (png_uint_32)h,
        bitdepth, colortype, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);
    
    doing = "write info";
    png_write_info (png_ptr, info_ptr);

    doing = "write image";
    png_write_image (png_ptr, rows);

    doing = "write end";
    png_write_end (png_ptr, NULL);

    doing = "closing file";
    if(0 != fclose (fp))
        goto fail;
    return 0;

fail:
    SDL_SetError ("could not %s", doing);
    return 0;
}

int
pyg_save_png (SDL_Surface *surface, char *file)
{
    static unsigned char** ss_rows;
    static int ss_size;
    static int ss_w, ss_h;
    SDL_Surface *ss_surface;
    SDL_Rect ss_rect;
    int r, i;
    int alpha = 0;
    int pixel_bits = 32;

    unsigned surf_flags;
    unsigned surf_alpha;

    if (!surface)
    {
        SDL_SetError ("surface argument NULL");
        return 0;
    }
    if (!file)
    {
        SDL_SetError ("file argument NULL");
        return 0;
    }

    ss_rows = 0;
    ss_size = 0;
    ss_surface = NULL;

    ss_w = surface->w;
    ss_h = surface->h;

    if (surface->format->Amask)
    {
        alpha = 1;
        pixel_bits = 32;
    }
    else
        pixel_bits = 24;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    ss_surface = SDL_CreateRGBSurface (SDL_SWSURFACE|SDL_SRCALPHA,
        ss_w, ss_h, pixel_bits, 0xff0000, 0xff00, 0xff, 0x000000ff);
#else
    ss_surface = SDL_CreateRGBSurface (SDL_SWSURFACE|SDL_SRCALPHA,
        ss_w, ss_h, pixel_bits, 0xff, 0xff00, 0xff0000, 0xff000000);
#endif

    if (ss_surface == NULL)
        return 0;
    
    surf_flags = surface->flags & (SDL_SRCALPHA | SDL_SRCCOLORKEY);
    surf_alpha = surface->format->alpha;
    if (surf_flags & SDL_SRCALPHA)
        SDL_SetAlpha (surface, 0, 255);
    if (surf_flags & SDL_SRCCOLORKEY)
        SDL_SetColorKey (surface, 0, surface->format->colorkey);

    ss_rect.x = 0;
    ss_rect.y = 0;
    ss_rect.w = ss_w;
    ss_rect.h = ss_h;
    SDL_BlitSurface (surface, &ss_rect, ss_surface, NULL);

    if (ss_size == 0)
    {
        ss_size = ss_h;
        ss_rows = (unsigned char**) malloc (sizeof (unsigned char*) * ss_size);
        if (ss_rows == NULL)
        {
            SDL_FreeSurface (ss_surface);
            return 0;
        }
    }
    if (surf_flags & SDL_SRCALPHA)
        SDL_SetAlpha (surface, SDL_SRCALPHA, (Uint8)surf_alpha);
    if (surf_flags & SDL_SRCCOLORKEY)
        SDL_SetColorKey (surface, SDL_SRCCOLORKEY, surface->format->colorkey);

    for (i = 0; i < ss_h; i++)
    {
        ss_rows[i] = ((unsigned char*)ss_surface->pixels) +
            i * ss_surface->pitch;
    }

    if (alpha)
    {
        r = _write_png (file, ss_rows, surface->w, surface->h,
            PNG_COLOR_TYPE_RGB_ALPHA, 8);
    }
    else
    {
        r = _write_png (file, ss_rows, surface->w, surface->h,
            PNG_COLOR_TYPE_RGB, 8);
    }

    free (ss_rows);
    SDL_FreeSurface (ss_surface);
    ss_surface = NULL;
    return r;
}

#endif /* HAVE_PNG */
