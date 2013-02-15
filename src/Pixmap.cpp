#include "IMem.h"
#include "Pixmap.h"
#include "Graf.h"
#include "Exception.h"

static char no_memory[] = "Pixmap: cannot allocate memory";

Pixmap::Pixmap(uint8 *area)
{
   operator =(area);
}

Pixmap::Pixmap(Coord x1, Coord y1, Coord x2, Coord y2)
{
   uint32 w, h;
   w = x2 - x1 + 1;
   h = y2 - y1 + 1;
   alloc_copy(0, w, h);
}

Pixmap::Pixmap(const Pixmap& p)
{
   operator =(p);
}

Pixmap& Pixmap::operator = (const Pixmap& p)
{
   alloc_copy(p.image+4, p.width(), p.height());
   return *this;
}

Pixmap& Pixmap::operator = (uint8 *area)
{
   uint32 w, h;
   w = ((uint16 *)area)[0];
   h = ((uint16 *)area)[1];
   alloc_copy(area+4, w, h);
   return *this;
}

void Pixmap::alloc_copy(uint8 *data, uint32 w, uint32 h)
{
   if(w != width() || h != height() || !image)
   {
      delete image;
      image = new uint8 [w * h + 4];
      if(!image) throw Exception(no_memory);
      ((uint16 *)image)[0] = w;
      ((uint16 *)image)[1] = h;
   }
   move_mem(image+4, data, w*h);
}
