#include "IMem.h" // Inline memory operations
#include "CPU.h"
#include "Graf.h"

uint8 *drawbuf  = 0; // указывает либо на LFB либо на doublebuffer
uint8 *LFB      = 0; // LFB
uint8 *dbuf     = 0; // double buffer
bool   dbufmode = false;
rgb    palette[256];

// ==================================
//           RENDER METHODS
// ==================================
void blit_dbuf_qwords() // по 8 байт
{
   move_memDQ(LFB, dbuf, 76800);
}

void blit_dbuf_mmx() // по 16 байт через MMX
{
   mmx_move(LFB, dbuf, 38400);
}

void (*blit_DBuf)() = blit_dbuf_qwords;
// ==================================

// Разбиение палитры: 16 базовых + 192 градиентных + 48 динамических
// $NB$ Градиентных может быть и меньше
// $NB$ Heh, strange glitch with vbe_GetPalette!!!
static void init_graf_palette()
{
   int x;
   vbe_GetPalette(palette, 0, 256);

   for(int i = 0; i < 32; i++)
   {
      x = i+16;
      palette[x].r = 0;
      palette[x].g = 0;
      palette[x].b = (i << 3) + 7;
   }
   for(i = 0; i < 32; i++)
   {
      x = i+16+32;
      palette[x].r = (i << 3) + 7;
      palette[x].g = 0;
      palette[x].b = 255;
   }
   for(i = 0; i < 32; i++)
   {
      x = i+16+64;
      palette[x].r = 255;
      palette[x].g = 0;
      palette[x].b = ((31 - i) << 3) + 7;
   }
   for(i = 0; i < 32; i++)
   {
      x = i+16+96;
      palette[x].r = 255;
      palette[x].g = (i << 3) + 7;
      palette[x].b = 0;
   }
   for(i = 0; i < 32; i++)
   {
      x = i+16+128;
      palette[x].r = ((31 - i) << 3) + 7;
      palette[x].g = 255;
      palette[x].b = 0;
   }
   for(i = 0; i < 32; i++)
   {
      x = i+16+160;
      palette[x].r = 0;
      palette[x].g = ((31 - i) << 3) + 7;
      palette[x].b = 0;
   }
   // 48 динамических цветов - используйте с пользой :))
   for(i = 0; i < 48; i++)
   {
      x = i+16+192;
      palette[x].r = palette[x].g = palette[x].b = 0;
   }
   vbe_SetPalette(palette, 0, 256);
}

bool graf_Init(bool setpal)
{
   // Инициализация VBE
   vbe_Init();
   uint16 mode = vbe_FindMode(800, 600, 8); // режим 800x600 256 цветов
   if(mode)
   {
      if(!vbe_SetMode(mode, true, false)) return false;
      if(!vbe_SetLogicalLineLength(1024)) return false;
      if(vbe_8bitDAC()) vbe_SetDACWidth(8);
      LFB = (uint8 *)vbe_GetLFB(mode);

      if(CPU::mmx()) blit_DBuf = blit_dbuf_mmx;
      drawbuf = LFB;
      clear();
      if(setpal) init_graf_palette();
      return true;
   }
   return false;
}

bool graf_InitDBuf(bool setpal)
{
   if(!graf_Init(setpal)) return false;
   // 100 extra lines for not corrupting memory when drawing mouse cursors
   dbuf = new uint8 [LFB_SIZE + 100*LINE_BYTES];
   if(!dbuf) return false;
   drawbuf = dbuf;
   clear();
   dbufmode = true;
   return true;
}

void graf_Close()
{
   vbe_SetMode(3);
   if(dbuf)
   {
      delete dbuf;
      dbuf = 0;
      dbufmode = false;
   }
   vbe_Done();
}

void clear(uint8 c)
{
   fill_mem(drawbuf, c, LFB_SIZE);
}

void hzline(Coord x1, Coord x2, Coord y, uint8 color)
{
   fill_mem(drawbuf + YMUL(y) + x1, color, x2 - x1 + 1);
}

void vtline(Coord x, Coord y1, Coord y2, uint8 color)
{
   uint8 *addr = drawbuf + YMUL(y1) + x;
   int    ht   = y2 - y1 + 1;

// ??  for(int i = y1; i <= y2; i++)
   while(ht--)
   {
      *addr = color;
      addr += LINE_BYTES;
   }
}

#define ABS(x) (((x) < 0) ? -(x) : (x))

void line(Coord x1, Coord y1, Coord x2, Coord y2, uint8 color)
{
   if(y1 == y2)      hzline(x1, x2, y1, color);
   else if(x1 == x2) vtline(x1, y1, y2, color);
   else
   {
      int x, y, delta_x, delta_y, t, distance, inc_x, inc_y;
      char *addr = drawbuf + YMUL(y1) + x1;

      x = y = t = 0;
      // Вычислить изменение позиции по осям
      delta_x = x2 - x1;
      delta_y = y2 - y1;
      // Вычислить инкременты по осям
      inc_x = (delta_x > 0) ? 1 : (delta_x == 0) ? 0 : -1;
      inc_y = (delta_y > 0) ? LINE_BYTES : (delta_y == 0) ? 0 : -LINE_BYTES;
      // Установить абсолютные значения delta
      delta_x = ABS(delta_x) + 1;
      delta_y = ABS(delta_y) + 1;
      // Установить distance в зависимости от delta_x и delta_y
      distance = (delta_x > delta_y) ? delta_x : delta_y;

      while(t <= distance)
      {
         *addr = color;

         x += delta_x;
         y += delta_y;

         if(x > distance)
         {
            x -= distance;
            addr += inc_x;
         }
         if(y > distance)
         {
            y -= distance;
            addr += inc_y;
         }
         t++;
      }
   }
}

void box(Coord x1, Coord y1, Coord x2, Coord y2, uint8 color)
{
   hzline(x1,x2,y1,color);
   vtline(x1,y1,y2,color);
   hzline(x1,x2,y2,color);
   vtline(x2,y1,y2,color);
}

// ?? see wi_plainbar()
void bar(Coord x1, Coord y1, Coord x2, Coord y2, uint8 color)
{
   while(y1 <= y2) hzline(x1,x2,y1++,color);
}

void drawpixmap(Coord x, Coord y, uint8 *pixmap, uint32 wd, uint32 ht)
{
   uint8 *addr = drawbuf + YMUL(y) + x;
   uint8 byte;
   uint32 w;

   while(ht--)
   {
      w = wd;
      while(w--)
      {
         if(byte = *pixmap) *addr = byte;
         addr++;
         pixmap++;
      }
      addr += (LINE_BYTES - wd);
   }
}

void movepixmap(Coord x, Coord y, uint8 *pixmap, uint32 wd, uint32 ht)
{
   uint8 *addr = drawbuf + YMUL(y) + x;

   while(ht--)
   {
      move_mem(addr, pixmap, wd);
      addr += LINE_BYTES;
      pixmap += wd;
   }
}

void clippixmap(Coord x, Coord y, uint8 *pixmap, uint32 wd, uint32 ht)
{
   uint8 *addr = drawbuf + YMUL(y) + x;
   uint32 w;

   while(ht--)
   {
      w = wd;
      while(w--)
      {
         if(*pixmap == 0) *addr = 0;
         addr++;
         pixmap++;
      }
      addr += (LINE_BYTES - wd);
   }
}

void readpixmap(Coord x, Coord y, uint8 *pixmap, uint32 wd, uint32 ht)
{
   uint8 *addr = drawbuf + YMUL(y) + x;

   while(ht--)
   {
      move_mem(pixmap, addr, wd);
      addr += LINE_BYTES;
      pixmap += wd;
   }
}
