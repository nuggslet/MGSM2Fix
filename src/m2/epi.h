#pragma once

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
