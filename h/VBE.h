#ifndef GRAF_VBE_H
#define GRAF_VBE_H

// Формат данных палитры для функций VBE
struct rgb
{
   uint8 b, g, r, padding;
};

// Инициализация библиотеки. НЕОБХОДИМО вызвать перед вызовом других функций.
void vbe_Init();

// Деинициализация библиотеки. Вызывать после окончания работы.
void vbe_Done();

// Найти заданный VBE-режим.
// Возвращает номер режима если режим найден, 0 в обратном случае.
uint16 vbe_FindMode(uint16 hres, uint16 vres, uint8 bpp);

// Инициализировать заданный видеорежим
bool vbe_SetMode(uint16 mode, bool lfb = true, bool clear = true);

// Получить указатель на LFB для заданного режима
void *vbe_GetLFB(uint16 mode);

// Установить логическую ширину линии в пикселах. Возвращает false при
// невозможности осуществления этой операции.
bool vbe_SetLogicalLineLength(uint16 pixels);

// Является-ли DAC 8 разрядным?
bool vbe_8bitDAC();

// Установить ширину регистров DAC
void vbe_SetDACWidth(uint8 bits); // 6 or 8 bits

// Установить палитру
void vbe_SetPalette(rgb *p, uint16 start, uint16 count);

// Считать палитру
void vbe_GetPalette(rgb *p, uint16 start, uint16 count);

#endif
