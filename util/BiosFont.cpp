// Convert a BIOS-style 8x8, 8x14 or 8x16 bitmap font
// into 256-color Font resource
// MOVE IT AS PART OF EnFonted!
#include "Font.h"
#include "FileStrm.h"
#include <StdIo.h>
#include <ConIo.h>

#pragma library (Graf)
#pragma library (SysLib)

int height;

uint8 *convert_bw(uint8 *bw)
{
   uint8 *d = new uint8 [16*8];
   uint8 *w = d;
   uint8  bitmask;

   for(int i = 0; i < 16; i++)
   {
      bitmask = 0x80;
      for(int j = 0; j < 8; j++)
      {
         if(*bw & bitmask)
         {
            *w = 15;
         }
         else
         {
            *w = 0;
         }
         w++;
         bitmask >>= 1;
      }
      bw++;
   }
   return d;
}

void main(int ac, char **av)
{
   puts("Grabs 8x8, 8x14, 8x16 BIOS style bitmap fonts into Font resources");
   puts("(c) 1999 Berk // NOD      -- this file is part of Graf project --");
   puts("usage: biosfont source_biosfont_file target_font_file. NO CHECKS!");

   Font f;
   uint8 *bwbuf, *c256buf;
   FileStream is(av[1], open_read);
   FileStream os(av[2], open_write);

   if(is.size() != 4096) return;

   f.num_ranges = 1;
   f.ranges = new Font::GlyphRange;
   f.nl_height = 16;
   f.ranges->start = 0;
   f.ranges->count = 256;
   f.ranges->flags = Font::rf256;
   f.ranges->glyphs = new Pixmap [256];
   bwbuf = new uint8 [16];
   for(int i = 0; i < 256; i++)
   {
      is.get_bytes(bwbuf, 16);
      c256buf = convert_bw(bwbuf);
      f.ranges->glyphs[i].alloc_copy(c256buf, 8, 16);
      delete c256buf;
      printf("Processed char %d of 256\r", i+1);
   }
   f.save(os);
   printf("\nDone\n");
}
