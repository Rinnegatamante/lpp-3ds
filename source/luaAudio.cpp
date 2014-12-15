#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "include/luaAudio.h"

//Custom CSND_playsound to prevent audio desynchronization and enable stereo sounds
void My_CSND_playsound(u32 channel, u32 looping, u32 encoding, u32 samplerate, u32 *vaddr0, u32 *vaddr1, u32 totalbytesize, u32 l_vol, u32 r_vol)
{
u32 physaddr0 = 0;
u32 physaddr1 = 0;
physaddr0 = osConvertVirtToPhys((u32)vaddr0);
physaddr1 = osConvertVirtToPhys((u32)vaddr1);
CSND_sharedmemtype0_cmde(channel, looping, encoding, samplerate, 2, 0, physaddr0, physaddr1, totalbytesize);
CSND_sharedmemtype0_cmd8(channel, samplerate);
if(looping)
{
if(physaddr1>physaddr0)totalbytesize-= (u32)physaddr1 - (u32)physaddr0;
CSND_sharedmemtype0_cmd3(channel, physaddr1, totalbytesize);
}
CSND_sharedmemtype0_cmd8(channel, samplerate);
u32 cmdparams[0x18>>2];
memset(cmdparams, 0, 0x18);
cmdparams[0] = channel & 0x1f;
cmdparams[1] = l_vol | (r_vol<<16);
CSND_writesharedmem_cmdtype0(0x9, (u8*)&cmdparams);
}