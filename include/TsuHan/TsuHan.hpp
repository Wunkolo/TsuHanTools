#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <unordered_map>

namespace TsuHan
{

namespace HGM
{

enum class TagID : std::uint32_t
{
	Geometry  = 0,
	Material  = 1,
	Mesh      = 2,
	Texture   = 3,
	Transform = 4,
	Unknown7  = 7,
	Unknown8  = 8,
	Unknown9  = 9,
	Unknown10 = 10,
	Bone      = 11,
};

const char* ToString(TagID Tag);

std::size_t GetVertexBufferStride(std::uint16_t VertexAttributeMask);

std::span<const std::byte>
	ReadFormattedBytes(std::span<const std::byte> Bytes, const char* Format...);

void HGMHandler(
	std::span<const std::byte> FileData, std::filesystem::path FilePath);

struct Chunk
{
	TagID         Tag;
	std::uint32_t Size;
};

} // namespace HGM

struct PackFileInfo
{
	std::uint32_t Key;
	const char*   Root;
	const char*   Extension;

	using HandlerProc
		= void(std::span<const std::byte>, std::filesystem::path&);

	std::function<HandlerProc> Handler;

	struct FileSpan
	{
		std::uint32_t Offset;
		std::uint32_t Size;
	};

	// clang-format off
	/*
	Generated with:
	def dump(off, count):
		for x in range(count):
			offset = off + x * 12
			fileoffset = idaapi.get_dword(offset)
			filesize = idaapi.get_dword(offset + 4)
			name = idc.get_strlit_contents(idaapi.get_dword(offset + 8), -1, idc.ASCSTR_C)
			name = '"' + name + '"'
			print('{{{:24}, {{0x{:06X}, 0x{:06X}}}}},'.format(name, fileoffset, filesize))
	*/
	// clang-format on
	std::unordered_map<std::string, FileSpan> Files;
};

const std::unordered_map<std::string, PackFileInfo> PackInfo = {

	// clang-format off
	{
		"model00.bin",
		{
			0x05C573A64, "model/common", ".hgm", HGM::HGMHandler,
			{
				{"BELTVIBRATOR"          , {0x000000, 0x008F08}},
				{"BODYBLADE"             , {0x008F08, 0x0069E4}},
				{"BODYVAC"               , {0x00F8EC, 0x003738}},
				{"CHAIR"                 , {0x013024, 0x00325C}},
				{"CONGA"                 , {0x016280, 0x00933C}},
				{"CUSHION"               , {0x01F5BC, 0x001A22}},
				{"DIABOLOS"              , {0x020FDE, 0x0054BA}},
				{"DOMOHORNWINKLE"        , {0x026498, 0x00327C}},
				{"DOOR"                  , {0x029714, 0x0064FC}},
				{"FLYPANB"               , {0x02FC10, 0x003D3A}},
				{"FURIN"                 , {0x03394A, 0x003200}},
				{"HOUCHOU"               , {0x036B4A, 0x001E70}},
				{"ICECANDY"              , {0x0389BA, 0x001254}},
				{"ICECREAM"              , {0x039C0E, 0x00302C}},
				{"KOTATSU"               , {0x03CC3A, 0x004086}},
				{"LIGHTSABER"            , {0x040CC0, 0x006F2C}},
				{"MILK"                  , {0x047BEC, 0x003338}},
				{"OTAMA"                 , {0x04AF24, 0x004630}},
				{"PAPAMAKURA"            , {0x04F554, 0x007C1A}},
				{"PUNCHINGBAG"           , {0x05716E, 0x001AF6}},
				{"REIZOUKO"              , {0x058C64, 0x00AF66}},
				{"ROOM"                  , {0x063BCA, 0x05DC8A}},
				{"SEGWAY"                , {0x0C1854, 0x01687E}},
				{"SENPUKI"               , {0x0D80D2, 0x00C9E2}},
				{"TELEPORTER"            , {0x0E4AB4, 0x0072DC}},
				{"TV"                    , {0x0EBD90, 0x017722}},
			}
		}
	},
	{
		"model01.bin",
		{
			0x0D8332DC9, "model/japanet", ".hgm", HGM::HGMHandler,
			{
				{"MONEYDIAL"             , {0x000000, 0x000712}},
				{"TSU_HANTV"             , {0x000712, 0x06BF3E}},
				{"TVANIMATETEXTURE"      , {0x06C650, 0x000878}},
			}
		}
	},
	{
		"model02.bin",
		{
			0x0967B7AC3, "model/indicator", ".hgm", HGM::HGMHandler,
			{
				{"CONFIGMENU"            , {0x000000, 0x0042D8}},
				{"SCREENSAVER"           , {0x0042D8, 0x001D5A}},
			}
		}
	},
	{
		"model03.bin",
		{
			0x0AB86195F, "model/ending", ".hgm", HGM::HGMHandler,
			{
				{"ENDROLL"               , {0x000000, 0x0050B0}},
				{"SMOKE"                 , {0x0050B0, 0x0001F4}},
			}
		}
	},
	{
		"model04.bin",
		{
			0x08795DBBE, "model/osaka", ".hgm", HGM::HGMHandler,
			{
				{"CHUCHU"                , {0x000000, 0x0074C2}},
				{"CLOTH"                 , {0x0074C2, 0x00B4F8}},
				{"DYNAMICSCLOTHBLUE"     , {0x0129BA, 0x0084AC}},
				{"DYNAMICSCLOTHPINK"     , {0x01AE66, 0x0082F2}},
				{"ETONADRESS"            , {0x023158, 0x008228}},
				{"KATAHIMO"              , {0x02B380, 0x000ADC}},
				{"MINISKIRT"             , {0x02BE5C, 0x001E40}},
				{"NAGASODE"              , {0x02DC9C, 0x007A22}},
				{"OSAKA"                 , {0x0356BE, 0x09D494}},
				{"OSAKANAGASODE"         , {0x0D2B52, 0x0089DA}},
				{"TIGHTSKIRT"            , {0x0DB52C, 0x001E28}},
			}
		}
	},

	{
		"texture00.bin",
		{
			0x083D9DB43, "texture/common", ".tga", {},
			{
				{"CONGA_METAL"           , {0x000000, 0x0000EC}},
				{"CONGA_REDWOOD"         , {0x0000EC, 0x003051}},
				{"CONGA_SKIN"            , {0x00313D, 0x00C03B}},
				{"DIABOLOS"              , {0x00F178, 0x00433D}},
				{"FONTTABLE"             , {0x0134B5, 0x03002C}},
				{"HOUCHOU_BLADE"         , {0x0434E1, 0x001D45}},
				{"HOUCHOU_WOOD"          , {0x045226, 0x003036}},
				{"MILK"                  , {0x04825C, 0x007846}},
				{"REF"                   , {0x04FAA2, 0x000C25}},
				{"ROOM_BEDWOOD2003"      , {0x0506C7, 0x00C085}},
				{"ROOM_BOOKS"            , {0x05C74C, 0x002958}},
				{"ROOM_DESKGRADMINI"     , {0x05F0A4, 0x000312}},
				{"ROOM_DESKWOOD2003"     , {0x05F3B6, 0x00C02C}},
				{"ROOM_DOMECFUCHI2"      , {0x06B3E2, 0x0008D2}},
				{"ROOM_FLOARING3"        , {0x06BCB4, 0x00C040}},
				{"ROOM_KOTATSUCARPET2003", {0x077CF4, 0x00C078}},
				{"ROOM_KOTATSUSTIRIPE2003", {0x083D6C, 0x00022C}},
				{"ROOM_KOTATSUTOP"       , {0x083F98, 0x001DD4}},
				{"ROOM_KOTATSUWOOD2003"  , {0x085D6C, 0x000C31}},
				{"ROOM_KRWOOD1B"         , {0x08699D, 0x00BF6B}},
				{"ROOM_KRWOOD5FUCHI2"    , {0x092908, 0x002F9F}},
				{"ROOM_PATTERNJ2FLOWER2" , {0x0958A7, 0x02E5D8}},
				{"ROOM_TVLINE"           , {0x0C3E7F, 0x002D4B}},
				{"ROOM_TVRACK2"          , {0x0C6BCA, 0x002F51}},
				{"ROOM_TVRACK3"          , {0x0C9B1B, 0x00C05E}},
				{"ROOM_WASASHAB"         , {0x0D5B79, 0x02FC8A}},
				{"ROOM_WOOD1C2TANSU"     , {0x105803, 0x00BF19}},
				{"ROOM_WOOD1CKYOUDAI"    , {0x11171C, 0x00BE03}},
				{"ROOM_WOOD1CKYOUDAI2"   , {0x11D51F, 0x00C04F}},
				{"SKY_DAY"               , {0x12956E, 0x01F8E4}},
				{"SKY_EVENING"           , {0x148E52, 0x00D79A}},
				{"SKY_NIGHT"             , {0x1565EC, 0x001344}},
				{"WHITETEX"              , {0x157930, 0x00002F}},
			}
		}
	},
	{
		"texture01.bin",
		{
			0x0FE6725D1, "texture/japanet", ".tga", {},
			{
				{"CHIYOP_TV_BOARD"       , {0x000000, 0x007CFF}},
				{"CLOTHR_AZMANGA"        , {0x007CFF, 0x00C02C}},
				{"CLOTHR_CHEERLEADER"    , {0x013D2B, 0x00C02C}},
				{"CLOTHR_CHUCHU"         , {0x01FD57, 0x00C02C}},
				{"CLOTHR_CLOTHBLUE"      , {0x02BD83, 0x007DB2}},
				{"CLOTHR_CLOTHORANGE"    , {0x033B35, 0x007CA9}},
				{"CLOTHR_CLOTHPINK"      , {0x03B7DE, 0x007C24}},
				{"CLOTHR_ETONADRESS"     , {0x043402, 0x00C02C}},
				{"CLOTHR_EXERSISE"       , {0x04F42E, 0x00C02C}},
				{"CLOTHR_HADAKA"         , {0x05B45A, 0x00C02C}},
				{"CLOTHR_HATAN"          , {0x067486, 0x00C02C}},
				{"CLOTHR_JAJIYELLOW"     , {0x0734B2, 0x00C02C}},
				{"CLOTHR_JAZIRED"        , {0x07F4DE, 0x00C02C}},
				{"CLOTHR_LEOTARD"        , {0x08B50A, 0x00C02C}},
				{"CLOTHR_MOMO"           , {0x097536, 0x00C02C}},
				{"CLOTHR_PAJAMAORANGE"   , {0x0A3562, 0x00C02C}},
				{"CLOTHR_SHADOW"         , {0x0AF58E, 0x0026B0}},
				{"CLOTHR_SWIMBIKINIA"    , {0x0B1C3E, 0x00C02C}},
				{"CLOTHR_SWIMBIKINIB"    , {0x0BDC6A, 0x00C02C}},
				{"CLOTHR_SWIMSCHOOL"     , {0x0C9C96, 0x00C02C}},
				{"CLOTHR_SWIMWANPIA"     , {0x0D5CC2, 0x00C02C}},
				{"CLOTHR_TRAININGA"      , {0x0E1CEE, 0x00C02C}},
				{"CLOTHR_TRAININGB"      , {0x0EDD1A, 0x00C02C}},
				{"CLOTHR_YAMAYURI"       , {0x0F9D46, 0x00C02C}},
				{"DIALNUMBERS"           , {0x105D72, 0x0055FB}},
				{"SHOPPING_ARROWA"       , {0x10B36D, 0x01002C}},
				{"SHOPPING_ARROWB"       , {0x11B399, 0x08002C}},
				{"SHOPPING_BUBBLE"       , {0x19B3C5, 0x01002C}},
				{"SHOPPING_TABLE"        , {0x1AB3F1, 0x10002C}},
				{"TV_ITEM01_KOTATSU"     , {0x2AB41D, 0x001E91}},
				{"TV_ITEM02_BODYBLADE"   , {0x2AD2AE, 0x00242A}},
				{"TV_ITEM03_DOMOHO"      , {0x2AF6D8, 0x002529}},
				{"TV_ITEM04_PUNCHINGBAG" , {0x2B1C01, 0x002271}},
				{"TV_ITEM05_HOUKYOU"     , {0x2B3E72, 0x002242}},
				{"TV_ITEM06_SUTEKIMAKURA", {0x2B60B4, 0x002477}},
				{"TV_ITEM07_LIGHTSAVER"  , {0x2B852B, 0x00224B}},
				{"TV_ITEM08_GINGER"      , {0x2BA776, 0x001F9E}},
				{"TV_ITEM09_TAKAEDA"     , {0x2BC714, 0x00272E}},
				{"TV_ITEM10_CAPMUGIWARA" , {0x2BEE42, 0x00213A}},
				{"TV_ITEM11_CAPNURSE"    , {0x2C0F7C, 0x001DC6}},
				{"TV_ITEM12_CAPMAID"     , {0x2C2D42, 0x002F11}},
				{"TV_ITEM13_KUROBUCHI"   , {0x2C5C53, 0x00267E}},
				{"TV_ITEM14_MIMINEKO"    , {0x2C82D1, 0x001D76}},
				{"TV_ITEM15_MIMIKUMA"    , {0x2CA047, 0x001C5E}},
				{"TV_ITEM16_MIMIUSA"     , {0x2CBCA5, 0x001C4A}},
				{"TV_ITEM17_MIMIASUKA"   , {0x2CD8EF, 0x001E6A}},
				{"TV_ITEM18_TAILNEKO"    , {0x2CF759, 0x002470}},
				{"TV_ITEM19_BURUBURU"    , {0x2D1BC9, 0x002327}},
				{"TV_ITEM20_KITAROU"     , {0x2D3EF0, 0x002C9E}},
				{"TV_ITEM21_EYEOYADI"    , {0x2D6B8E, 0x0027F2}},
				{"TV_ITEM22_SHAMPOOHAT"  , {0x2D9380, 0x002523}},
				{"TV_ITEM23_GLASSSTAR"   , {0x2DB8A3, 0x0020D5}},
				{"TV_ITEM24_MULTISENSOR" , {0x2DD978, 0x0025E0}},
				{"TV_ITEM25_ANGELRING"   , {0x2DFF58, 0x002A22}},
				{"TV_ITEM26_ANGELWING"   , {0x2E297A, 0x0022C8}},
				{"TV_ITEM27_TELEPORTER"  , {0x2E4C42, 0x0028CF}},
				{"TV_ITEM28_HOUCHOU"     , {0x2E7511, 0x001FAA}},
				{"TV_ITEM29_FLYPANB"     , {0x2E94BB, 0x002E8C}},
				{"TV_ITEM30_NABE"        , {0x2EC347, 0x0021C7}},
				{"TV_ITEM30_OSAGECHIYO"  , {0x2EE50E, 0x0022B4}},
				{"TV_ITEM32_FLYPANA"     , {0x2F07C2, 0x002C76}},
				{"TV_ITEM33_CONGA"       , {0x2F3438, 0x002113}},
				{"TV_ITEM34_MIKAN"       , {0x2F554B, 0x001DCD}},
				{"TV_ITEM35_OTAMA"       , {0x2F7318, 0x001E80}},
				{"TV_ITEM36_YUNOMI"      , {0x2F9198, 0x0025DE}},
				{"TV_ITEM37_CAPCHIYOPAPA", {0x2FB776, 0x0020DF}},
				{"TV_ITEM38_MIMISARU"    , {0x2FD855, 0x002095}},
				{"TV_ITEM39_SIPPOSARU"   , {0x2FF8EA, 0x00240E}},
				{"TV_ITEM40_CAPNENEKO"   , {0x301CF8, 0x002C4F}},
				{"TV_ITEM41_CAPALIAN"    , {0x304947, 0x002A61}},
				{"TV_ITEM42_CAPSUMMER"   , {0x3073A8, 0x001FDB}},
				{"TV_ITEM43_OSAGESPIN"   , {0x309383, 0x0027A6}},
				{"TV_ITEM44_CAPHELMET"   , {0x30BB29, 0x00217F}},
				{"TV_ITEM45_FURIN"       , {0x30DCA8, 0x002483}},
				{"TV_ITEM46_REIZOUKO"    , {0x31012B, 0x0021FA}},
				{"TV_ITEM47_SENPUKI"     , {0x312325, 0x0023AD}},
				{"TV_ITEM48_COMIC"       , {0x3146D2, 0x001958}},
				{"TV_ITEM49_POOL"        , {0x31602A, 0x001EFC}},
				{"TV_ITEM50_KAIGOURI"    , {0x317F26, 0x0022AE}},
				{"TV_ITEM51_ICECREAM"    , {0x31A1D4, 0x002192}},
				{"TV_ITEM52_ICECANDY"    , {0x31C366, 0x001ABC}},
				{"TV_ITEM53_MILK"        , {0x31DE22, 0x001C83}},
				{"TV_ITEM54_GENSHIBEEF"  , {0x31FAA5, 0x002282}},
				{"TV_ITEM55_MELONPAN"    , {0x321D27, 0x001B4F}},
				{"TV_ITEM56_ANPAN"       , {0x323876, 0x002884}},
				{"TV_ITEM57_BLUECLOTH"   , {0x3260FA, 0x00202D}},
				{"TV_ITEM58_CAPAORINGO"  , {0x328127, 0x00239B}},
				{"TV_ITEM59_ORANGECLOTH" , {0x32A4C2, 0x002275}},
				{"TV_ITEM60_CAPGREENBERET", {0x32C737, 0x002D34}},
				{"TV_ITEM61_SWIMSEPARATE", {0x32F46B, 0x00244C}},
				{"TV_ITEM62_CAMERA"      , {0x3318B7, 0x0027DF}},
				{"TV_ITEM63_PAJAMAORANGE", {0x334096, 0x002BCE}},
				{"TV_ITEM64_JAZIRED"     , {0x336C64, 0x0021E8}},
				{"TV_ITEM65_JAZIYELLOW"  , {0x338E4C, 0x00200F}},
				{"TV_ITEM66_CHEERLEADER" , {0x33AE5B, 0x002533}},
				{"TV_ITEM67_LILIAN"      , {0x33D38E, 0x0027F0}},
				{"TV_ITEM68_HATAN"       , {0x33FB7E, 0x002A28}},
				{"TV_ITEM69_AZMANGA"     , {0x3425A6, 0x003001}},
				{"TV_ITEM70_MOMODRESS"   , {0x3455A7, 0x00202F}},
				{"TV_ITEM71_ETONADRESS"  , {0x3475D6, 0x002828}},
				{"TV_ITEM72_CHUCHU"      , {0x349DFE, 0x001D64}},
				{"TV_ITEM73_LEOTARD"     , {0x34BB62, 0x001FA6}},
				{"TV_ITEM74_SWIMONPIECEA", {0x34DB08, 0x002768}},
				{"TV_ITEM75_SWIMBIKINIA" , {0x350270, 0x00225C}},
				{"TV_ITEM76_TRAININGA"   , {0x3524CC, 0x00233B}},
				{"TV_ITEM77_TRAININGB"   , {0x354807, 0x0025FF}},
				{"TV_ITEM78_SWIMBIKINIB" , {0x356E06, 0x001F61}},
				{"TV_ITEM79_SWIMSCHOOL"  , {0x358D67, 0x0025A6}},
				{"TV_ITEM80_EXCERSISE"   , {0x35B30D, 0x002348}},
				{"TV_ITEM81_HENSHINBELT" , {0x35D655, 0x002CD2}},
				{"TV_ITEM82_ETONAWING"   , {0x360327, 0x0029E1}},
				{"TV_ITEM83_ETONABRACELET", {0x362D08, 0x002ECB}},
				{"TV_ITEM84_ETONAKUBIWA" , {0x365BD3, 0x002841}},
				{"TV_ITEM85_CAPAKAZUKIN" , {0x368414, 0x001EAE}},
				{"TV_ITEM86_CAPANTENA"   , {0x36A2C2, 0x001BCF}},
				{"TV_ITEM87_CAPWITCH"    , {0x36BE91, 0x0025CB}},
				{"TV_ITEM88_MIMIAYANAMI" , {0x36E45C, 0x0021F2}},
				{"TV_ITEM89_CAPMOMO"     , {0x37064E, 0x001ED4}},
				{"TV_ITEM90_CAPKOSMOS"   , {0x372522, 0x002848}},
				{"TV_ITEM91_ETONAOSAGE"  , {0x374D6A, 0x00323B}},
				{"TV_ITEM92_RIBBONDEFRON", {0x377FA5, 0x0035AC}},
				{"TV_ITEM93_DIABOLOS"    , {0x37B551, 0x002FDC}},
				{"TV_ITEM94_DRYER"       , {0x37E52D, 0x0033C2}},
				{"TV_ITEM95_ETONABRACELET2", {0x3818EF, 0x002EE8}},
			}
		}
	},
	{
		"texture02.bin",
		{
			0x075893254, "texture/indicator", ".tga", {},
			{
				{"CONFIG"                , {0x000000, 0x0218CE}},
				{"SC_MOZI"               , {0x0218CE, 0x004BE2}},
			}
		}
	},
	{
		"texture03.bin",
		{
			0x0323D47A5, "texture/ending", ".tga", {},
			{
				{"DUMMY_END_CONGRATULATIONS", {0x000000, 0x00002F}},
				{"DUMMY_END_GRADATION"   , {0x00002F, 0x00002F}},
				{"DUMMY_END_STAFF1"      , {0x00005E, 0x00002F}},
				{"DUMMY_END_STAFF2"      , {0x00008D, 0x00002F}},
				{"EFFECT_BLACKSMOKEC"    , {0x0000BC, 0x009878}},
				{"END_CONGRATULATIONS"   , {0x009934, 0x022E12}},
				{"END_GRADATION"         , {0x02C746, 0x0000CC}},
				{"END_STAFF1"            , {0x02C812, 0x016415}},
				{"END_STAFF2"            , {0x042C27, 0x01CD1C}},
			}
		}
	},
	{
		"texture04.bin",
		{
			0x098D57FFC, "texture/osaka", ".tga", {},
			{
				{"CAPANTENAGRAY"         , {0x000000, 0x0002CC}},
				{"CAPAORINGOFRAME"       , {0x0002CC, 0x000807}},
				{"CAPKOSMOS"             , {0x000AD3, 0x004415}},
				{"CAPNENEKO"             , {0x004EE8, 0x00560B}},
				{"CAPRIBBONDEFRON"       , {0x00A4F3, 0x00033C}},
				{"CLOTHAZMANGA"          , {0x00A82F, 0x00952C}},
				{"CLOTHBLUE"             , {0x013D5B, 0x02E15F}},
				{"CLOTHCHEERLEADER"      , {0x041EBA, 0x000316}},
				{"CLOTHCHUCHU"           , {0x0421D0, 0x00013C}},
				{"CLOTHETONA"            , {0x04230C, 0x017DAD}},
				{"CLOTHHATAN"            , {0x05A0B9, 0x00A99A}},
				{"CLOTHMOMO"             , {0x064A53, 0x00A12A}},
				{"CLOTHORANGE"           , {0x06EB7D, 0x02F0CC}},
				{"CLOTHPINK"             , {0x09DC49, 0x02ECA7}},
				{"CLOTHYAMAYURI"         , {0x0CC8F0, 0x000030}},
				{"HAIBANE_RING"          , {0x0CC920, 0x000092}},
				{"JAZI"                  , {0x0CC9B2, 0x01EED2}},
				{"JAZIYELLOW"            , {0x0EB884, 0x01CA92}},
				{"MARUKAGE"              , {0x108316, 0x00102C}},
				{"MIMIASUKA"             , {0x109342, 0x00031C}},
				{"MIMIMOMO"              , {0x10965E, 0x00091D}},
				{"MIMIMOMOGRAY"          , {0x109F7B, 0x000BC0}},
				{"MIMIREI"               , {0x10AB3B, 0x00032C}},
				{"MIMIWITCH"             , {0x10AE67, 0x000108}},
				{"OSAGECHIYOHAIRCOLOR"   , {0x10AF6F, 0x000092}},
				{"OSAGESPIN"             , {0x10B001, 0x002F6C}},
				{"OSAKA_ARMAZMANGA"      , {0x10DF6D, 0x000EAF}},
				{"OSAKA_ARMCHEERLEADER"  , {0x10EE1C, 0x000F93}},
				{"OSAKA_ARMETONA"        , {0x10FDAF, 0x0059FF}},
				{"OSAKA_ARMHATAN"        , {0x1157AE, 0x00336B}},
				{"OSAKA_ARMMOMO"         , {0x118B19, 0x00B3FB}},
				{"OSAKA_ARMTRAININGA"    , {0x123F14, 0x000FB7}},
				{"OSAKA_ARMTRAININGB"    , {0x124ECB, 0x001428}},
				{"OSAKA_ARMYAMAYURI"     , {0x1262F3, 0x000A07}},
				{"OSAKA_FACE_1"          , {0x126CFA, 0x002B67}},
				{"OSAKA_FACE_10"         , {0x129861, 0x002489}},
				{"OSAKA_FACE_11"         , {0x12BCEA, 0x0014EB}},
				{"OSAKA_FACE_12"         , {0x12D1D5, 0x00160C}},
				{"OSAKA_FACE_2"          , {0x12E7E1, 0x0009F9}},
				{"OSAKA_FACE_3"          , {0x12F1DA, 0x001237}},
				{"OSAKA_FACE_4"          , {0x130411, 0x000E3B}},
				{"OSAKA_FACE_5"          , {0x13124C, 0x002211}},
				{"OSAKA_FACE_6"          , {0x13345D, 0x000B91}},
				{"OSAKA_FACE_7"          , {0x133FEE, 0x0013A9}},
				{"OSAKA_FACE_8"          , {0x135397, 0x0013AA}},
				{"OSAKA_FACE_9"          , {0x136741, 0x00323A}},
				{"OSAKA_HAIR01GRAD"      , {0x13997B, 0x00012C}},
				{"OSAKA_HAIR02ETONA"     , {0x139AA7, 0x00012C}},
				{"OSAKA_LEGAZMANGA"      , {0x139BD3, 0x000670}},
				{"OSAKA_LEGCHEERLEADER"  , {0x13A243, 0x000B73}},
				{"OSAKA_LEGCHUCHU"       , {0x13ADB6, 0x004C5E}},
				{"OSAKA_LEGETONA"        , {0x13FA14, 0x00593D}},
				{"OSAKA_LEGHATAN"        , {0x145351, 0x002989}},
				{"OSAKA_LEGLEOTARD"      , {0x147CDA, 0x006FD2}},
				{"OSAKA_LEGMOMO"         , {0x14ECAC, 0x0117AF}},
				{"OSAKA_LEGSWIMSHOLDER1" , {0x16045B, 0x00022C}},
				{"OSAKA_LEGTRAININGAPSD" , {0x160687, 0x0005A4}},
				{"OSAKA_LEGTRAININGBPSD" , {0x160C2B, 0x0019EB}},
				{"OSAKA_LEGYAMAYURI"     , {0x162616, 0x005131}},
				{"OSAKA_NECKCHEERLEADER" , {0x167747, 0x0000D4}},
				{"OSAKA_NECKETONA"       , {0x16781B, 0x001A15}},
				{"OSAKA_NECKHATAN"       , {0x169230, 0x00190F}},
				{"OSAKA_NECKMOMO"        , {0x16AB3F, 0x0000AC}},
				{"OSAKA_NECKTRAININGA"   , {0x16ABEB, 0x00010C}},
				{"OSAKA_NECKTRAININGB"   , {0x16ACF7, 0x00014C}},
				{"OSAKA_SKINARM"         , {0x16AE43, 0x00002F}},
				{"OSAKA_SKINAZMANGA"     , {0x16AE72, 0x011DF1}},
				{"OSAKA_SKINBODY"        , {0x17CC63, 0x00801F}},
				{"OSAKA_SKINBODYCHUCHU"  , {0x184C82, 0x01736B}},
				{"OSAKA_SKINBODYLEOTARD" , {0x19BFED, 0x015A77}},
				{"OSAKA_SKINBODYSWIM"    , {0x1B1A64, 0x0111F6}},
				{"OSAKA_SKINBODYSWIMNS1" , {0x1C2C5A, 0x01298C}},
				{"OSAKA_SKINBODYSWIMNS2" , {0x1D55E6, 0x0101B0}},
				{"OSAKA_SKINBODYSWIMSHOLDER1", {0x1E5796, 0x013359}},
				{"OSAKA_SKINBODYSWIMSHOLDER2", {0x1F8AEF, 0x0124AF}},
				{"OSAKA_SKINBODYTRAININGA", {0x20AF9E, 0x009BD2}},
				{"OSAKA_SKINBODYTRAININGB", {0x214B70, 0x00C50B}},
				{"OSAKA_SKINCHEERLEADER" , {0x22107B, 0x006CF4}},
				{"OSAKA_SKINHATAN"       , {0x227D6F, 0x00F3E3}},
				{"OSAKA_SKINLEG"         , {0x237152, 0x00002F}},
				{"OSAKA_SKINMOMO"        , {0x237181, 0x012EAF}},
				{"OSAKA_SKINNECK"        , {0x24A030, 0x00002F}},
				{"OSAKA_SKINYAMAYURI"    , {0x24A05F, 0x00DB58}},
				{"PAJAMAORANGE"          , {0x257BB7, 0x004741}},
			}
		}
	},
	// clang-format on
};

} // namespace TsuHan