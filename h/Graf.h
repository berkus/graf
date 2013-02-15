//╔══════════════════════════════════════════════════════════════════════════
//║ Библиотека Graf - работа с VESA 2.0 800x600x256 LFB видеорежимом
//╚══════════════════════════════════════════════════════════════════════════

#ifndef GRAF_H
#define GRAF_H

#include "Tailor.h"
#include "VBE.h"

typedef short Coord;
const short
   COORD_MIN = -32768,
   COORD_MAX =  32767;

#define LINE_BYTES 1024
#define LFB_SIZE   (600L*LINE_BYTES)
#define YMUL(y)    ((y) << 10)

extern uint8 *drawbuf; // buffer POINTER (may be either dbuf or LFB)
extern bool   dbufmode; // true when using double buffer
extern rgb    palette[256];

//$NB$ Возможно стоит выкинуть инициализацию своей палитры нафиг!

// INITIALIZATION //
// Установить режим 800x600x256
// Возвращает true при успешной установке режима
bool graf_Init(bool setpalette = true);

// Установить режим 800x600x256, создать double buffer
// Возвращает true при успешной установке режима
bool graf_InitDBuf(bool setpalette = true);

// Восстановить предыдущий режим
void graf_Close();


inline bool graf_DBuf() { return dbufmode; }


// --- DRAWING PRIMITIVES --- //

// Рисовать горизонтальную линию
void hzline(Coord x1, Coord x2, Coord y, uint8 color);

// Рисовать вертикальную линию
void vtline(Coord x, Coord y1, Coord y2, uint8 color);

// Рисовать произвольную линию
void line(Coord x1, Coord y1, Coord x2, Coord y2, uint8 color);

// Рисовать прямоугольник
void box(Coord x1, Coord y1, Coord x2, Coord y2, uint8 color);

// Рисовать залитый прямоугольник
void bar(Coord x1, Coord y1, Coord x2, Coord y2, uint8 color);


// --- RENDER FUNCTIONS --- //

// Очистить экран (или double buffer)
void clear(uint8 color = 0);

// Рисовать double buffer на экран (в режиме double buffered lfb)
// ДОБАВИТЬ ИНИЦИАЛИЗАЦИЮ ДЛЯ MMX!
extern void (*blit_DBuf)();


// Рисовать pixmap (с сохранением прозрачности)
void drawpixmap(Coord x, Coord y, uint8 *pixmap, uint32 wd, uint32 ht);

// Рисовать pixmap (без сохранения прозрачности)
void movepixmap(Coord x, Coord y, uint8 *pixmap, uint32 wd, uint32 ht);

// Рисовать pixmap (только нулевые пикселы)
void clippixmap(Coord x, Coord y, uint8 *pixmap, uint32 wd, uint32 ht);

// Считать pixmap в буфер (без прозрачности :)
void readpixmap(Coord x, Coord y, uint8 *pixmap, uint32 wd, uint32 ht);


// --- TEXT OUTPUT: TODO --- //

class Font;
class Pixmap;

void gsetpos(Coord x, Coord y);
void gprintf(const Font& fnt, char *fmt, ...);
// It is assumed that texture covers the whole letter in all letters range
void gtprintf(const Font& fnt, const Pixmap& texture, char *fmt, ...);

#endif
