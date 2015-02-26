/* Copyright  (C) 2010-2015 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (rpng.c).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _RPNG_DECODE_COMMON_H
#define _RPNG_DECODE_COMMON_H

#include <retro_inline.h>

static INLINE void copy_line_rgb(uint32_t *data,
      const uint8_t *decoded, unsigned width, unsigned bpp)
{
   unsigned i;

   bpp /= 8;

   for (i = 0; i < width; i++)
   {
      uint32_t r, g, b;

      r        = *decoded;
      decoded += bpp;
      g        = *decoded;
      decoded += bpp;
      b        = *decoded;
      decoded += bpp;
      data[i]  = (0xffu << 24) | (r << 16) | (g << 8) | (b << 0);
   }
}

static INLINE void copy_line_rgba(uint32_t *data,
      const uint8_t *decoded, unsigned width, unsigned bpp)
{
   unsigned i;

   bpp /= 8;

   for (i = 0; i < width; i++)
   {
      uint32_t r, g, b, a;
      r        = *decoded;
      decoded += bpp;
      g        = *decoded;
      decoded += bpp;
      b        = *decoded;
      decoded += bpp;
      a        = *decoded;
      decoded += bpp;
      data[i]  = (a << 24) | (r << 16) | (g << 8) | (b << 0);
   }
}

static INLINE void copy_line_bw(uint32_t *data,
      const uint8_t *decoded, unsigned width, unsigned depth)
{
   unsigned i, bit;
   static const unsigned mul_table[] = { 0, 0xff, 0x55, 0, 0x11, 0, 0, 0, 0x01 };
   unsigned mul, mask;
   
   if (depth == 16)
   {
      for (i = 0; i < width; i++)
      {
         uint32_t val = decoded[i << 1];
         data[i]      = (val * 0x010101) | (0xffu << 24);
      }
      return;
   }

   mul  = mul_table[depth];
   mask = (1 << depth) - 1;
   bit  = 0;

   for (i = 0; i < width; i++, bit += depth)
   {
      unsigned byte = bit >> 3;
      unsigned val  = decoded[byte] >> (8 - depth - (bit & 7));

      val          &= mask;
      val          *= mul;
      data[i]       = (val * 0x010101) | (0xffu << 24);
   }
}

static INLINE void copy_line_gray_alpha(uint32_t *data,
      const uint8_t *decoded, unsigned width,
      unsigned bpp)
{
   unsigned i;

   bpp /= 8;

   for (i = 0; i < width; i++)
   {
      uint32_t gray, alpha;

      gray     = *decoded;
      decoded += bpp;
      alpha    = *decoded;
      decoded += bpp;

      data[i]  = (gray * 0x010101) | (alpha << 24);
   }
}

static INLINE void copy_line_plt(uint32_t *data,
      const uint8_t *decoded, unsigned width,
      unsigned depth, const uint32_t *palette)
{
   unsigned i, bit;
   unsigned mask = (1 << depth) - 1;

   bit = 0;

   for (i = 0; i < width; i++, bit += depth)
   {
      unsigned byte = bit >> 3;
      unsigned val  = decoded[byte] >> (8 - depth - (bit & 7));

      val          &= mask;
      data[i]       = palette[val];
   }
}

static void png_pass_geom(const struct png_ihdr *ihdr,
      unsigned width, unsigned height,
      unsigned *bpp_out, unsigned *pitch_out, size_t *pass_size)
{
   unsigned bpp;
   unsigned pitch;

   switch (ihdr->color_type)
   {
      case 0:
         bpp   = (ihdr->depth + 7) / 8;
         pitch = (ihdr->width * ihdr->depth + 7) / 8;
         break;

      case 2:
         bpp   = (ihdr->depth * 3 + 7) / 8;
         pitch = (ihdr->width * ihdr->depth * 3 + 7) / 8;
         break;

      case 3:
         bpp   = (ihdr->depth + 7) / 8;
         pitch = (ihdr->width * ihdr->depth + 7) / 8;
         break;

      case 4:
         bpp   = (ihdr->depth * 2 + 7) / 8;
         pitch = (ihdr->width * ihdr->depth * 2 + 7) / 8;
         break;

      case 6:
         bpp   = (ihdr->depth * 4 + 7) / 8;
         pitch = (ihdr->width * ihdr->depth * 4 + 7) / 8;
         break;

      default:
         bpp = 0;
         pitch = 0;
         break;
   }

   if (pass_size)
      *pass_size = (pitch + 1) * ihdr->height;
   if (bpp_out)
      *bpp_out = bpp;
   if (pitch_out)
      *pitch_out = pitch;
}

static void deinterlace_pass(uint32_t *data, const struct png_ihdr *ihdr,
      const uint32_t *input, unsigned pass_width, unsigned pass_height,
      const struct adam7_pass *pass)
{
   unsigned x, y;

   data += pass->y * ihdr->width + pass->x;

   for (y = 0; y < pass_height;
         y++, data += ihdr->width * pass->stride_y, input += pass_width)
   {
      uint32_t *out = data;
     
      for (x = 0; x < pass_width; x++, out += pass->stride_x)
         *out = input[x];
   }
}

static void png_reverse_filter_deinit(struct rpng_process_t *pngp)
{
   if (pngp->decoded_scanline)
      free(pngp->decoded_scanline);
   pngp->decoded_scanline = NULL;
   if (pngp->prev_scanline)
      free(pngp->prev_scanline);
   pngp->prev_scanline    = NULL;

   pngp->pass_initialized = false;
   pngp->h                = 0;
}

static bool png_reverse_filter_init(uint32_t *data, const struct png_ihdr *ihdr,
      const uint8_t *inflate_buf, struct rpng_process_t *pngp,
      const uint32_t *palette)
{
   size_t pass_size;

   png_pass_geom(ihdr, ihdr->width, ihdr->height, &pngp->bpp, &pngp->pitch, &pass_size);

   if (pngp->stream.total_out < pass_size)
      return false;

   pngp->prev_scanline    = (uint8_t*)calloc(1, pngp->pitch);
   pngp->decoded_scanline = (uint8_t*)calloc(1, pngp->pitch);

   if (!pngp->prev_scanline || !pngp->decoded_scanline)
      goto error;

   pngp->h = 0;
   pngp->pass_initialized = true;

   return true;

error:
   png_reverse_filter_deinit(pngp);
   return false;
}

static int png_reverse_filter_wrapper(uint32_t *data, const struct png_ihdr *ihdr,
      const uint8_t *inflate_buf, struct rpng_process_t *pngp,
      const uint32_t *palette)
{
   unsigned i, filter;
   bool cont;

begin:
   cont = pngp->h < ihdr->height;

   if (!cont)
   {
      png_reverse_filter_deinit(pngp);
      return 1;
   }

   filter = *inflate_buf++;

   switch (filter)
   {
      case 0: /* None */
         memcpy(pngp->decoded_scanline, inflate_buf, pngp->pitch);
         break;

      case 1: /* Sub */
         for (i = 0; i < pngp->bpp; i++)
            pngp->decoded_scanline[i] = inflate_buf[i];
         for (i = pngp->bpp; i < pngp->pitch; i++)
            pngp->decoded_scanline[i] = pngp->decoded_scanline[i - pngp->bpp] + inflate_buf[i];
         break;

      case 2: /* Up */
         for (i = 0; i < pngp->pitch; i++)
            pngp->decoded_scanline[i] = pngp->prev_scanline[i] + inflate_buf[i];
         break;

      case 3: /* Average */
         for (i = 0; i < pngp->bpp; i++)
         {
            uint8_t avg = pngp->prev_scanline[i] >> 1;
            pngp->decoded_scanline[i] = avg + inflate_buf[i];
         }
         for (i = pngp->bpp; i < pngp->pitch; i++)
         {
            uint8_t avg = (pngp->decoded_scanline[i - pngp->bpp] + pngp->prev_scanline[i]) >> 1;
            pngp->decoded_scanline[i] = avg + inflate_buf[i];
         }
         break;

      case 4: /* Paeth */
         for (i = 0; i < pngp->bpp; i++)
            pngp->decoded_scanline[i] = paeth(0, pngp->prev_scanline[i], 0) + inflate_buf[i];
         for (i = pngp->bpp; i < pngp->pitch; i++)
            pngp->decoded_scanline[i] = paeth(pngp->decoded_scanline[i - pngp->bpp],
                  pngp->prev_scanline[i], pngp->prev_scanline[i - pngp->bpp]) + inflate_buf[i];
         break;

      default:
         goto error;
   }

   switch (ihdr->color_type)
   {
      case 0:
         copy_line_bw(data, pngp->decoded_scanline, ihdr->width, ihdr->depth);
         break;
      case 2:
         copy_line_rgb(data, pngp->decoded_scanline, ihdr->width, ihdr->depth);
         break;
      case 3:
         copy_line_plt(data, pngp->decoded_scanline, ihdr->width,
               ihdr->depth, palette);
         break;
      case 4:
         copy_line_gray_alpha(data, pngp->decoded_scanline, ihdr->width,
               ihdr->depth);
         break;
      case 6:
         copy_line_rgba(data, pngp->decoded_scanline, ihdr->width, ihdr->depth);
         break;
   }

   memcpy(pngp->prev_scanline, pngp->decoded_scanline, pngp->pitch);

   pngp->h++;
   inflate_buf += pngp->pitch;
   data += ihdr->width;

   goto begin;

   return 0;

error:
   png_reverse_filter_deinit(pngp);
   return -1;
}

static bool png_reverse_filter(uint32_t *data, const struct png_ihdr *ihdr,
      const uint8_t *inflate_buf, struct rpng_process_t *pngp,
      const uint32_t *palette)
{
   int ret;

   if (!pngp)
      return -1;
   if (!pngp->pass_initialized)
   {
      if (!png_reverse_filter_init(data, ihdr, inflate_buf, pngp, palette))
         return -1;
   }

   do{
      ret = png_reverse_filter_wrapper(data,
                  ihdr, inflate_buf, pngp, palette);
      if (ret == 1 || ret == -1)
         break;
   }while(1);

   if (ret == 1)
      return true;
   return false;
}

static bool png_reverse_filter_adam7(uint32_t *data,
      const struct png_ihdr *ihdr,
      const uint8_t *inflate_buf, struct rpng_process_t *pngp,
      const uint32_t *palette)
{
   static const struct adam7_pass passes[] = {
      { 0, 0, 8, 8 },
      { 4, 0, 8, 8 },
      { 0, 4, 4, 8 },
      { 2, 0, 4, 4 },
      { 0, 2, 2, 4 },
      { 1, 0, 2, 2 },
      { 0, 1, 1, 2 },
   };

   for (; pngp->pass < ARRAY_SIZE(passes); pngp->pass++)
   {
      unsigned pass_width, pass_height;
      size_t pass_size;
      struct png_ihdr tmp_ihdr;
      uint32_t *tmp_data = NULL;

      if (ihdr->width <= passes[pngp->pass].x ||
            ihdr->height <= passes[pngp->pass].y) /* Empty pass */
         continue;

      pass_width  = (ihdr->width - 
            passes[pngp->pass].x + passes[pngp->pass].stride_x - 1) / passes[pngp->pass].stride_x;
      pass_height = (ihdr->height - passes[pngp->pass].y + 
            passes[pngp->pass].stride_y - 1) / passes[pngp->pass].stride_y;

      tmp_data = (uint32_t*)malloc(
            pass_width * pass_height * sizeof(uint32_t));

      if (!tmp_data)
         return false;

      tmp_ihdr        = *ihdr;
      tmp_ihdr.width  = pass_width;
      tmp_ihdr.height = pass_height;

      png_pass_geom(&tmp_ihdr, pass_width,
            pass_height, NULL, NULL, &pass_size);

      if (pass_size > pngp->stream.total_out)
      {
         free(tmp_data);
         return false;
      }

      if (!png_reverse_filter(tmp_data,
               &tmp_ihdr, inflate_buf, pngp, palette))
      {
         free(tmp_data);
         return false;
      }

      inflate_buf            += pass_size;
      pngp->stream.total_out -= pass_size;

      deinterlace_pass(data,
            ihdr, tmp_data, pass_width, pass_height, &passes[pngp->pass]);

      free(tmp_data);
   }

   return true;
}

#endif