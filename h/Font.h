#ifndef GRAF_FONT_H
#define GRAF_FONT_H

#include "Tailor.h"
#include "Pixmap.h"

class File;

class Font
{
   public:
      Font();         // construct an empty Font
      Font(File &is); // construct and load a Font from File
     ~Font();

      void save(File &os);
      bool load(File &is);

      Pixmap *find_glyph(int chr) const;

   public:
      friend void gprintf(const Font&, char *, ...);
      friend void gtprintf(const Font&, char *, ...);

      enum RangeFlags
      {
         rfHasMask = 1,
         rfBW = 2,
         rf256 = 4,
         rfTruecolor = 8,
         rfFixedsize = 16
      };

      int nl_height; // height of a newline
      int num_ranges;
      struct GlyphRange
      {
         uint16 start, count, flags;
         Pixmap *glyphs;
      } *ranges;
};

#endif
