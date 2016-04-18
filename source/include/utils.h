#include "ttf/Font.hpp"
#include "lua/lua.hpp"
#include <citro3d.h>

// Old FS_dirent struct
typedef struct{
	u16 name[0x106];		 ///< UTF-16 encoded name
	u8	shortName[0x0A]; ///< 8.3 File name
	u8	shortExt[0x04];	///< 8.3 File extension (set to spaces for directories)
	u8	unknown2;				///< ???
	u8	unknown3;				///< ???
	u8	isDirectory;		 ///< Directory bit
	u8	isHidden;				///< Hidden bit
	u8	isArchive;			 ///< Archive bit
	u8	isReadOnly;			///< Read-only bit
	u64 fileSize;				///< File size
} FS_dirent;

// fileStream struct
struct fileStream{
	u32 magic;
	u32 handle;
	bool isRomfs;
};

// Timer struct
struct Timer{
	u32 magic;
	bool isPlaying;
	u64 tick;
};

// Font struct
struct ttf{
	u32 magic;
	Font f;
	unsigned char* buffer;
	u32 bufsize;
};

// GPU color struct
typedef struct{
	u32 magic;
	float r;
	float g;
	float b;
	float a;
} color;

// Vertex struct
typedef struct{
	float x;
	float y;
	float z;
	float t1;
	float t2;
	float n1;
	float n2;
	float n3;
} vertex;

// 3D Model struct
typedef struct{
	u32 magic;
	u8* vbo_data;
	u32 vertex_count;
	C3D_Tex* texture;
	C3D_Mtx* material;
} model;

// Vertex List struct
struct vertexList{
	vertex* vert;
	vertexList* next;
};

// Generic FS functions with romFs support
void FS_GetSize(fileStream* Handle, u64* size);
void FS_Read(fileStream* Handle, u32* bytesRead, u32 init, void* buffer, u32 size);
void FS_Close(fileStream* Handle);

// UTF-8 / ASCII compatibility functions
void utf2ascii(char* dst, u16* src);
void ascii2utf(u16* dst, char* src);

// Endianess conversion functions
u16 Endian_UInt16_Conversion(u16 value);
u32 Endian_UInt32_Conversion(u32 value);

// GPU / Normal color compatibility functions
void int2float(u32 color, float* r, float* g, float* b, float* a);
void float2int(color* base, u32* result);
u32 ARGB2RGBA(u32 color);

// Miscellaneous images utilities
void RBswap(u32* color);

// Image to Texture conversion utilities
u32 get_morton_offset(u32 x, u32 y, u32 bytes_per_pixel);
u32 morton_interleave(u32 x, u32 y);

// Utilities used by Core module for arguments parsing
void* luaL_checkbuffer(lua_State *L, int argv);