#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

#include <3ds.h>
#if INCLUDE_INTTYPES_H
#  include <inttypes.h>
#endif
#if INCLUDE_STDINT_H
#  include <stdint.h>
#endif
#if INCLUDE_SYS_TYPES_H
#  include <sys/types.h>
#endif

typedef s16 ogg_int16_t;
typedef u16 ogg_uint16_t;
typedef s32 ogg_int32_t;
typedef u32 ogg_uint32_t;
typedef s64 ogg_int64_t;

#endif
