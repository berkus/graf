//
// Text output functions using Font
//

#include <StdIo.h>
#include <StdArg.h>
#include <String.hpp>
#include "Font.h"

static char   gtBuffer[256];
static uint32 gtX = 0, gtY = 0;
static uint32 gtMaxHeight = 0;

void gsetpos(uint32 x, uint32 y)
{
   gtX = x;
   gtY = y;
}

void gprintf(const Font& fnt, char *fmt, ...)
{
   va_list ap;
   uint32 x, y, old_x = gtX;
   int len;
   char ch;
   Pixmap *glyph;

   va_start(ap, fmt);
   vsprintf(gtBuffer, fmt, ap);
   va_end(ap);
   len = strlen(gtBuffer);

   for(int i = 0; i < len; i++)
   {
      ch = gtBuffer[i];
      if(ch == '\n')      gtY += fnt.nl_height;
      else if(ch == '\r') gtX = old_x;
      else
      {
         glyph = fnt.find_glyph(ch);
         glyph->draw(gtX, gtY);
         gtX += glyph->width();
         if(gtX > 799)
         {
            gtX = old_x;
            gtY += fnt.nl_height;
         }
      }
   }
}

void gtprintf(const Font& fnt, const Pixmap& texture, char *fmt, ...)
{
   va_list ap;
   uint32 x, y;
   int len;
   char ch;
   Pixmap *glyph;

   va_start(ap, fmt);
   vsprintf(gtBuffer, fmt, ap);
   va_end(ap);
   len = strlen(gtBuffer);

   for(int i = 0; i < len; i++)
   {
      ch = gtBuffer[i];
      if(ch == '\n')      gtY += fnt.nl_height;
      else if(ch == '\r') gtX = 0;
      else
      {
         glyph = fnt.find_glyph(ch);
         glyph->draw(gtX, gtY);
         gtX += glyph->width();
         if(gtX > 799)
         {
            gtX = 0;
            gtY += fnt.nl_height;
         }
      }
   }
}
