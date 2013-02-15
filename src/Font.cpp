/*
╔════════════════════════════════════════════════════════════════════════════
║ FONT file structure: VEC & BMP font
║
║ ChunkHeader file_header('FONT',size,VERSION);
║ далее 1 или 2 чанка в любой последовательности
║
║ Чанк 'Bitmap font'
║   char type = 'B' (Bitmap)
║   short newline_height;
║   short num_fontdefs;
║   fontdefs[num_fontdefs] {
║      short start, count; // UNICODE
║      short flags; // BW Bitmap, 256 indexed, RGB truecolor, has_mask flag
║      letters[count] { letter map [,mask map] };
║   };
║  Bitmap glyphs are allocated as 'Pixmap[] letters, masks;' and inited to
║  emptyLetter (■), then loaded from file.
║
║ Чанк 'Vector font'
║   char type = 'V' (Vector)
║   short num_fontdefs;
║   fontdefs[num_fontdefs] {
║      short start, count, flags;
║   }
╚════════════════════════════════════════════════════════════════════════════
*/
#include "Font.h"
#include "File.h"
#include "Exception.h"

static char no_memory[] = "Font: Cannot allocate data";
static char no_loaded[] = "Font: Cannot load font";

// construct an empty Font
Font::Font()
{
   num_ranges = 0;
   ranges     = 0;
}

// construct and load a Font from File
Font::Font(File &in)
{
   num_ranges = 0;
   ranges     = 0;
   if(!load(in)) throw Exception(no_loaded);
}

Font::~Font()
{
   if(ranges)
   {
      for(int i = 0; i < num_ranges; i++) delete ranges[i].glyphs;
      delete ranges;
   }
}

const uint32
   FONT_MAGIC   = MAGIC('F','O','N','T'),
   FONT_VERSION = 0x00010101; // "0.1.1 alpha"

#define P 0x0F
#define v 0x00
static uint8 emptyLetterImage[] =
{
   // dimensions 8x8
   8, 0, 8, 0,
   v,v,v,v,v,v,v,v,
   v,P,P,P,P,P,P,v,
   v,P,P,P,P,P,P,v,
   v,P,P,P,P,P,P,v,
   v,P,P,P,P,P,P,v,
   v,P,P,P,P,P,P,v,
   v,P,P,P,P,P,P,v,
   v,v,v,v,v,v,v,v
};
#undef P
#undef v

static Pixmap emptyLetter(emptyLetterImage);

// load a Font from File
bool Font::load(File &is)
{
   Chunk c;
   uint8 type;

   c.magic   = is.get_dword();
   c.size    = is.get_dword();
   c.version = is.get_dword();

   if(c.magic != FONT_MAGIC || c.version != FONT_VERSION) return false;

   type = is.get_byte();

   if(type == 'B') // Bitmap Font
   {
      nl_height  = is.get_word();
      num_ranges = is.get_word();
      if(num_ranges)
      {
         ranges = new GlyphRange [num_ranges];
         if(!ranges) throw Exception(no_memory);

         for(int i = 0; i < num_ranges; i++)
         {
            GlyphRange& r = ranges[i];

            r.start = is.get_word();
            r.count = is.get_word();
            r.flags = is.get_word();

            // $NB$ masked glyphs are not supported yet
            //      only 256 color fonts supported
            if(r.flags & rfHasMask) return false;
            if(!(r.flags & rf256)) return false;

            r.glyphs = new Pixmap [r.count];
            if(!r.glyphs) throw Exception(no_memory);

            for(int g = 0; g < r.count; g++)
            {
               uint8 *buf;
               uint16 w, h;

               w = is.get_word();
               h = is.get_word();
               buf = new uint8 [w*h];
               if(!buf) throw Exception(no_memory);
               is.get_bytes(buf, w*h);
               r.glyphs[g].alloc_copy(buf, w, h);
               delete buf;
            }
         }
      }
   }
   else if(type == 'V') // Vector Font
   {
      // just skip past chunk, return error
      // !! should support for any sequence of chunks !!
      is.seek(c.size-12, seek_move);
      return false;
   }
   else return false; // unknown/invalid Font chunk

   return true;
}

void Font::save(File &os)
{
   uint32 start_pos = os.tell();
   os.put_dword(FONT_MAGIC);
   os.put_dword(0); // size field
   os.put_dword(FONT_VERSION);

   os.put_byte('B'); // Bitmap Font

   os.put_word(nl_height);
   os.put_word(num_ranges);

   for(int i = 0; i < num_ranges; i++)
   {
      GlyphRange& r = ranges[i];

      os.put_word(r.start);
      os.put_word(r.count);
      os.put_word(r.flags);

      for(int g = 0; g < r.count; g++)
      {
         Pixmap& p = r.glyphs[g];

         os.put_bytes(p.image, p.width()*p.height()+4);
      }
   }
   uint32 end_pos = os.tell();
   os.seek(start_pos+4, seek_start);
   os.put_dword(end_pos - start_pos);
   os.seek(end_pos, seek_start);
}

Pixmap *Font::find_glyph(int chr) const
{
   for(int i = 0; i < num_ranges; i++)
   {
      GlyphRange &r = ranges[i];
      if(r.start > chr || chr > r.start+r.count) continue;
      return &r.glyphs[chr - r.start];
   }
   return &emptyLetter;
}
