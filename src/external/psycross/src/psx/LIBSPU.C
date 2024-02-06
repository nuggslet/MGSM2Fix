#define HAVE_M_PI
#include "../PsyX_main.h"
#include "../audio/PsyX_SPUAL.h"

unsigned long SpuWrite(unsigned char* addr, unsigned long size)
{
	return PsyX_SPUAL_Write(addr, size);
}

unsigned long SpuRead(unsigned char* addr, unsigned long size)
{
	return PsyX_SPUAL_Read(addr, size);
}

long SpuSetTransferMode(long mode)
{
	// TODO: handle different transfer modes?

	long mode_fix = mode == 0 ? 0 : 1;

	//trans_mode = mode;
	//transMode = mode_fix;

	return mode_fix;
}

unsigned long SpuSetTransferStartAddr(unsigned long addr)
{
	return PsyX_SPUAL_SetTransferStartAddr(addr);
}

long SpuIsTransferCompleted(long flag)
{
	return 1;
}

void SpuInit(void)
{
	ResetCallback();
	PsyX_SPUAL_InitSound();
}

void SpuQuit(void)
{
	PsyX_SPUAL_ShutdownSound();
}

void SpuSetVoiceAttr(SpuVoiceAttr *arg)
{
	PsyX_SPUAL_SetVoiceAttr(arg);
}

void SpuSetKey(long on_off, unsigned long voice_bit)
{
	PsyX_SPUAL_SetKey(on_off, voice_bit);
}

long SpuGetKeyStatus(unsigned long voice_bit)
{
	return PsyX_SPUAL_GetKeyStatus(voice_bit);
}

void SpuGetAllKeysStatus(char* status)
{
	PsyX_SPUAL_GetAllKeysStatus(status);
}

void SpuSetKeyOnWithAttr(SpuVoiceAttr* attr)
{
	SpuSetVoiceAttr(attr);
	SpuSetKey(SPU_ON, attr->voice);
}

long SpuSetMute(long on_off)
{
	return PsyX_SPUAL_SetMute(on_off);
}

long SpuSetReverb(long on_off)
{
	return PsyX_SPUAL_SetReverb(on_off);
}

long SpuGetReverb(void)
{
	return PsyX_SPUAL_GetReverbState();
}

long SpuSetReverbModeParam(SpuReverbAttr* attr)
{
	PSYX_UNIMPLEMENTED();
	return 0;
}

void SpuGetReverbModeParam(SpuReverbAttr* attr)
{
	PSYX_UNIMPLEMENTED();
}

long SpuSetReverbDepth(SpuReverbAttr* attr)
{
	PSYX_UNIMPLEMENTED();
	return 0;
}

long SpuReserveReverbWorkArea(long on_off)
{
	return 1;
}

long SpuIsReverbWorkAreaReserved(long on_off)
{
	PSYX_UNIMPLEMENTED();
	return 0;
}

unsigned long SpuSetReverbVoice(long on_off, unsigned long voice_bit)
{
	return PsyX_SPUAL_SetReverbVoice(on_off, voice_bit);
}

unsigned long SpuGetReverbVoice(void)
{
	return PsyX_SPUAL_GetReverbVoice();
}

long SpuClearReverbWorkArea(long mode)
{
	PSYX_UNIMPLEMENTED();
	return 0;
}

void SpuSetCommonAttr(SpuCommonAttr* attr)
{
	PSYX_UNIMPLEMENTED();
}

long SpuInitMalloc(long num, char* top)
{
	return PsyX_SPUAL_InitAlloc(num, top);
}

long SpuMalloc(long size)
{
	return PsyX_SPUAL_Alloc(size);
}

long SpuMallocWithStartAddr(unsigned long addr, long size)
{
	PSYX_UNIMPLEMENTED();
	return 0;
}

void SpuFree(unsigned long addr)
{
	PsyX_SPUAL_Free(addr);
}

unsigned long SpuFlush(unsigned long ev)
{
	//PSYX_UNIMPLEMENTED();
	return 0;
}

void SpuSetCommonMasterVolume(short mvol_left, short mvol_right)// (F)
{
	//MasterVolume.VolumeLeft.Raw = mvol_left;
	//MasterVolume.VolumeRight.Raw = mvol_right;
	PSYX_UNIMPLEMENTED();
}

long SpuSetReverbModeType(long mode)
{
	PSYX_UNIMPLEMENTED();
	return 0;
}

void SpuSetReverbModeDepth(short depth_left, short depth_right)
{
	PSYX_UNIMPLEMENTED();
}

void SpuGetVoiceVolume(int vNum, short* volL, short* volR)
{
	PsyX_SPUAL_GetVoiceVolume(vNum, volL, volR);
}

void SpuGetVoicePitch(int vNum, unsigned short* pitch)
{
	PsyX_SPUAL_GetVoicePitch(vNum, pitch);
}

#define VOICE_ATTRIB_SETTER_SHORTCUT(flag, field, value) \
	SpuVoiceAttr attr; \
	attr.voice = SPU_VOICECH(vNum); \
	attr.mask = flag; \
	attr.field = value; \
	SpuSetVoiceAttr(&attr)

void SpuSetVoiceVolume(int vNum, short volL, short volR)
{
	SpuVoiceAttr attr;

	attr.mask = SPU_VOICE_VOLL | SPU_VOICE_VOLR;
	attr.voice = SPU_VOICECH(vNum);
	attr.volume.left = volL;
	attr.volume.right = volR;

	SpuSetVoiceAttr(&attr);
}

void SpuSetVoicePitch(int vNum, unsigned short pitch)
{
	VOICE_ATTRIB_SETTER_SHORTCUT(SPU_VOICE_PITCH, pitch, pitch);
}

void SpuSetVoiceStartAddr(int vNum, unsigned long startAddr)
{
	VOICE_ATTRIB_SETTER_SHORTCUT(SPU_VOICE_WDSA, addr, startAddr);
}

void SpuSetVoiceAR(int vNum, unsigned short AR)
{
	VOICE_ATTRIB_SETTER_SHORTCUT(SPU_VOICE_ADSR_AR, ar, AR);
}

extern void SpuSetVoiceDR(int vNum, unsigned short DR)
{
	VOICE_ATTRIB_SETTER_SHORTCUT(SPU_VOICE_ADSR_DR, dr, DR);
}

extern void SpuSetVoiceSR(int vNum, unsigned short SR)
{
	VOICE_ATTRIB_SETTER_SHORTCUT(SPU_VOICE_ADSR_SR, sr, SR);
}

void SpuSetVoiceRR(int vNum, unsigned short RR)
{
	VOICE_ATTRIB_SETTER_SHORTCUT(SPU_VOICE_ADSR_RR, rr, RR);
}

extern void SpuSetVoiceSL(int vNum, unsigned short SL)
{
	VOICE_ATTRIB_SETTER_SHORTCUT(SPU_VOICE_ADSR_SL, sl, SL);
}

void SpuSetVoiceADSRAttr(int vNum,
	unsigned short AR, unsigned short DR,
	unsigned short SR, unsigned short RR,
	unsigned short SL,
	long ARmode, long SRmode, long RRmode)
{
	SpuVoiceAttr attr;

	attr.mask = SPU_VOICE_ADSR_AR | SPU_VOICE_ADSR_DR | 
				SPU_VOICE_ADSR_SR | SPU_VOICE_ADSR_RR | 
				SPU_VOICE_ADSR_SL |
				SPU_VOICE_ADSR_AMODE | SPU_VOICE_ADSR_SMODE | SPU_VOICE_ADSR_RMODE;

	attr.voice = SPU_VOICECH(vNum);
	attr.ar = AR;
	attr.dr = DR;
	attr.sr = SR;
	attr.rr = RR;
	attr.sl = SL;
	attr.a_mode = ARmode;
	attr.s_mode = SRmode;
	attr.r_mode = RRmode;

	SpuSetVoiceAttr(&attr);
}

SpuTransferCallbackProc SpuSetTransferCallback(SpuTransferCallbackProc func)
{
	PSYX_UNIMPLEMENTED();
	return 0;
}

long SpuReadDecodedData(SpuDecodedData * d_data, long flag)
{
	PSYX_UNIMPLEMENTED();
	return 0;
}

long SpuSetIRQ(long on_off)
{
	PSYX_UNIMPLEMENTED();
	return 0;
}

unsigned long SpuSetIRQAddr(unsigned long x)
{
	PSYX_UNIMPLEMENTED();
	return 0;
}

SpuIRQCallbackProc SpuSetIRQCallback(SpuIRQCallbackProc x)
{
	PSYX_UNIMPLEMENTED();
	return 0;
}

void SpuSetCommonCDMix(long cd_mix)
{
	PSYX_UNIMPLEMENTED();
}

void SpuSetCommonCDVolume(short cd_left, short cd_right)
{
	PSYX_UNIMPLEMENTED();
}

void SpuSetCommonCDReverb(long cd_reverb)
{
	PSYX_UNIMPLEMENTED();
}
