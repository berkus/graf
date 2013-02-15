#ifndef GRAF_PIXMAP_H
#define GRAF_PIXMAP_H

#include "Graf.h"

class Pixmap
{
   public:
       Pixmap();
       Pixmap(uint8 *);
       Pixmap(Coord x1, Coord y1, Coord x2, Coord y2);
       Pixmap(const Pixmap&);
      ~Pixmap();

       Pixmap& operator = (const Pixmap&);
       Pixmap& operator = (uint8 *);

       uint32 width() const;
       uint32 height() const;

       operator uint8 *();

       void draw(Coord x, Coord y);
       void move(Coord x, Coord y);
       void clip(Coord x, Coord y);
       void read(Coord x, Coord y);

       void alloc_copy(uint8 *data, uint32 w, uint32 h);

   public:
       uint8 *image;
};

inline Pixmap::Pixmap()
: image(0)
{}

inline Pixmap::~Pixmap()
{
   delete image;
}

inline uint32 Pixmap::width() const
{
   return ((uint16 *)image)[0];
}

inline uint32 Pixmap::height() const
{
   return ((uint16 *)image)[1];
}

inline Pixmap::operator uint8 *()
{
   return image + 4;
}

inline void Pixmap::draw(Coord x, Coord y)
{
   drawpixmap(x, y, image+4, width(), height());
}

inline void Pixmap::clip(Coord x, Coord y)
{
   clippixmap(x, y, image+4, width(), height());
}

inline void Pixmap::move(Coord x, Coord y)
{
   movepixmap(x, y, image+4, width(), height());
}

inline void Pixmap::read(Coord x, Coord y)
{
   readpixmap(x, y, image+4, width(), height());
}

#endif
