#include <TsuHan/TsuHan.hpp>

#include <cstdarg>
#include <cstring>
#include <string_view>

namespace TsuHan
{

namespace HGM
{

const char* ToString(TagID Tag)
{
	switch( Tag )
	{
	case TagID::Geometry:
		return "Geometry";
	case TagID::Material:
		return "Material";
	case TagID::Mesh:
		return "Mesh";
	case TagID::Texture:
		return "Texture";
	case TagID::Transform:
		return "Transform";
	case TagID::Unknown7:
		return "Unknown7";
	case TagID::Unknown8:
		return "Unknown8";
	case TagID::Unknown9:
		return "Unknown9";
	case TagID::Unknown10:
		return "Unknown10";
	case TagID::Bone:
		return "Bone";
	default:
		return "Unknown";
	}
}

std::size_t GetVertexBufferStride(std::uint16_t VertexAttributeMask)
{
	constexpr std::array<std::uint32_t, 15> VertexAttributeSize
		= {12, 12, 12, 12, 16, 16, 4, 4, 4, 4, 16, 8, 8, 8, 8};

	std::size_t Result = 0;

	for( std::uint8_t i = 0; i < 16; ++i )
	{
		if( (VertexAttributeMask >> i) & 1 )
		{
			Result += VertexAttributeSize[i];
		}
	}

	return Result;
}

std::span<const std::byte>
	ReadFormattedBytes(std::span<const std::byte> Bytes, const char* Format...)
{
	va_list Args;
	va_start(Args, Format);

	for( const char& Token : std::string_view(Format) )
	{
		if( Bytes.empty() )
		{
			return Bytes;
		}

		switch( std::tolower(Token) )
		{
		case 's':
		{
			const std::size_t StringLength
				= std::strlen((const char*)Bytes.data());

			const std::size_t StringLengthAligned
				= 4 * ((StringLength - 1) / 4) + 4;

			char* Value = va_arg(Args, char*);

			std::memcpy(Value, Bytes.data(), StringLength);

			Bytes = Bytes.subspan(StringLengthAligned);
			break;
		}
		case 'l':
		case 'f':
		{
			std::uint32_t* Value = va_arg(Args, std::uint32_t*);
			std::memcpy(Value, Bytes.data(), sizeof(std::uint32_t));

			Bytes = Bytes.subspan(sizeof(std::uint32_t));
			break;
		}
		}
	}
	va_end(Args);

	return Bytes;
}

void HGMHandler(
	std::span<const std::byte> FileData, std::filesystem::path FilePath
)
{
	while( FileData.size() )
	{
		const Chunk& CurChunk
			= *reinterpret_cast<const Chunk*>(FileData.data());

		const std::span<const std::byte> CurChunkData
			= FileData.subspan(sizeof(Chunk), CurChunk.Size);

		std::printf("%s(%u)\n", ToString(CurChunk.Tag), CurChunk.Size);

		std::printf("\t-%s\n", (const char*)CurChunkData.data());

		switch( CurChunk.Tag )
		{
		case TagID::Geometry:
		{
			break;
		}
		case TagID::Material:
		{
			break;
		}
		case TagID::Mesh:
		{
			break;
		}
		case TagID::Texture:
		{
			// ssllllll
			break;
		}
		case TagID::Transform:
		{
			// sllllllllll
			break;
		}
		case TagID::Unknown7:
		{
			break;
		}
		case TagID::Unknown8:
		{
			break;
		}
		case TagID::Unknown9:
		{
			break;
		}
		case TagID::Unknown10:
		{
			break;
		}
		case TagID::Bone:
		{
			// sllllllllll
			break;
		}
		default:
		{
			break;
		}
		}

		FileData = FileData.subspan(CurChunk.Size);
	}
}
} // namespace HGM
} // namespace TsuHan