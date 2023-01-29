#pragma endian little
#include <std/mem.pat>

enum ChunkType : u32
{
	Geometry     = 0x0,
	Material     = 0x1,
	MeshInstance = 0x2,
	Texture      = 0x3,
	Transform    = 0x4,
	Unknown7         = 0x7,
	Group        = 0xA,
};

struct Chunk
{
	ChunkType Type;
	u32       Size;
	u8        Data[Size - 8];
};

struct Chunks
{
	Chunk chunks[while( !std::mem::eof() )][[inline]];
}[[inline]];

Chunks chunks @0x0;