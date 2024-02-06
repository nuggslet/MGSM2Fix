#pragma once

#include "psx/gtereg.h"

typedef int (*M2FUNCTION)(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);

typedef struct M2_EmuPSX {
    struct M2_MethodsPSX *Methods;
    void *Components;
    void *Archive;
    void *ImageBIOS;
    void *ImageDRAM;
    void *MemoryDRAM;
    void *MemoryTCM;
    struct M2_EmuBusPSX *Bus;
    struct M2_EmuR3000 *DevR3000;
    void *DevCDROM;
    void *DevDMAC;
    struct M2_EmuGPU *DevGPU;
    void *DevINTC;
    void *DevMDEC;
    void *DevRTC;
    void *DevSIO;
    void *DevSPU;
    union {
        struct {
            // No more, but space for up to 16.
            unsigned char Wide;
            unsigned char HighP;
        } Flag;
        unsigned char Flags[16]; // "InfoIntegers".
    };

} M2_EmuPSX;

#ifndef _WIN64
static_assert(sizeof(M2_EmuPSX) == 0x54);
#endif

typedef struct M2_MethodsPSX {
    void (*Destroy)     (struct M2_EmuPSX *machine);
    void (*_Stub)       (struct M2_EmuPSX *machine);
    void (*Reload)      (struct M2_EmuPSX *machine, void *param);
    void (*Command)     (struct M2_EmuPSX *machine, int cmd, void *param);
    void (*Update)      (struct M2_EmuPSX *machine, int op);
} M2_MethodsPSX;

#ifndef _WIN64
static_assert(sizeof(M2_MethodsPSX) == 0x14);
#endif

typedef struct M2_EmuBusPSX {
    unsigned char  (*Read8)  (struct M2_EmuBusPSX *bus, unsigned int address);
    unsigned short (*Read16) (struct M2_EmuBusPSX *bus, unsigned int address);
    unsigned int   (*Read32) (struct M2_EmuBusPSX *bus, unsigned int address);
    void           (*Write8) (struct M2_EmuBusPSX *bus, unsigned int address, unsigned char value);
    void           (*Write16)(struct M2_EmuBusPSX *bus, unsigned int address, unsigned short value);
    void           (*Write32)(struct M2_EmuBusPSX *bus, unsigned int address, unsigned int value);
    unsigned int _Spare[2];
    struct M2_EmuPSX *Machine;
} M2_EmuBusPSX;

#ifndef _WIN64
static_assert(sizeof(M2_EmuBusPSX) == 0x24);
#endif

typedef struct M2_EmuCoprocGTE {
    union {
        struct {
            psxCP2Data CP2D;
            psxCP2Ctrl CP2C;
        };
        unsigned int Reg[64];
    };
    unsigned int Instruction;
    int (*Command)(struct M2_EmuCoprocGTE *gte, int id);
    int (*Result) (struct M2_EmuCoprocGTE *gte, int id);
} M2_EmuCoprocGTE;

#ifndef _WIN64
static_assert(sizeof(M2_EmuCoprocGTE) == 0x10C);
#endif

typedef struct M2_EmuR3000 {
    struct M2_MethodsR3000 *Methods;
    void *Components;
    struct M2_EmuBusPSX *Bus;
    void *DevINTC;
    struct M2_EmuCoprocGTE *CoprocGTE;
    M2FUNCTION *Segment; // The main/non-accelerated segment.
    bool           (*Execute)(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);
    unsigned char  (*Read8)  (struct M2_EmuR3000 *cpu, unsigned int address);
    unsigned short (*Read16) (struct M2_EmuR3000 *cpu, unsigned int address);
    unsigned int   (*Read32) (struct M2_EmuR3000 *cpu, unsigned int address);
    void           (*Write8) (struct M2_EmuR3000 *cpu, unsigned int address, unsigned char value);
    void           (*Write16)(struct M2_EmuR3000 *cpu, unsigned int address, unsigned short value);
    void           (*Write32)(struct M2_EmuR3000 *cpu, unsigned int address, unsigned int value);
    unsigned int Accelerator;
    bool           (*Step)   (struct M2_EmuR3000 *cpu, int cycle, unsigned int address);
    unsigned int Cycle;
    unsigned int _Spare; // Unused, leaks uninitialised memory.
    unsigned int Target;
    unsigned int Instruction;
    unsigned int State;
    unsigned int Hi;
    unsigned int Lo;
    unsigned int PC0;
    unsigned int PC1;
    unsigned int Reg[32];
    unsigned int SR;
    unsigned int Cause;
    unsigned int EPC;
    unsigned int OffsetDRAM;
    unsigned int SizeDRAM;
    unsigned char *BufferDRAM;
    M2FUNCTION *Segments[0x80000000 / 0x80000];
    void *MemoryVRAM; // What the fuck?
    unsigned int Cycles;
} M2_EmuR3000;

#ifndef _WIN64
static_assert(sizeof(M2_EmuR3000) == 0x4100);
#endif

typedef struct M2_MethodsR3000 {
    void (*Destroy)(struct M2_EmuR3000 *cpu);
    void (*_Stub)  (struct M2_EmuR3000 *cpu);
    void (*Reload) (struct M2_EmuR3000 *cpu);
    void (*Boot)   (struct M2_EmuR3000 *cpu, int op);
    void (*Command)(struct M2_EmuR3000 *cpu, int cmd, void *param);
    void (*Dump)   (struct M2_EmuR3000 *cpu);
    void (*Error)  (struct M2_EmuR3000 *cpu);
} M2_MethodsR3000;

#ifndef _WIN64
static_assert(sizeof(M2_MethodsR3000) == 0x1C);
#endif

typedef struct M2_EmuGPU {
    void *Methods;
    void *Components;
    void *MemoryVRAM;
    unsigned int VideoMode;
    unsigned int SceneMode;
    unsigned int State;
    unsigned int Status;
    int Result;
    unsigned int FIFO[16];
    unsigned int OffsetFIFO;
    unsigned short Operation[6];
    unsigned int ScreenAreaVRAM;
    unsigned int ScreenRangeW;
    unsigned int ScreenRangeH;
    void *Device;
} M2_EmuGPU;

#ifndef _WIN64
//static_assert(sizeof(M2_EmuGPU) == 0x2000EC);
#endif

enum class MgsGame
{
    Unknown,
    MGS1,
    MGSR,
};

typedef enum {
    PAD_BUTTON_A     =        0x1,
    PAD_BUTTON_B     =        0x2,
    PAD_BUTTON_C     =        0x4,
    PAD_BUTTON_X     =        0x8,
    PAD_BUTTON_Y     =       0x10,
    PAD_BUTTON_L     =       0x10,
    PAD_BUTTON_Z     =       0x20,
    PAD_BUTTON_R     =       0x20,
    PAD_DOWN         =       0x40,
    PAD_LEFT         =       0x80,
    PAD_RIGHT        =      0x100,
    PAD_COIN         =      0x200,
    PAD_SELECT       =      0x200,
    PAD_START        =      0x400,
    PAD_UP           =      0x800,
    PAD_DISCONNECTED =     0x1000,
    PAD_BUTTON_L2    =     0x4000,
    PAD_BUTTON_R2    =     0x8000,
    PAD_RPD_GC_POS   =  0x2000000,
    PAD_RPD_GC_NEG   =  0x4000000,
    PAD_SHORTCUT     = 0x20000000,
    PAD_MENU         = 0x40000000,
    PAD_RPD          = 0x80000000,
} M2EpiPadFlag;

typedef enum {
    MD_6B_DISABLE =  0x1,
    MD_MULTITAP_0 =  0x2,
    MD_MULTITAP_1 =  0x4,
    USE_ANALOG    =  0x8,
    DIRECTION_4   = 0x10,
} M2EpiArchSubInfo;

typedef enum {
    BUTTON_CIRCLE   =  0,
    BUTTON_CROSS    =  1,
    BUTTON_TRIANGLE =  2,
    BUTTON_SQUARE   =  3,
    BUTTON_R1       =  4,
    BUTTON_R2       =  5,
    BUTTON_R3       =  6,
    BUTTON_L1       =  7,
    BUTTON_L2       =  8,
    BUTTON_L3       =  9,
    BUTTON_SELECT   = 10,
    BUTTON_START    = 11,
    BUTTON_TOUCH    = 12,
} PlatformButtonId;
