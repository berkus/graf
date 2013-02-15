// Performance measure: Framerate calc
#include <StdIo.h>
#include "Graf.h"

#pragma library (Graf)

// timer interface
uint32 *timerTicks = (uint32 *)0x46C;

int timer, fps;

//───────────────────────────────────────────────────────────────────────────
// frame rate for simple screen cleaning w/o dbuf
// actually, a measure of LFB memory speed
//
int fps_clean = 0;

void framerate_clean()
{
   if(!graf_init_lfb()) return;
   START_TIME(timer);
   for(int i = 0; i < 1000; i++) clear(i);
   STOP_TIME(timer);
   fps_clean = FPS(1000,timer);
   graf_close();
}

void main()
{
   framerate_clean();
   puts(">> Framerates <<\n");
   puts("LFB mode                        Dbuf mode");
   printf("clear screen: %d\n", fps_clean);
}
