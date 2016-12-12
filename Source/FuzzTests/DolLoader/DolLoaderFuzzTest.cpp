#include "Core/Boot/Boot_DOL.h"
#include "DiscIO/NANDContentLoader.h"
#include "DiscIO/Enums.h"
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
#include "Core/HW/DVDInterface.h"

// bool EmulatedBS2_Wii()
// {
//   INFO_LOG(BOOT, "Faking Wii BS2...");

//   // Setup Wii memory
//   DiscIO::Country country_code = DiscIO::Country::COUNTRY_UNKNOWN;
//   if (DVDInterface::VolumeIsValid())
//     country_code = DVDInterface::GetVolume().GetCountry();
//   if (SetupWiiMemory(country_code) == false)
//     return false;

//   // Execute the apploader
//   bool apploaderRan = false;
//   if (DVDInterface::VolumeIsValid() &&
//       DVDInterface::GetVolume().GetVolumeType() == DiscIO::Platform::WII_DISC)
//   {
//     // This is some kind of consistency check that is compared to the 0x00
//     // values as the game boots. This location keeps the 4 byte ID for as long
//     // as the game is running. The 6 byte ID at 0x00 is overwritten sometime
//     // after this check during booting.
//     DVDRead(0, 0x3180, 4, true);

//     // Set up MSR and the BAT SPR registers.
//     UReg_MSR& m_MSR = ((UReg_MSR&)PowerPC::ppcState.msr);
//     m_MSR.FP = 1;
//     m_MSR.DR = 1;
//     m_MSR.IR = 1;
//     m_MSR.EE = 1;
//     PowerPC::ppcState.spr[SPR_IBAT0U] = 0x80001fff;
//     PowerPC::ppcState.spr[SPR_IBAT0L] = 0x00000002;
//     PowerPC::ppcState.spr[SPR_IBAT4U] = 0x90001fff;
//     PowerPC::ppcState.spr[SPR_IBAT4L] = 0x10000002;
//     PowerPC::ppcState.spr[SPR_DBAT0U] = 0x80001fff;
//     PowerPC::ppcState.spr[SPR_DBAT0L] = 0x00000002;
//     PowerPC::ppcState.spr[SPR_DBAT1U] = 0xc0001fff;
//     PowerPC::ppcState.spr[SPR_DBAT1L] = 0x0000002a;
//     PowerPC::ppcState.spr[SPR_DBAT4U] = 0x90001fff;
//     PowerPC::ppcState.spr[SPR_DBAT4L] = 0x10000002;
//     PowerPC::ppcState.spr[SPR_DBAT5U] = 0xd0001fff;
//     PowerPC::ppcState.spr[SPR_DBAT5L] = 0x1000002a;
//     PowerPC::DBATUpdated();
//     PowerPC::IBATUpdated();

//     Memory::Write_U32(0x4c000064, 0x00000300);  // Write default DSI Handler:     rfi
//     Memory::Write_U32(0x4c000064, 0x00000800);  // Write default FPU Handler:     rfi
//     Memory::Write_U32(0x4c000064, 0x00000C00);  // Write default Syscall Handler: rfi

//     HLE::Patch(0x81300000, "OSReport");  // HLE OSReport for Apploader

//     PowerPC::ppcState.gpr[1] = 0x816ffff0;  // StackPointer

//     const u32 apploader_offset = 0x2440;  // 0x1c40;

//     // Load Apploader to Memory
//     const DiscIO::IVolume& volume = DVDInterface::GetVolume();
//     u32 apploader_entry, apploader_size;
//     if (!volume.ReadSwapped(apploader_offset + 0x10, &apploader_entry, true) ||
//         !volume.ReadSwapped(apploader_offset + 0x14, &apploader_size, true) ||
//         apploader_entry == (u32)-1 || apploader_size == (u32)-1)
//     {
//       ERROR_LOG(BOOT, "Invalid apploader. Probably your image is corrupted.");
//       return false;
//     }
//     DVDRead(apploader_offset + 0x20, 0x01200000, apploader_size, true);

//     // call iAppLoaderEntry
//     DEBUG_LOG(BOOT, "Call iAppLoaderEntry");

//     u32 iAppLoaderFuncAddr = 0x80004000;
//     PowerPC::ppcState.gpr[3] = iAppLoaderFuncAddr + 0;
//     PowerPC::ppcState.gpr[4] = iAppLoaderFuncAddr + 4;
//     PowerPC::ppcState.gpr[5] = iAppLoaderFuncAddr + 8;
//     RunFunction(apploader_entry);
//     u32 iAppLoaderInit = PowerPC::Read_U32(iAppLoaderFuncAddr + 0);
//     u32 iAppLoaderMain = PowerPC::Read_U32(iAppLoaderFuncAddr + 4);
//     u32 iAppLoaderClose = PowerPC::Read_U32(iAppLoaderFuncAddr + 8);

//     // iAppLoaderInit
//     DEBUG_LOG(BOOT, "Run iAppLoaderInit");
//     PowerPC::ppcState.gpr[3] = 0x81300000;
//     RunFunction(iAppLoaderInit);

//     // Let the apploader load the exe to memory. At this point I get an unknown IPC command
//     // (command zero) when I load Wii Sports or other games a second time. I don't notice
//     // any side effects however. It's a little disconcerting however that Start after Stop
//     // behaves differently than the first Start after starting Dolphin. It means something
//     // was not reset correctly.
//     DEBUG_LOG(BOOT, "Run iAppLoaderMain");
//     do
//     {
//       PowerPC::ppcState.gpr[3] = 0x81300004;
//       PowerPC::ppcState.gpr[4] = 0x81300008;
//       PowerPC::ppcState.gpr[5] = 0x8130000c;

//       RunFunction(iAppLoaderMain);

//       u32 iRamAddress = PowerPC::Read_U32(0x81300004);
//       u32 iLength = PowerPC::Read_U32(0x81300008);
//       u32 iDVDOffset = PowerPC::Read_U32(0x8130000c) << 2;

//       INFO_LOG(BOOT, "DVDRead: offset: %08x   memOffset: %08x   length: %i", iDVDOffset,
//                iRamAddress, iLength);
//       DVDRead(iDVDOffset, iRamAddress, iLength, true);
//     } while (PowerPC::ppcState.gpr[3] != 0x00);

//     // iAppLoaderClose
//     DEBUG_LOG(BOOT, "Run iAppLoaderClose");
//     RunFunction(iAppLoaderClose);

//     apploaderRan = true;

//     // Pass the "#002 check"
//     // Apploader writes the IOS version and revision here, we copy it
//     // Fake IOSv9 r2.4 if no version is found (elf loading)
//     u32 firmwareVer = PowerPC::Read_U32(0x80003188);
//     PowerPC::Write_U32(firmwareVer ? firmwareVer : 0x00090204, 0x80003140);

//     // Load patches and run startup patches
//     PatchEngine::LoadPatches();

//     // return
//     PC = PowerPC::ppcState.gpr[3];
//   }

//   return apploaderRan;
// }

// bool EmulatedBS2(bool _bIsWii)
// {
//   return _bIsWii ? EmulatedBS2_Wii() : EmulatedBS2_GC();
// }

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

int main(int argc, char* argv[]) {
  SConfig& _StartupPara = SConfig::GetInstance();

  CDolLoader dolLoader(_StartupPara.m_strFilename);
  if (!dolLoader.IsValid())
    return false;
  
  dolLoader.IsWii();

  // // Check if we have gotten a Wii file or not
  // bool dolWii = dolLoader.IsWii();
  // if (dolWii != _StartupPara.bWii)
  // {
  //   PanicAlertT("Warning - starting DOL in wrong console mode!");
  // }

  bool BS2Success = false;

  // if (dolWii)
  // {
  //   BS2Success = EmulatedBS2(dolWii);
  // }
  // else if ((!DVDInterface::VolumeIsValid() ||
  //           DVDInterface::GetVolume().GetVolumeType() != DiscIO::Platform::WII_DISC) &&
  //           !_StartupPara.m_strDefaultISO.empty())
  // {
  //   DVDInterface::SetVolumeName(_StartupPara.m_strDefaultISO);
  //   BS2Success = EmulatedBS2(dolWii);
  // }

  // if (!_StartupPara.m_strDVDRoot.empty())
  // {
  //   NOTICE_LOG(BOOT, "Setting DVDRoot %s", _StartupPara.m_strDVDRoot.c_str());
  //   DVDInterface::SetVolumeDirectory(_StartupPara.m_strDVDRoot, dolWii,
  //                                     _StartupPara.m_strApploader, _StartupPara.m_strFilename);
  //   BS2Success = EmulatedBS2(dolWii);
  // }

  // DVDInterface::SetDiscInside(DVDInterface::VolumeIsValid());

  if (!BS2Success)
  {
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
    if (dolLoader.IsWii())
      HID4.SBE = 1;
    PowerPC::DBATUpdated();
    PowerPC::IBATUpdated();

    dolLoader.Load();
    PC = dolLoader.GetEntryPoint();
  }

  if (LoadMapFromFilename())
    HLE::PatchFunctions();
  
  return 0;
}