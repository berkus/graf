//
// VBE 2.0 support
// Converted from Code by Olgerd E aka 2e
// Added some code from Submissive's VBE Core (visit www.cubic.org)
//
#include <i86.h>
#include <Mem.h>
#include <Bios.h>
#include <StdIo.h>
#include "Tailor.h"
#include "VBE.h"
#include "DPMI.h"
#include "Exception.h"

// Internal structures for VBE support

struct InfoBlock
{
   uint32  magic;             // VBE Signature
   uint16  version;           // VBE Version
   char   *oem_string;        // Pointer to OEM String
   uint32  caps;              // Capabilities of graphics controller
   uint16 *video_modes;       // Pointer to Video Mode List
                              // Added for VBE 1.2+
   uint16  total_memory;      // Number of 64kb memory blocks
                              // Added for VBE 2.0+
   uint16  oem_software_rev;  // VBE implementation Software revision
   char   *oem_vendor_name;   // Pointer to Vendor Name String
   char   *oem_product_name;  // Pointer to Product Name String
   char   *oem_product_rev;   // Pointer to Product Revision String
   uint16  accel_version;     // VBE/AF Version
   uint16 *accel_video_modes; // Pointer to Acclelerated Mode List
   uint8   reserved20[216];   // Reserved for VBE implementation scratch area
   uint8   oem_data[256];     // Data Area for OEM Strings
};

struct ModeInfoBlock
{
   uint16 attrs;                     // mode attributes
   uint8  a_attrs;                   // window A attributes
   uint8  b_attrs;                   // window B attributes
   uint16 gran;                      // window granularity
   uint16 wsize;                     // window size
   uint16 a_seg;                     // window A start segment
   uint16 b_seg;                     // window B start segment
   uint32 func;                      // pointer to window function
   uint16 bps;                       // bytes per scan line
                                     // Mandatory information for VBE 1.2 and above
   uint16 xres;                      // horizontal resolution in pixels or chars
   uint16 yres;                      // vertical resolution in pixels or chars
   uint8  XCharSize;                 // character cell width in pixels
   uint8  YCharSize;                 // character cell height in pixels
   uint8  NumberOfPlanes;            // number of memory planes
   uint8  bpp;                       // bits per pixel
   uint8  NumberOfBanks;             // number of banks
   uint8  MemoryModel;               // memory model type
   uint8  BankSize;                  // bank size in KB
   uint8  NumberOfImagePages;        // number of images
   uint8  Reserved12;                // reserved for page function
                                     // Direct Color fields (required
                                     // for direct/6 and YUV/7 memory models)
   uint8  RedMaskSize;               // size of direct color red mask in bits
   uint8  RedFieldPosition;          // bit position of lsb of red mask
   uint8  GreenMaskSize;             // size of direct color green mask in bits
   uint8  GreenFieldPosition;        // bit position of lsb of green mask
   uint8  BlueMaskSize;              // size of direct color blue mask in bits
   uint8  BlueFieldPosition;         // bit position of lsb of blue mask
   uint8  RsvdMaskSize;              // size of direct color reserved mask in bits
   uint8  RsvdFieldPosition;         // bit position of lsb of reserved mask
   uint8  DirectColorModeInfo;       // direct color mode attributes
                                     // Mandatory information for VBE 2.0 and above
   uint32 lfb_phys;                  // physical address for flat frame buffer
   uint32 OffScreenMemOffset;        // pointer to start of offscreen memory
   uint16 OffScreenMemSize;          // amount of offscreen memory in 1k units
   uint8  Reserved20[206];           // remainder of ModeInfoBlock
};

struct DPMI_PTR
{
   uint16 sel, seg;
};

//===========================================================================//

// Global module data
// PUBLICLY USED
bool                    DAC8 = false;
// PRIVATE
// Дальние (DPMI) указатели на структуры VBE.
DPMI_PTR                ib;
DPMI_PTR                mib;
// Ближние (транслированные) указатели.
InfoBlock              *vinfo;
ModeInfoBlock          *minfo;
// Регистры для вызова прерываний
union REGS              r;
struct SREGS            sr;
static DPMI::RMI        rmi;
// Маппинг физического адреса LFB для заданного режима
void                   *mapping;

//===========================================================================//

#define VBE_INT  0x10

inline void clear_regs()
{
   memset(&r,   0, sizeof r);
   memset(&sr,  0, sizeof sr);
   memset(&rmi, 0, sizeof rmi);
}

static void *ptr_to_linear(void *phys)
{
   return (void *)((((uint32)phys>>16)<<4)+(uint16)phys);
}

// Распределение памяти для библиотеки. Startup.
void vbe_Init()
{
   if(!DPMI::dos_alloc(512 / 16, ib.seg, ib.sel))
      throw Exception("VBE: Allocation of InfoBlock failed");
   if(!DPMI::dos_alloc(256 / 16, mib.seg, mib.sel))
      throw Exception("VBE: Allocation of ModeInfoBlock failed");
   vinfo = (InfoBlock *)(ib.seg << 4);
   minfo = (ModeInfoBlock *)(mib.seg << 4);

   // Get VBE info block
   memset(vinfo, 0, sizeof InfoBlock);
   vinfo->magic = MAGIC('V','B','E','2');

   clear_regs();
   rmi.EAX = 0x00004f00;   // Get VBE Info
   rmi.ES = ib.seg;
   rmi.EDI = 0;
   if(!DPMI::real_int(VBE_INT, &rmi)
   || (uint16)rmi.EAX != 0x004F) throw Exception("VBE: Failed to get VBE info");
   if(vinfo->version < 0x0200) throw Exception("VBE: Version 2.0+ required");

   // Translate the Realmode Pointers into flat-memory address space
   vinfo->oem_string        = (char   *)ptr_to_linear(vinfo->oem_string);
   vinfo->video_modes       = (uint16 *)ptr_to_linear(vinfo->video_modes);
   vinfo->oem_vendor_name   = (char   *)ptr_to_linear(vinfo->oem_vendor_name);
   vinfo->oem_product_name  = (char   *)ptr_to_linear(vinfo->oem_product_name);
   vinfo->oem_product_rev   = (char   *)ptr_to_linear(vinfo->oem_product_rev);
   vinfo->accel_video_modes = (uint16 *)ptr_to_linear(vinfo->accel_video_modes);
}

// Освобождение памяти. Cleanup.
void vbe_Done()
{
   if(mapping) DPMI::unmap_linear((uint32)mapping);
   if(!DPMI::dos_free(ib.sel))
      throw Exception("VBE: Freeing of InfoBlock failed");
   if(!DPMI::dos_free(mib.sel))
      throw Exception("VBE: Freeing of ModeInfoBlock failed");
}

// Заполняет minfo информацией о режиме mode
static void vbe_GetModeInfo(uint16 mode)
{
   clear_regs();
   rmi.EAX = 0x00004f01;
   rmi.ECX = mode;
   rmi.ES = mib.seg;
   rmi.EDI = 0;
   if(!DPMI::real_int(VBE_INT, &rmi)
   || (uint16)rmi.EAX != 0x004F) throw Exception("VBE: Failed to get mode info");
}

static bool vbe_IsLinear(uint16 mode) // make PUBLIC
{
   vbe_GetModeInfo(mode);
   return minfo->attrs & 128;
}

uint16 vbe_FindMode(uint16 hres, uint16 vres, uint8 bpp)
{
   int i, real_bpp; // true number of bits/pixel... fix lots of buggy bioses
   // find a mode in the VBE modes list
   // terminates on a -1 (vesa spec) and on a 0 (lousy bioses)
   for(i = 0; ((vinfo->video_modes[i] != 0xffff) &&
               (vinfo->video_modes[i] != 0)); i++)
   {
      vbe_GetModeInfo(vinfo->video_modes[i]);
      // use the field-masks to calculate the actual bit-size if we
      // are searching for a 15 or 16 bit mode (fix for lousy bioses)
      if((bpp == 15) || (bpp == 16))
      {
         real_bpp = minfo->RedMaskSize   +
                    minfo->GreenMaskSize +
                    minfo->BlueMaskSize;
      }
      else
         real_bpp = minfo->bpp;

      if((hres == minfo->xres) &&
         (vres == minfo->yres) &&
         (bpp  == real_bpp)) return vinfo->video_modes[i];
   }
   return 0;
}

void setbiosmode(uint16 c);
#pragma aux setbiosmode = "int 0x10" parm[ax] modify[eax ebx ecx edx esi edi];

bool vbe_SetMode(uint16 mode, bool lfb, bool clear)
{
   static char notinit[] = "VBE: Error initializing mode";
   uint16 rawmode;

   if(mode == 3) // set textmode using BIOS
   {
      setbiosmode(3);
      return true;
   }
   rawmode = mode & 0x0fff;
   if(lfb) rawmode    |= 1<<14;
   if(!clear) rawmode |= 1<<15;

   // S3FIX: If both lfb and clear are set, we should first set
   // the mode without lfb but with clear, then with lfb but without clear..
   // S3 cards really suffer from this trouble...sure, they suck..
   if(lfb && clear)
   {
      clear_regs();
      r.w.ax = 0x4F02;
      r.w.bx = mode & 0x0fff;  // clear w/o lfb
      int386(VBE_INT, &r, &r);
      if(r.w.ax != 0x004f) throw Exception(notinit);

      clear_regs();
      r.w.ax = 0x4F02;
      r.w.bx = mode | (1<<14) | (1<<15); // lfb w/o clear
      int386(VBE_INT, &r, &r);
      if(r.w.ax != 0x004f) throw Exception(notinit);
   }
   else
   {
      clear_regs();
      r.w.ax = 0x4F02;
      r.w.bx = rawmode;
      int386(VBE_INT, &r, &r);
      if(r.w.ax != 0x004f) throw Exception(notinit);
   }

   return true;
}

void *vbe_GetLFB(uint16 mode)
{
   vbe_GetModeInfo(mode);
   if(mapping)
   {
      DPMI::unmap_linear((uint32)mapping);
      mapping = 0;
   }
   uint32 t = DPMI::map_linear(minfo->lfb_phys, vinfo->total_memory*64*1024);
   if(t != -1) mapping = (void *)t;
   return mapping;
}

bool vbe_SetLogicalLineLength(uint16 pixels)
{
   clear_regs();
   r.w.ax = 0x4F06;
   r.h.bl = 0x00;
   r.w.cx = pixels;
   int386(VBE_INT, &r, &r);

   // if vbe error or scanline len is not as requested, return false
   return !((r.w.ax != 0x004F) || (r.w.cx != pixels));
}

// Возвращает только то, что об этом думает VBE
bool vbe_8bitDAC()
{
   return vinfo->caps & 1;
}

void vbe_SetDACWidth(uint8 bits)
{
   clear_regs();
   r.w.ax = 0x4F08;
   r.w.bx = bits << 8;
   int386(VBE_INT, &r, &r);
   DAC8 = false;
   if(r.w.ax != 0x004F) return; // DAC failed to switch
   if(bits == 8) DAC8 = true;   // DAC switched to 8 bits mode
}

void vbe_SetPalette(rgb *p, uint16 start, uint16 count)
{
   rgb temp[256];
   memcpy(temp, p, count*sizeof(rgb));
   if(!DAC8)
   {
      for(int i = 0; i < count; i++)
      {
         temp[i].r >>= 2;
         temp[i].g >>= 2;
         temp[i].b >>= 2;
      }
   }
   clear_regs();
   r.w.ax = 0x4F09;
   r.h.bl = 0;
   r.w.dx = start;
   r.w.cx = count;
   sr.es = FP_SEG(temp);
   r.x.edi = FP_OFF(temp);
   int386x(VBE_INT, &r, &r, &sr);
}

void vbe_GetPalette(rgb *p, uint16 start, uint16 count)
{
   rgb temp[256];

   clear_regs();
   r.w.ax = 0x4F09;
   r.h.bl = 1;
   r.w.dx = start;
   r.w.cx = count;
   sr.es = FP_SEG(temp);
   r.x.edi = FP_OFF(temp);
   int386x(VBE_INT, &r, &r, &sr);

   if(!DAC8)
   {
      for(int i = 0; i < count; i++)
      {
         temp[i].r <<= 2;
         temp[i].g <<= 2;
         temp[i].b <<= 2;
      }
   }
   memcpy(p, temp, count*sizeof(rgb));
}
