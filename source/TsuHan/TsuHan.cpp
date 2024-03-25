#include <TsuHan/TsuHan.hpp>

#include <cstdarg>
#include <cstring>
#include <regex>
#include <string_view>

#include "tiny_gltf.h"

#include <mio/mmap.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/range.hpp>

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
	case TagID::SceneDescriptor:
		return "SceneDescriptor";
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
		if( ((VertexAttributeMask >> i) & 1) != 0 )
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

			const std::size_t StringLengthAligned = 4 * (StringLength / 4) + 4;

			char* Value = va_arg(Args, char*);

			std::memcpy(Value, Bytes.data(), StringLengthAligned);

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
		default:
		{
			break;
		}
		}
	}
	va_end(Args);

	return Bytes;
}

namespace
{
std::span<const std::byte>
	PrintFormattedBytes(std::span<const std::byte> Bytes, const char* Format)
{
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

			const std::size_t StringLengthAligned = 4 * (StringLength / 4) + 4;

			char String[256];
			std::memcpy(String, Bytes.data(), StringLengthAligned);

			std::printf(
				"\t %%s \'%.*s\'\n", (std::uint32_t)StringLength, String
			);
			Bytes = Bytes.subspan(StringLengthAligned);
			break;
		}
		case 'l':
		{
			std::uint32_t Integer;
			std::memcpy(&Integer, Bytes.data(), sizeof(std::uint32_t));
			std::printf("\t %%l %d(0x%08x)\n", Integer, Integer);
			Bytes = Bytes.subspan(sizeof(std::uint32_t));
			break;
		}
		case 'f':
		{
			float Float;
			std::memcpy(&Float, Bytes.data(), sizeof(float));
			std::printf("\t %%f %f\n", Float);
			Bytes = Bytes.subspan(sizeof(float));
			break;
		}
		}
	}
	return Bytes;
}

template<class ForwardIt>
ForwardIt max_element_nth(ForwardIt first, ForwardIt last, int n)
{
	if( first == last )
	{
		return last;
	}
	ForwardIt largest = first;
	first += n;
	for( ; first < last; first += n )
	{
		if( *largest < *first )
		{
			largest = first;
		}
	}
	return largest;
}
} // namespace

void HGMHandler(
	std::span<const std::byte> FileData, std::filesystem::path FilePath
)
{
	tinygltf::Asset GLTFAsset{};
	GLTFAsset.generator = "TsuHanTools:" __TIMESTAMP__;
	GLTFAsset.version   = "2.0";

	tinygltf::Model GLTFModel;
	GLTFModel.extensionsUsed = {
		"KHR_materials_unlit",
	};
	GLTFModel.extensionsRequired = {
		// Unlit not needed
	};

	tinygltf::Scene GLTFScene;
	GLTFScene.name = FilePath.filename();
	GLTFScene.nodes.push_back(0);

	GLTFModel.scenes.push_back(GLTFScene);

	std::unordered_map<std::string, std::array<std::int32_t, 16>> GeometryLUT;
	std::unordered_map<std::string, std::uint32_t>                MeshLUT;
	std::unordered_map<std::string, std::uint32_t>                MaterialLUT;
	std::unordered_map<std::string, std::uint32_t>                TextureLUT;
	std::unordered_map<std::string, std::uint32_t>                TransformLUT;

	while( !FileData.empty() )
	{
		const Chunk& CurChunk
			= *reinterpret_cast<const Chunk*>(FileData.data());

		std::span<const std::byte> CurChunkData
			= FileData.subspan(sizeof(Chunk), CurChunk.Size);

		std::printf("%s(%u)\n", ToString(CurChunk.Tag), CurChunk.Size);

		std::printf("\\%s\n", (const char*)CurChunkData.data());

		switch( CurChunk.Tag )
		{
		case TagID::Geometry:
		{
			// slllllll
			struct GeometryHeader
			{
				char          Name[256] = {};
				float         UnknownA;
				float         UnknownB;
				float         UnknownC;
				float         UnknownD;
				std::uint32_t UnknownE; // UnknownFlag
				std::uint32_t VertexAttributeMask;

				// Not sure what this indicates but when non-zero then it skips
				// loading all geometry data from the file.
				std::uint32_t UnknownSkip;
			} Header;
			PrintFormattedBytes(CurChunkData, "sfffflll");
			CurChunkData = ReadFormattedBytes(
				CurChunkData, "sfffflll", Header.Name, &Header.UnknownA,
				&Header.UnknownB, &Header.UnknownC, &Header.UnknownD,
				&Header.UnknownE, &Header.VertexAttributeMask,
				&Header.UnknownSkip
			);

			if( Header.UnknownSkip != 0U )
			{
				break;
			}

			std::uint32_t VertexCount;
			PrintFormattedBytes(CurChunkData, "l");
			CurChunkData = ReadFormattedBytes(CurChunkData, "l", &VertexCount);
			const std::size_t VertexDataSize
				= GetVertexBufferStride(Header.VertexAttributeMask)
				* VertexCount;

			// Vertex data
			const std::span<const std::byte> VertexData
				= CurChunkData.first(VertexDataSize);

			// Add vertex data to gltf
			std::int32_t VertexPositionAccessorIdx = -1;
			std::int32_t VertexNormalAccessorIdx   = -1;
			std::int32_t VertexTangentAccessorIdx  = -1;
			std::int32_t VertexColorAccessorIdx    = -1;
			std::int32_t VertexWeightsAccessorIdx  = -1;
			std::int32_t VertexJointsAccessorIdx   = -1;
			std::int32_t VertexTexCoordAccessorIdx = -1;
			{
				tinygltf::Buffer VertexBuffer;
				VertexBuffer.name
					= VertexBuffer.name + Header.Name + ": VertexBuffer";
				std::transform(
					VertexData.begin(),
					VertexData.end(),
					std::back_inserter(VertexBuffer.data),
					std::to_integer<unsigned char>
				);
				GLTFModel.buffers.push_back(VertexBuffer);

				std::span<const float> FloatData(
					(const float*)VertexData.data(),
					VertexData.size() / sizeof(float)
				);

				tinygltf::BufferView VertexBufferView;
				VertexBufferView.buffer     = GLTFModel.buffers.size() - 1;
				VertexBufferView.byteOffset = 0;
				VertexBufferView.byteLength = VertexDataSize;
				VertexBufferView.byteStride
					= GetVertexBufferStride(Header.VertexAttributeMask);
				VertexBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
				GLTFModel.bufferViews.push_back(VertexBufferView);

				// Positions
				if( const std::uint32_t AttribMask = 0b0000'0'0000'00'0001;
					Header.VertexAttributeMask & AttribMask )
				{

					tinygltf::Accessor PositionAccessor;
					PositionAccessor.bufferView
						= GLTFModel.bufferViews.size() - 1;
					PositionAccessor.byteOffset = GetVertexBufferStride(
						(AttribMask - 1) & Header.VertexAttributeMask
					);
					PositionAccessor.maxValues = {+5.0, +5.0, +5.0};
					PositionAccessor.minValues = {-5.0, -5.0, -5.0};
					PositionAccessor.componentType
						= TINYGLTF_COMPONENT_TYPE_FLOAT;
					PositionAccessor.count = VertexCount;
					PositionAccessor.type  = TINYGLTF_TYPE_VEC3;

					GLTFModel.accessors.push_back(PositionAccessor);
					VertexPositionAccessorIdx = GLTFModel.accessors.size() - 1;

					FloatData = FloatData.subspan(3);
				}

				// Normals
				if( const std::uint32_t AttribMask = 0b0000'0'0000'00'0010;
					Header.VertexAttributeMask & AttribMask )
				{
					tinygltf::Accessor NormalAccessor;
					NormalAccessor.bufferView
						= GLTFModel.bufferViews.size() - 1;
					NormalAccessor.byteOffset = GetVertexBufferStride(
						(AttribMask - 1) & Header.VertexAttributeMask
					);
					NormalAccessor.maxValues = {+1.0, +1.0, +1.0};
					NormalAccessor.minValues = {-1.0, -1.0, -1.0};
					NormalAccessor.componentType
						= TINYGLTF_COMPONENT_TYPE_FLOAT;
					NormalAccessor.count = VertexCount;
					NormalAccessor.type  = TINYGLTF_TYPE_VEC3;

					GLTFModel.accessors.push_back(NormalAccessor);
					VertexNormalAccessorIdx = GLTFModel.accessors.size() - 1;

					FloatData = FloatData.subspan(3);
				}

				// Tangents
				if( const std::uint32_t AttribMask = 0b0000'0'0000'00'0100;
					Header.VertexAttributeMask & AttribMask )
				{
					tinygltf::Accessor TangentAccessor;
					TangentAccessor.bufferView
						= GLTFModel.bufferViews.size() - 1;
					TangentAccessor.byteOffset = GetVertexBufferStride(
						(AttribMask - 1) & Header.VertexAttributeMask
					);
					TangentAccessor.maxValues = {+1.0, +1.0, +1.0};
					TangentAccessor.minValues = {-1.0, -1.0, -1.0};
					TangentAccessor.componentType
						= TINYGLTF_COMPONENT_TYPE_FLOAT;
					TangentAccessor.count = VertexCount;
					TangentAccessor.type  = TINYGLTF_TYPE_VEC3;

					GLTFModel.accessors.push_back(TangentAccessor);
					VertexTangentAccessorIdx = GLTFModel.accessors.size() - 1;
					FloatData                = FloatData.subspan(3);
				}

				//  Vertex Color
				if( const std::uint32_t AttribMask = 0b0000'0'0000'01'0000;
					Header.VertexAttributeMask & AttribMask )
				{
					tinygltf::Accessor ColorAccessor;
					ColorAccessor.bufferView = GLTFModel.bufferViews.size() - 1;
					ColorAccessor.byteOffset = GetVertexBufferStride(
						(AttribMask - 1) & Header.VertexAttributeMask
					);
					ColorAccessor.maxValues     = {+1.0, +1.0, +1.0, +1.0};
					ColorAccessor.minValues     = {0.0, 0.0, 0.0, 0.0};
					ColorAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
					ColorAccessor.count         = VertexCount;
					ColorAccessor.type          = TINYGLTF_TYPE_VEC4;

					GLTFModel.accessors.push_back(ColorAccessor);
					VertexColorAccessorIdx = GLTFModel.accessors.size() - 1;

					FloatData = FloatData.subspan(4);
				}

				//  Unknown
				if( const std::uint32_t AttribMask = 0b0000'0'0000'10'0000;
					Header.VertexAttributeMask & AttribMask )
				{
					FloatData = FloatData.subspan(4);
				}

				//  Weight {0,1,2,3}
				if( const std::uint32_t AttribMask = 0b0000'0'1111'00'0000;
					Header.VertexAttributeMask & AttribMask )
				{
					const std::size_t WeightCount = std::popcount(
						Header.VertexAttributeMask & AttribMask
					);
					tinygltf::Accessor WeightsAccessor;
					WeightsAccessor.bufferView
						= GLTFModel.bufferViews.size() - 1;
					WeightsAccessor.byteOffset = GetVertexBufferStride(
						(AttribMask - 1) & Header.VertexAttributeMask
					);
					WeightsAccessor.maxValues = {+1.0, +1.0, +1.0, +1.0};
					WeightsAccessor.minValues = {0.0, 0.0, 0.0, 0.0};
					WeightsAccessor.componentType
						= TINYGLTF_COMPONENT_TYPE_FLOAT;
					WeightsAccessor.count = VertexCount;
					// switch( WeightCount )
					// {
					// case 1:
					// 	WeightsAccessor.type = TINYGLTF_TYPE_SCALAR;
					// 	break;
					// case 2:
					// 	WeightsAccessor.type = TINYGLTF_TYPE_VEC2;
					// 	break;
					// case 3:
					// 	WeightsAccessor.type = TINYGLTF_TYPE_VEC3;
					// 	break;
					// case 4:
					// 	WeightsAccessor.type = TINYGLTF_TYPE_VEC4;
					// 	break;
					// }

					WeightsAccessor.type = TINYGLTF_TYPE_VEC4;

					GLTFModel.accessors.push_back(WeightsAccessor);
					// VertexWeightsAccessorIdx = GLTFModel.accessors.size() -
					// 1;

					FloatData = FloatData.subspan(WeightCount);
				}

				//  Joints
				if( const std::uint32_t AttribMask = 0b0000'1'0000'00'0000;
					Header.VertexAttributeMask & AttribMask )
				{
					tinygltf::Accessor JointsAccessor;
					JointsAccessor.bufferView
						= GLTFModel.bufferViews.size() - 1;
					JointsAccessor.byteOffset = GetVertexBufferStride(
						(AttribMask - 1) & Header.VertexAttributeMask
					);
					JointsAccessor.maxValues = {+255.0, +255.0, +255.0, +255.0};
					JointsAccessor.minValues = {0.0, 0.0, 0.0, 0.0};
					JointsAccessor.componentType
						= TINYGLTF_COMPONENT_TYPE_FLOAT;
					JointsAccessor.count = VertexCount;
					JointsAccessor.type  = TINYGLTF_TYPE_VEC4;

					GLTFModel.accessors.push_back(JointsAccessor);
					// VertexJointsAccessorIdx = GLTFModel.accessors.size() - 1;

					FloatData = FloatData.subspan(4);
				}

				// TexCoord
				if( const std::uint32_t AttribMask = 0b0001'0'0000'00'0000;
					Header.VertexAttributeMask & AttribMask )
				{
					tinygltf::Accessor TexCoordAccessor;
					TexCoordAccessor.bufferView
						= GLTFModel.bufferViews.size() - 1;
					TexCoordAccessor.byteOffset = GetVertexBufferStride(
						(AttribMask - 1) & Header.VertexAttributeMask
					);
					TexCoordAccessor.maxValues = {+1.0, +1.0};
					TexCoordAccessor.minValues = {0.0, 0.0};
					TexCoordAccessor.componentType
						= TINYGLTF_COMPONENT_TYPE_FLOAT;
					TexCoordAccessor.count = VertexCount;
					TexCoordAccessor.type  = TINYGLTF_TYPE_VEC2;

					GLTFModel.accessors.push_back(TexCoordAccessor);
					VertexTexCoordAccessorIdx = GLTFModel.accessors.size() - 1;
					FloatData                 = FloatData.subspan(2);
				}
			}

			CurChunkData = CurChunkData.subspan(VertexDataSize);

			std::uint32_t IndexStreamCount;
			PrintFormattedBytes(CurChunkData, "l");
			CurChunkData
				= ReadFormattedBytes(CurChunkData, "l", &IndexStreamCount);

			// This is technically iterated, but there has yet to be a single
			// mesh that uses anything other than 1
			assert(IndexStreamCount == 1);

			// for( std::size_t i = 0; i < IndexStreamCount; ++i )
			//{
			std::uint32_t UnknownOne; // Index format?
			std::uint32_t CurIndexCount;
			PrintFormattedBytes(CurChunkData, "ll");
			CurChunkData = ReadFormattedBytes(
				CurChunkData, "ll", &UnknownOne, &CurIndexCount
			);

			const std::size_t IndexDataSize
				= CurIndexCount * sizeof(std::uint16_t);

			const std::span<const std::uint16_t> IndexData{
				reinterpret_cast<const std::uint16_t*>(CurChunkData.data()),
				CurIndexCount
			};

			// Add vertex data to gltf
			std::int32_t IndexAccessorIdx = -1;
			{
				tinygltf::Buffer IndexBuffer;
				IndexBuffer.name
					= IndexBuffer.name + Header.Name + ": IndexBuffer";
				std::transform(
					std::as_bytes(IndexData).begin(),
					std::as_bytes(IndexData).end(),
					std::back_inserter(IndexBuffer.data),
					std::to_integer<unsigned char>
				);
				GLTFModel.buffers.push_back(IndexBuffer);

				tinygltf::BufferView IndexBufferView;
				IndexBufferView.buffer     = GLTFModel.buffers.size() - 1;
				IndexBufferView.byteOffset = 0;
				IndexBufferView.byteLength = IndexDataSize;
				IndexBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
				GLTFModel.bufferViews.push_back(IndexBufferView);

				tinygltf::Accessor VertexAccessor;
				VertexAccessor.bufferView = GLTFModel.bufferViews.size() - 1;
				VertexAccessor.byteOffset = 0;
				VertexAccessor.maxValues.push_back(
					*std::max_element(IndexData.begin(), IndexData.end())
				);
				VertexAccessor.minValues.push_back(
					*std::min_element(IndexData.begin(), IndexData.end())
				);
				VertexAccessor.componentType
					= TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
				VertexAccessor.count = CurIndexCount;
				VertexAccessor.type  = TINYGLTF_TYPE_SCALAR;

				GLTFModel.accessors.push_back(VertexAccessor);
				IndexAccessorIdx = GLTFModel.accessors.size() - 1;
			}

			CurChunkData = CurChunkData.subspan(IndexDataSize);
			//}

			GeometryLUT.insert_or_assign(
				Header.Name,
				std::array<std::int32_t, 16>{
					IndexAccessorIdx,
					VertexPositionAccessorIdx,
					VertexNormalAccessorIdx,
					VertexTangentAccessorIdx,
					VertexColorAccessorIdx,
					VertexWeightsAccessorIdx,
					VertexJointsAccessorIdx,
					VertexTexCoordAccessorIdx,
				}
			);

			break;
		}
		case TagID::Material:
		{
			// sl
			char          MaterialName[256];
			std::uint32_t MaterialType;
			PrintFormattedBytes(CurChunkData, "sl");
			CurChunkData = ReadFormattedBytes(
				CurChunkData, "sl", MaterialName, &MaterialType
			);

			tinygltf::Material NewMaterial;
			NewMaterial.name        = MaterialName;
			NewMaterial.doubleSided = true;

			NewMaterial.pbrMetallicRoughness.metallicFactor = 0.0;
			NewMaterial.pbrMetallicRoughness.baseColorFactor
				= {1.0f, 1.0f, 1.0f, 1.0f};
			NewMaterial.extensions["KHR_materials_unlit"] = {};

			PrintFormattedBytes(CurChunkData, "s");
			char TextureName[256];
			CurChunkData = ReadFormattedBytes(
				CurChunkData, "s", TextureName, &MaterialType
			);

			// BaseColor
			glm::vec4 BaseColor;
			PrintFormattedBytes(CurChunkData, "ffff");
			CurChunkData = ReadFormattedBytes(
				CurChunkData, "ffff", &BaseColor[0], &BaseColor[1],
				&BaseColor[2], &BaseColor[3]
			);
			NewMaterial.pbrMetallicRoughness.baseColorFactor = {
				BaseColor.r,
				BaseColor.g,
				BaseColor.b,
				BaseColor.a,
			};

			if( MaterialType > 0 )
			{
				CurChunkData = PrintFormattedBytes(CurChunkData, "ffff");
			}

			switch( MaterialType )
			{
			case 2:
			case 3:
			case 4:
			case 6:
			case 7:
			{
				std::uint32_t Count;
				PrintFormattedBytes(CurChunkData, "l");
				CurChunkData = ReadFormattedBytes(CurChunkData, "l", &Count);
				for( std::size_t i = 0; i < Count; ++i )
				{
					CurChunkData = PrintFormattedBytes(CurChunkData, "s");
				}
				break;
			}
			default:
			{
				break;
			}
			}

			if( std::string_view(TextureName) != "__NOTEX__" )
			{
				if( !TextureLUT.contains(TextureName) )
				{
					// Texture might not exist yet, put a place-holder
					// for now
					GLTFModel.textures.push_back({});
					TextureLUT.emplace(
						TextureName, GLTFModel.textures.size() - 1
					);
				}
				NewMaterial.pbrMetallicRoughness.baseColorTexture.texCoord = 0;
				NewMaterial.pbrMetallicRoughness.baseColorTexture.index
					= TextureLUT.at(TextureName);
			}
			GLTFModel.materials.push_back(NewMaterial);
			MaterialLUT.emplace(MaterialName, GLTFModel.materials.size() - 1);
			break;
		}
		case TagID::Mesh:
		{
			// s
			char MeshName[256];
			CurChunkData = ReadFormattedBytes(CurChunkData, "s", MeshName);

			std::uint32_t SubmeshCount;
			CurChunkData = ReadFormattedBytes(CurChunkData, "l", &SubmeshCount);

			tinygltf::Mesh NewMesh;
			NewMesh.name = MeshName;

			for( std::uint32_t i = 0; i < SubmeshCount; ++i )
			{
				char MaterialName[256];
				char GeometryName[256];
				CurChunkData = ReadFormattedBytes(
					CurChunkData, "ss", MaterialName, GeometryName
				);
				std::printf(
					"\t%u : (Material: %s, Geometry: %s)\n", i, MaterialName,
					GeometryName
				);

				const auto&         Geo = GeometryLUT.at(GeometryName);
				tinygltf::Primitive NewPrimitive;

				if( Geo[0] >= 0 )
				{
					NewPrimitive.indices = Geo[0];
				}

				const char* VertexAttributeNames[] = {
					"POSITION",   // VertexPositionAccessorIdx
					"NORMAL",     // VertexNormalAccessorIdx
					"TANGENT",    // VertexTangentAccessorIdx
					"COLOR_0",    // VertexColorAccessorIdx
					"WEIGHTS_0",  // VertexWeightsAccessorIdx
					"JOINTS_0",   // VertexJointsAccessorIdx
					"TEXCOORD_0", // VertexTexCoordAccessorIdx
				};
				for( std::size_t AttributeIndex = 0;
					 AttributeIndex < std::size(VertexAttributeNames);
					 ++AttributeIndex )
				{
					if( Geo[AttributeIndex + 1] >= 0 )
					{
						NewPrimitive
							.attributes[VertexAttributeNames[AttributeIndex]]
							= Geo[AttributeIndex + 1];
					}
				}
				NewPrimitive.material = MaterialLUT.at(MaterialName);
				NewPrimitive.mode     = TINYGLTF_MODE_TRIANGLE_STRIP;
				NewMesh.primitives.push_back(NewPrimitive);
			}

			GLTFModel.meshes.push_back(NewMesh);
			MeshLUT.emplace(MeshName, GLTFModel.meshes.size() - 1);
			break;
		}
		case TagID::Texture:
		{
			// ssllllll
			PrintFormattedBytes(CurChunkData, "ssllllll");
			char TextureName[256];
			char TextureFileName[256];
			CurChunkData = ReadFormattedBytes(
				CurChunkData, "ss", TextureName, TextureFileName
			);

			std::filesystem::path TextureURI(std::regex_replace(
				FilePath.string(), std::regex("model"), "texture"
			));

			std::string TextureFileNameUpper(TextureFileName);
			std::transform(
				TextureFileNameUpper.begin(), TextureFileNameUpper.end(),
				TextureFileNameUpper.begin(), ::toupper
			);

			TextureURI.replace_filename(TextureFileNameUpper);
			TextureURI.replace_extension(".tga");

			auto MappedImage = mio::mmap_source(TextureURI.string().c_str());

			tinygltf::Buffer ImageBuffer;
			ImageBuffer.name = TextureFileNameUpper + ": Buffer";
			ImageBuffer.data.assign(MappedImage.begin(), MappedImage.end());
			GLTFModel.buffers.push_back(ImageBuffer);

			tinygltf::BufferView ImageBufferView;
			ImageBufferView.name       = TextureFileNameUpper + ": BufferView";
			ImageBufferView.buffer     = GLTFModel.buffers.size() - 1;
			ImageBufferView.byteLength = MappedImage.size();

			GLTFModel.bufferViews.push_back(ImageBufferView);

			tinygltf::Image NewImage;
			NewImage.name       = TextureName;
			NewImage.mimeType   = "image/tga";
			NewImage.bufferView = GLTFModel.bufferViews.size() - 1;
			GLTFModel.images.push_back(NewImage);

			tinygltf::Texture NewTexture;
			NewTexture.name   = TextureName;
			NewTexture.source = GLTFModel.images.size() - 1;

			if( TextureLUT.contains(TextureName) )
			{
				// Replace the place-holder texture
				GLTFModel.textures[TextureLUT.at(TextureName)] = NewTexture;
			}
			else
			{
				GLTFModel.textures.push_back(NewTexture);
				TextureLUT.emplace(TextureName, GLTFModel.textures.size() - 1);
			}
			break;
		}
		case TagID::Transform:
		{
			// sllllllllll
			PrintFormattedBytes(CurChunkData, "slfffffffff");
			char          TransformName[256];
			std::uint32_t Unknown1;
			glm::vec3     Position = {};
			glm::vec3     Rotation = {};
			glm::vec3     Scale    = {};

			CurChunkData = ReadFormattedBytes(
				CurChunkData, "slfffffffff", TransformName, &Unknown1,
				&Position[0], &Position[1], &Position[2], &Rotation[0],
				&Rotation[1], &Rotation[2], &Scale[0], &Scale[1], &Scale[2]
			);

			tinygltf::Node NewNode;

			NewNode.name = TransformName;

			std::copy(
				glm::begin(Position), glm::end(Position),
				std::back_inserter(NewNode.translation)
			);

			const glm::quat RotationQuat = glm::quat(glm::radians(Rotation));

			NewNode.rotation.push_back(RotationQuat[0]);
			NewNode.rotation.push_back(RotationQuat[1]);
			NewNode.rotation.push_back(RotationQuat[2]);
			NewNode.rotation.push_back(RotationQuat[3]);

			std::copy(
				glm::begin(Scale), glm::end(Scale),
				std::back_inserter(NewNode.scale)
			);

			GLTFModel.nodes.push_back(NewNode);
			TransformLUT.emplace(TransformName, GLTFModel.nodes.size() - 1);

			break;
		}
		case TagID::Unknown7:
		{
			// Nothing seems to use this
			// sssll
			CurChunkData = PrintFormattedBytes(CurChunkData, "sssll");
			break;
		}
		case TagID::Unknown8:
		{
			// Nothing seems to use this
			// sl
			CurChunkData = PrintFormattedBytes(CurChunkData, "sl");
			break;
		}
		case TagID::Unknown9:
		{
			// Nothing seems to use this
			// sl

			CurChunkData = PrintFormattedBytes(CurChunkData, "sl");
			break;
		}
		case TagID::SceneDescriptor:
		{
			// sll
			// Object name, Property name
			// Todo: Recursive lambda
			const auto ReadNode = [&](std::span<const std::byte> Data
								  ) -> std::span<const std::byte> {
				std::size_t TabLevel = 0;
				const auto  ReadNodeImpl
					= [&](std::span<const std::byte> /*Data*/,
						  auto& self) -> std::span<const std::byte> {
					++TabLevel;
					// PrintFormattedBytes(CurChunkData, "sll");
					char          CurrentNodeName[256];
					std::uint32_t AttributeType;
					std::uint32_t ChildrenCount;
					CurChunkData = ReadFormattedBytes(
						CurChunkData, "sll", CurrentNodeName, &AttributeType,
						&ChildrenCount
					);
					std::printf(
						"%*s-%s(%u)\n", TabLevel * 5, "", CurrentNodeName,
						AttributeType
					);

					// Iterate children data:
					std::span<const std::byte> ChildrenData = CurChunkData;
					for( std::size_t i = 0; i < ChildrenCount; ++i )
					{
						char          ChildNodeName[256];
						std::uint32_t ChildAttributeType;
						std::uint32_t ChildChildrenCount;
						ChildrenData = ReadFormattedBytes(
							ChildrenData, "sll", ChildNodeName,
							&ChildAttributeType, &ChildChildrenCount
						);

						// 4: Set child
						if( ChildAttributeType == 4
							|| ChildAttributeType == 11 )
						{
							const auto CurrentNodeIndex
								= TransformLUT.at(CurrentNodeName);
							const auto ChildNodeIndex
								= TransformLUT.at(ChildNodeName);
							auto& Node = GLTFModel.nodes.at(CurrentNodeIndex);
							Node.children.push_back(ChildNodeIndex);
						}
						// 2: Set Mesh
						else if( ChildAttributeType == 2 )
						{
							const auto MeshIndex = MeshLUT.at(ChildNodeName);

							const auto CurrentNodeIndex
								= TransformLUT.at(CurrentNodeName);
							auto& Node = GLTFModel.nodes.at(CurrentNodeIndex);
							Node.mesh  = MeshIndex;
							// auto&      Node      =
							// GLTFModel.nodes.at(NodeIndex);
						}
					}

					for( std::size_t i = 0; i < ChildrenCount; ++i )
					{
						// std::printf(
						//	"%*s-Child:%zu/%zu\n", TabLevel * 5, "", i,
						//	ChildrenCount
						//);
						CurChunkData = self(CurChunkData, self);
					}
					--TabLevel;
					return CurChunkData;
				};
				return ReadNodeImpl(Data, ReadNodeImpl);
			};
			CurChunkData = ReadNode(CurChunkData);
			break;
		}
		case TagID::Bone:
		{
			// sllllllllll
			PrintFormattedBytes(CurChunkData, "slfffffffff");
			char          TransformName[256];
			std::uint32_t Unknown1;
			glm::vec3     Position = {};
			glm::vec3     Rotation = {};
			glm::vec3     Scale    = {};

			CurChunkData = ReadFormattedBytes(
				CurChunkData, "slfffffffff", TransformName, &Unknown1,
				&Position[0], &Position[1], &Position[2], &Rotation[0],
				&Rotation[1], &Rotation[2], &Scale[0], &Scale[1], &Scale[2]
			);

			tinygltf::Node NewNode;

			NewNode.name = TransformName;

			std::copy(
				glm::begin(Position), glm::end(Position),
				std::back_inserter(NewNode.translation)
			);

			const glm::quat RotationQuat = glm::quat(glm::radians(Rotation));

			NewNode.rotation.push_back(RotationQuat[0]);
			NewNode.rotation.push_back(RotationQuat[1]);
			NewNode.rotation.push_back(RotationQuat[2]);
			NewNode.rotation.push_back(RotationQuat[3]);

			std::copy(
				glm::begin(Scale), glm::end(Scale),
				std::back_inserter(NewNode.scale)
			);

			GLTFModel.nodes.push_back(NewNode);
			TransformLUT.emplace(TransformName, GLTFModel.nodes.size() - 1);
			break;
		}
		default:
		{
			break;
		}
		}

		FileData = FileData.subspan(CurChunk.Size);
	}

	const std::filesystem::path DestPath = FilePath.replace_extension(".gltf");
	// Save it to a file
	tinygltf::TinyGLTF gltf;
	gltf.WriteGltfSceneToFile(
		&GLTFModel, DestPath.string(),
		true, // embedImages
		true, // embedBuffers
		true, // pretty print
		false // write binary
	);
}
} // namespace HGM
} // namespace TsuHan