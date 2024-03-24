#include <TsuHan/TsuHan.hpp>

#include <cstdarg>
#include <cstring>
#include <regex>
#include <string_view>

#include "tiny_gltf.h"

#include <mio/mmap.hpp>

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

	tinygltf::Scene GLTFScene;
	GLTFScene.name = FilePath.filename();
	GLTFScene.nodes.push_back(0);

	GLTFModel.scenes.push_back(GLTFScene);

	std::unordered_map<std::string, std::array<std::int32_t, 16>> GeometryLUT;
	std::unordered_map<std::string, std::uint32_t>                MaterialLUT;
	std::unordered_map<std::string, std::uint32_t>                TextureLUT;

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
			NewMaterial.pbrMetallicRoughness.baseColorFactor
				= {1.0F, 1.0F, 1.0F, 1.0F};

			PrintFormattedBytes(CurChunkData, "s");
			char TextureName[256];
			CurChunkData = ReadFormattedBytes(
				CurChunkData, "s", TextureName, &MaterialType
			);
			CurChunkData = PrintFormattedBytes(CurChunkData, "ffff");

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
				if( Geo[1] >= 0 )
				{
					NewPrimitive.attributes["POSITION"] = Geo[1];
				}
				if( Geo[2] >= 0 )
				{
					NewPrimitive.attributes["NORMAL"] = Geo[2];
				}
				if( Geo[3] >= 0 )
				{
					NewPrimitive.attributes["TANGENT"] = Geo[3];
				}
				if( Geo[4] >= 0 )
				{
					NewPrimitive.attributes["TEXCOORD_0"] = Geo[4];
				}
				NewPrimitive.material = MaterialLUT.at(MaterialName);
				NewPrimitive.mode     = TINYGLTF_MODE_TRIANGLE_STRIP;
				NewMesh.primitives.push_back(NewPrimitive);
			}

			GLTFModel.meshes.push_back(NewMesh);

			// Todo, move the scene node/hiearchy stuff
			tinygltf::Node NewNode;
			NewNode.name = MeshName;
			NewNode.mesh = GLTFModel.meshes.size() - 1;

			GLTFModel.nodes.push_back(NewNode);
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
			CurChunkData = PrintFormattedBytes(CurChunkData, "slffflllfff");
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
			// Todo: Recursive lambda
			const auto ReadNode = [&](std::span<const std::byte> Data
								  ) -> std::span<const std::byte> {
				const auto ReadNodeImpl
					= [&](std::span<const std::byte> /*Data*/,
						  auto& self) -> std::span<const std::byte> {
					PrintFormattedBytes(CurChunkData, "sll");
					char          Name[256];
					std::uint32_t DescriptorType;
					std::uint32_t ChildrenCount;
					CurChunkData = ReadFormattedBytes(
						CurChunkData, "sll", Name, &DescriptorType,
						&ChildrenCount
					);
					for( std::size_t i = 0; i < ChildrenCount; ++i )
					{
						std::printf("\t-Child:%zu\n", i);
						CurChunkData = self(CurChunkData, self);
					}
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
			CurChunkData = PrintFormattedBytes(CurChunkData, "slffflllfff");
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
		false
	); // write binary
}
} // namespace HGM
} // namespace TsuHan