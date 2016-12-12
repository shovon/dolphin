#include <iostream>

#include "DiscIO/NANDContentLoader.h"
#include "Common/CommonFuncs.h"
#include "Common/CommonPaths.h"
#include "Common/FileUtil.h"
#include "Core/Boot/ElfReader.h"
#include "Core/PowerPC/Gekko.h"
#include "Core/PowerPC/PowerPC.h"
#include "Core/PowerPC/PPCSymbolDB.h"
#include "Core/HLE/HLE.h"
#include "Core/ConfigManager.h"
#include "Core/IPC_HLE/WII_IPC_HLE_Device_es.h"

void UpdateDebugger_MapLoaded()
{
  // Host_NotifyMapLoaded();
}

bool FindMapFile(std::string* existing_map_file, std::string* writable_map_file,
                        std::string* title_id = nullptr)
{
  std::string title_id_str;
  size_t name_begin_index;

  SConfig& _StartupPara = SConfig::GetInstance();
  switch (_StartupPara.m_BootType)
  {
  case SConfig::BOOT_WII_NAND:
  {
    const DiscIO::CNANDContentLoader& Loader =
        DiscIO::CNANDContentManager::Access().GetNANDLoader(_StartupPara.m_strFilename);
    if (Loader.IsValid())
    {
      u64 TitleID = Loader.GetTitleID();
      title_id_str = StringFromFormat("%08X_%08X", (u32)(TitleID >> 32) & 0xFFFFFFFF,
                                      (u32)TitleID & 0xFFFFFFFF);
    }
    break;
  }

  case SConfig::BOOT_ELF:
  case SConfig::BOOT_DOL:
    // Strip the .elf/.dol file extension and directories before the name
    name_begin_index = _StartupPara.m_strFilename.find_last_of("/") + 1;
    if ((_StartupPara.m_strFilename.find_last_of("\\") + 1) > name_begin_index)
    {
      name_begin_index = _StartupPara.m_strFilename.find_last_of("\\") + 1;
    }
    title_id_str = _StartupPara.m_strFilename.substr(
        name_begin_index, _StartupPara.m_strFilename.size() - 4 - name_begin_index);
    break;

  default:
    title_id_str = _StartupPara.GetGameID();
    break;
  }

  if (writable_map_file)
    *writable_map_file = File::GetUserPath(D_MAPS_IDX) + title_id_str + ".map";

  if (title_id)
    *title_id = title_id_str;

  bool found = false;
  static const std::string maps_directories[] = {File::GetUserPath(D_MAPS_IDX),
                                                 File::GetSysDirectory() + MAPS_DIR DIR_SEP};
  for (size_t i = 0; !found && i < ArraySize(maps_directories); ++i)
  {
    std::string path = maps_directories[i] + title_id_str + ".map";
    if (File::Exists(path))
    {
      found = true;
      if (existing_map_file)
        *existing_map_file = path;
    }
  }

  return found;
}

bool LoadMapFromFilename()
{
  std::string strMapFilename;
  bool found = FindMapFile(&strMapFilename, nullptr);
  if (found && g_symbolDB.LoadMap(strMapFilename))
  {
    UpdateDebugger_MapLoaded();
    return true;
  }

  return false;
}

bool IsElfWii(const std::string& filename)
{
  /* We already check if filename existed before we called this function, so
     there is no need for another check, just read the file right away */

  size_t filesize = File::GetSize(filename);
  auto elf = std::make_unique<u8[]>(filesize);

  {
    File::IOFile f(filename, "rb");
    f.ReadBytes(elf.get(), filesize);
  }

  // Use the same method as the DOL loader uses: search for mfspr from HID4,
  // which should only be used in Wii ELFs.
  //
  // Likely to have some false positives/negatives, patches implementing a
  // better heuristic are welcome.

  // Swap these once, instead of swapping every word in the file.
  u32 HID4_pattern = Common::swap32(0x7c13fba6);
  u32 HID4_mask = Common::swap32(0xfc1fffff);
  ElfReader reader(elf.get());

  for (int i = 0; i < reader.GetNumSegments(); ++i)
  {
    if (reader.IsCodeSegment(i))
    {
      u32* code = (u32*)reader.GetSegmentPtr(i);
      for (u32 j = 0; j < reader.GetSegmentSize(i) / sizeof(u32); ++j)
      {
        if ((code[j] & HID4_mask) == HID4_pattern)
          return true;
      }
    }
  }

  return false;
}

int main(int argc, char* argv[])
{
  if (argc <= 1) {
    std::cerr << "Please provide a filename\n";
    return 1;
  }
  std::string filename(argv[1]);
  std::cout << filename << std::endl;
  // Read ELF from file
  size_t filesize = File::GetSize(filename);
  auto elf = std::make_unique<u8[]>(filesize);

  {
    File::IOFile f(filename, "rb");
    f.ReadBytes(elf.get(), filesize);
  }

  // Load ELF into GameCube Memory
  ElfReader reader(elf.get());
  if (!reader.LoadIntoMemory())
    return false;

  // Set up MSR and the BAT SPR registers.
  UReg_MSR& m_MSR = ((UReg_MSR&)PowerPC::ppcState.msr);
  m_MSR.FP = 1;
  m_MSR.DR = 1;
  m_MSR.IR = 1;
  m_MSR.EE = 1;
  PowerPC::ppcState.spr[SPR_IBAT0U] = 0x80001fff;
  PowerPC::ppcState.spr[SPR_IBAT0L] = 0x00000002;
  PowerPC::ppcState.spr[SPR_IBAT4U] = 0x90001fff;
  PowerPC::ppcState.spr[SPR_IBAT4L] = 0x10000002;
  PowerPC::ppcState.spr[SPR_DBAT0U] = 0x80001fff;
  PowerPC::ppcState.spr[SPR_DBAT0L] = 0x00000002;
  PowerPC::ppcState.spr[SPR_DBAT1U] = 0xc0001fff;
  PowerPC::ppcState.spr[SPR_DBAT1L] = 0x0000002a;
  PowerPC::ppcState.spr[SPR_DBAT4U] = 0x90001fff;
  PowerPC::ppcState.spr[SPR_DBAT4L] = 0x10000002;
  PowerPC::ppcState.spr[SPR_DBAT5U] = 0xd0001fff;
  PowerPC::ppcState.spr[SPR_DBAT5L] = 0x1000002a;
  if (IsElfWii(filename))
    HID4.SBE = 1;
  PowerPC::DBATUpdated();
  PowerPC::IBATUpdated();

  if (!reader.LoadSymbols())
  {
    if (LoadMapFromFilename())
      HLE::PatchFunctions();
  }
  else
  {
    HLE::PatchFunctions();
  }

  PC = reader.GetEntryPoint();

  // return 0;
}