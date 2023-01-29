#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <span>
#include <vector>

#include <TsuHan/TsuHan.hpp>

#include <mio/mmap.hpp>

bool ProcessPack(
	const std::filesystem::path& DumpPath,
	const std::filesystem::path& PackPath, TsuHan::PackFileInfo PackInfo);

int main(int argc, char* argv[])
{
	const auto Arguments = std::span<char*>(argv, argc);

	if( Arguments.size() < 3 )
	{
		// nothing to d
		return EXIT_SUCCESS;
	}

	const std::filesystem::path DumpPath(Arguments[1]);
	std::filesystem::create_directories(DumpPath);

	for( const char* Path : Arguments.subspan(2) )
	{
		const std::filesystem::path CurPath(Path);

		const auto FileName = CurPath.filename().string();
		std::printf("%s\n", FileName.c_str());
		if( TsuHan::PackInfo.contains(FileName) )
		{
			const auto PackInfo = TsuHan::PackInfo.at(FileName);
			ProcessPack(DumpPath, CurPath, PackInfo);
		}
		else
		{
			std::printf("-Unknown file\n");
		}
	}

	return EXIT_SUCCESS;
}

bool ProcessPack(
	const std::filesystem::path& DumpPath,
	const std::filesystem::path& PackPath, TsuHan::PackFileInfo PackInfo)

{
	auto MappedFile = mio::mmap_source(PackPath.string().c_str());

	const auto FileData = std::span<const std::byte>(
		reinterpret_cast<const std::byte*>(MappedFile.data()),
		MappedFile.size());

	std::vector<std::byte> DecryptedData(FileData.begin(), FileData.end());

	// Apply the XOR key
	{
		const auto DecryptedData32 = std::span<std::uint32_t>(
			reinterpret_cast<std::uint32_t*>(DecryptedData.data()),
			DecryptedData.size() / 4);

		for( std::uint32_t& CurWord : DecryptedData32 )
		{
			CurWord ^= PackInfo.Key;
		}
	}

	std::filesystem::create_directories(DumpPath / PackInfo.Root);

	for( const auto& [FileName, FileSpan] : PackInfo.Files )
	{
		std::filesystem::path OutPath = DumpPath / PackInfo.Root / FileName;
		OutPath.replace_extension(PackInfo.Extension);

		std::ofstream OutFile(OutPath);

		const std::span<const std::byte> CurFileData
			= std::span(DecryptedData).subspan(FileSpan.Offset, FileSpan.Size);

		std::printf(
			"-%s (%zu bytes)\n", OutPath.string().c_str(), CurFileData.size());

		OutFile.write(
			reinterpret_cast<const char*>(CurFileData.data()),
			CurFileData.size());

		if( PackInfo.Handler )
		{
			PackInfo.Handler(CurFileData, OutPath);
		}
	}

	return true;
}