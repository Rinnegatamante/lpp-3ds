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

// GPU color struct
typedef struct{
	u32 magic;
	float r;
	float g;
	float b;
	float a;
} color;

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