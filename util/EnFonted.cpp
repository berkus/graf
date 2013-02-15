// EnFonted font editor for Graf
/* Leftmouse draw, rightmouse erase
╔════════════════════════════════════════════════╗
║                                               │ Open │ Save │ Save as │Exit║
║  Letter edit area (16x16 w/ grid up           │──────┘──────┘─────────┘────║
║                                               v                            ║
║              to 32x32 in size)                t────┘────┘────┐──────┐─────┐║
║                                               s Point │ Line │ Rect │ Box │║
║                                               i───────┘──────┘──────┘─────┘║
║                                               z        │                   ║
║                                               e Scrol  │                   ║
║                                               r        │                   ║
║                                               │────────┘                   ║
║                                               │                            ║
║                                               │                            ║
║─────────────────────hzsizer───────────────────■─────────────────────────── ║
║ Add range │ Resize range │   │                                         │   ║
║───────────┴┐─────────────┘   │   Letters list & selector               │   ║
║ Kill range │ Range list  │   │                                         │   ║
╚════════════════════════════════════════════════╝
*/
#include "Graf.h"
#include "Font.h"
#include "FileStrm.h"
#include "EventMan.h"
#include "GUI.h"
#include "Xception.h"

#pragma library (Graf)
#pragma library (KAOS)
#pragma library (SysLib)

Mouse mouse;
Font square;

void load_fonts()
{
   FileStream is("square.frs", open_read);
   if(!square.load(is)) throw Exception("Cannot load font file");
}

// Область редактирования буквы
// 32x32 16x16 grid (512x512 area) at 3,3
void draw_letter_grid(int w, int h)
{
   int i;

   bar(3,3,514,514, 0);
   for(i = 0; i < w; i++) vtline(3+16+i*16,3,514, 12);
   for(i = 0; i < h; i++) hzline(3,514,3+16+i*16, 12);
}

// ──────────────────────────────────────────────────────────────────────────
// Editor globals
bool going = true;           // while not quitting
bool FileIsNew = true;       // when file isn't saved, it doesn't have a name
bool FileIsModified = false; // when file is modified, ask if it needs saving

// ──────────────────────────────────────────────────────────────────────────
// Action handlers
// After start, all handlers are called to provide necessary init.
// Event is ev_null.
//
class fnhandler
{
   public:
      int left, top, w, h;
      fnhandler *next;

      // return false if event is NOT processed
      virtual bool handle_event(Event& e) = 0;
};

fnhandler *handlers = 0;

void AddHandler(fnhandler *h)
{
   h->next = handlers;
   handlers = h;
}

bool CallHandlers(Event& e)
{
   fnhandler *h = handlers;

   mouse.hide();
   while(h)
   {
      if(h->handle_event(e))
      {
         mouse.show();
         return true;
      }
      h = h->next;
   }
   mouse.show();
   return false;
}
// ──────────────────────────────────────────────────────────────────────────

// - PAINT A LETTER AREA
class PaintImageHandler : public fnhandler
{
   public:
      PaintImageHandler();
      virtual bool handle_event(Event& e);
};

PaintImageHandler::PaintImageHandler()
{
   left = 3;
   top  = 3;
   w    = 512;
   h    = 512;
}

bool PaintImageHandler::handle_event(Event& e)
{
   switch(e.what)
   {
      case ev_null:
         draw_letter_grid(32,32);
         return false;
   }
   return false;
}

// -- BUTTONS
class ButtonHandler : public fnhandler
{
   public:
      ButtonHandler(char *title);
      virtual bool handle_event(Event& e);
      virtual void action() = 0;

      char *text;
      int   tlen;
};

ButtonHandler::ButtonHandler(char *title) : text(title)
{
   tlen = strlen(text);
   w = tlen * 8 + 20;
   h = 30;
}

bool ButtonHandler::handle_event(Event& e)
{
   // make coords local here (for mouse)
   uint32 loc_x = e.m.x - left;
   uint32 loc_y = e.m.y - top;

   switch(e.what)
   {
      case ev_null:
         buttonframe(left, top, left+w-1, top+h-1, true);
         gsetpos(left+(w-tlen*8)/2-1, top+(h-16)/2-1);
         gprintf(square, text);
         return false; // let other handlers init

      case ev_mouse_down:
         if(e.m.x >= left && e.m.y >= top && loc_x < w && loc_y < h)
         {
            buttonframe(left, top, left+w-1, top+h-1, false);
            gsetpos(left+(w-tlen*8)/2+1, top+(h-16)/2+1);
            gprintf(square, text);
            return true;
         }
         break;

      case ev_mouse_up:
         if(e.m.x >= left && e.m.y >= top && loc_x < w && loc_y < h)
         {
            buttonframe(left, top, left+w-1, top+h-1, true);
            gsetpos(left+(w-tlen*8)/2-1, top+(h-16)/2-1);
            gprintf(square, text);
            return true;
         }
         break;

      case ev_mouse_click:
         if(e.m.x >= left && e.m.y >= top && loc_x < w && loc_y < h)
         {
            buttonframe(left, top, left+w-1, top+h-1, true);
            gsetpos(left+(w-tlen*8)/2-1, top+(h-16)/2-1);
            gprintf(square, text);
            action();
            return true;
         }
         break;
   }
   return false;
}

// - OPEN FILE
class OpenFileHandler : public ButtonHandler
{
   public:
      OpenFileHandler();
      virtual void action() {}
};

OpenFileHandler::OpenFileHandler() : ButtonHandler("Open")
{
   left =  522;
   top  =  10;
}

// - SAVE FILE
class SaveFileHandler : public ButtonHandler
{
   public:
      SaveFileHandler();
      virtual void action();
};

SaveFileHandler::SaveFileHandler() : ButtonHandler("Save")
{
   left =  588;
   top  =  10;
}

void SaveFileHandler::action()
{
   if(FileIsNew) return; // don't allow save of unnamed file (PUSH SAVE AS!)
}

// - SAVE FILE AS
class SaveFileAsHandler : public ButtonHandler
{
   public:
      SaveFileAsHandler();
      virtual void action() {}
};

SaveFileAsHandler::SaveFileAsHandler() : ButtonHandler("Save as")
{
   left =  662;
   top  =  10;
}

// - EXIT
class ExitHandler : public ButtonHandler
{
   public:
      ExitHandler();
      virtual void action();
};

ExitHandler::ExitHandler() : ButtonHandler("Exit")
{
   left =  742;
   top  =  10;
}

void ExitHandler::action()
{
   if(FileIsModified) ; // ask user to save file
   going = false;
}

// -- DIALOG BOXES
class Dialog
{
   public:
      Dialog(char *title);
      virtual int do_modal() = 0;

      void show_frame();

      char *text;
      fnhandler *fnsave;
};

Dialog::Dialog(char *title) : text(title)
{
}

// ──────────────────────────────────────────────────────────────────────────
int main()
{
   try {
      load_fonts();
      if(!graf_init_lfb()) return -1;

      EventManager ev;
      Event e;

      boxframe(0,0,799,599, true);
      buttonframe(515,3,796,514,false);

      AddHandler(new OpenFileHandler);
      AddHandler(new SaveFileHandler);
      AddHandler(new SaveFileAsHandler);
      AddHandler(new ExitHandler);
      AddHandler(new PaintImageHandler);

      e.what = ev_null; // init handlers
      CallHandlers(e);

      while(going)
      {
         ev.get_event(e);
         if(e.what != ev_null) CallHandlers(e);
      }

      graf_close();
   }
   catch(Exception& e)
   {
      graf_close();
      printf("Exception! %s\n", e.reason());
   }
   return 0;
}
