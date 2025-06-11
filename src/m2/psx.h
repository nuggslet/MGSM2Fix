#pragma once

typedef int (*PSXFUNCTION)(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);

typedef struct M2_EmuPSX {
    struct M2_MethodsPSX *Methods;
    void *Components;
    void *Archive;
    unsigned int *ImageBIOS;
    unsigned int *ImageDRAM;
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
    unsigned char  (_cdecl *Read8)  (struct M2_EmuBusPSX *bus, unsigned int address);
    void           (_cdecl *Write8) (struct M2_EmuBusPSX *bus, unsigned int address, unsigned char value);
    unsigned short (_cdecl *Read16) (struct M2_EmuBusPSX *bus, unsigned int address);
    void           (_cdecl *Write16)(struct M2_EmuBusPSX *bus, unsigned int address, unsigned short value);
    unsigned int   (_cdecl *Read32) (struct M2_EmuBusPSX *bus, unsigned int address);
    void           (_cdecl *Write32)(struct M2_EmuBusPSX *bus, unsigned int address, unsigned int value);
    unsigned int _Spare[2];
    struct M2_EmuPSX *Machine;
} M2_EmuBusPSX;

#ifndef _WIN64
static_assert(sizeof(M2_EmuBusPSX) == 0x24);
#endif

typedef struct M2_EmuCoprocGTE {
    unsigned int Reg[64];
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
    PSXFUNCTION *Segment; // The main/non-accelerated segment.
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
    PSXFUNCTION *Segments[0x80000000 / 0x80000];
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
    unsigned short *MemoryVRAM;
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
    unsigned int _80[23];
    struct M2_SceneGPU *Scene;
    unsigned int _E0[2];
    unsigned short BufferVRAM[0x100000];
    unsigned int OffsetVRAM;
} M2_EmuGPU;

#ifndef _WIN64
static_assert(sizeof(M2_EmuGPU) == 0x2000EC);
#endif

typedef struct M2_SceneGPU {
    unsigned int Type;
    unsigned int *CommandBuffer;
    unsigned int Command;
    unsigned int *VertexBuffer;
    unsigned int Vertex;
    unsigned int _14[34];
    unsigned int TextureWindow;
    unsigned short TexturePage;
    unsigned short _A2[5];
    unsigned int DrawPosition;
    unsigned int DrawTopLeft;
    unsigned int DrawBottomRight;
    unsigned int DrawOffset;
    unsigned int _BC;
    unsigned int DrawOffsetReset;
    unsigned int DrawAreaReset;
    unsigned int _C8[20];
    void *Buffer;
    unsigned int _11C[3];
    unsigned int VideoMode;
    unsigned int _12C[2];
    unsigned int MaskBit;
    unsigned int _138;
} M2_SceneGPU;

#ifndef _WIN64
static_assert(sizeof(M2_SceneGPU) == 0x13C);
#endif

typedef struct {
    unsigned int address;
    PSXFUNCTION handler;
} M2_EmuPSX_TableEntry;

typedef struct {
    const char *name;
    void *create;
} M2_EmuPSX_SystemModule;

typedef struct {
    const char *name;
    int id;
    M2_EmuPSX_TableEntry *table;
    int count;
} M2_EmuPSX_KernelModule;

typedef struct {
    const char *name;
    int id;
    M2_EmuPSX_TableEntry *table;
    int count;
    void *deps;
} M2_EmuPSX_UserModule;

typedef struct {
    const char *name;
    int type;
    union {
        M2_EmuPSX_SystemModule *system;
        M2_EmuPSX_KernelModule *kernel;
        M2_EmuPSX_UserModule *user;
    };
} M2_EmuPSX_Module;
