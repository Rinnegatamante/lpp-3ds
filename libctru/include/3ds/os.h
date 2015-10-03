#pragma once

#define SYSTEM_VERSION(major, minor, revision) \
	(((major)<<24)|((minor)<<16)|((revision)<<8))

#define GET_VERSION_MAJOR(version)    ((version) >>24)
#define GET_VERSION_MINOR(version)    (((version)>>16)&0xFF)
#define GET_VERSION_REVISION(version) (((version)>> 8)&0xFF)

u32 osConvertVirtToPhys(u32 vaddr);
u32 osConvertOldLINEARMemToNew(u32 addr);//Converts 0x14* vmem to 0x30*. Returns the input addr when it's already within the new vmem. Returns 0 when outside of either LINEAR mem areas.
const char* osStrError(u32 error);
u32 osGetFirmVersion();
u32 osGetKernelVersion();
u64 osGetTime();
