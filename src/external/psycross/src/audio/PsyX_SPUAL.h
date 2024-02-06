#ifndef PSYX_SPUAL_H
#define PSYX_SPUAL_H

#include "psx/types.h"
#include "psx/libspu.h"
#include "PsyX/PsyX_config.h"
#include "PsyX/common/pgxp_defs.h"

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif

extern int PsyX_SPUAL_InitSound();
extern void PsyX_SPUAL_ShutdownSound();

// Private
extern long PsyX_SPUAL_Alloc(long size);
extern long PsyX_SPUAL_InitAlloc(long num, char* top);
extern void PsyX_SPUAL_Free(u_long addr);
extern u_long PsyX_SPUAL_Write(u_char* addr, u_long size);
extern u_long PsyX_SPUAL_Read(u_char* addr, u_long size);
extern u_long PsyX_SPUAL_SetTransferStartAddr(u_long addr);

extern void PsyX_SPUAL_GetVoiceVolume(int vNum, short* volL, short* volR);
extern void PsyX_SPUAL_GetVoicePitch(int vNum, u_short* pitch);
extern void PsyX_SPUAL_SetVoiceAttr(SpuVoiceAttr* psxAttrib);
extern void PsyX_SPUAL_SetKey(long on_off, u_long voice_bit);

extern int PsyX_SPUAL_GetKeyStatus(u_long voice_bit);
extern void PsyX_SPUAL_GetAllKeysStatus(char* status);

extern long PsyX_SPUAL_SetMute(long on_off);
extern long PsyX_SPUAL_SetReverb(long on_off);
extern long PsyX_SPUAL_GetReverbState();
extern u_long PsyX_SPUAL_SetReverbVoice(long on_off, u_long voice_bit);
extern u_long PsyX_SPUAL_GetReverbVoice();

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif

#endif // PSYX_SPUAL_H