#include <winnt.h>
#include "TDM_handler.h"

namespace logger = SKSE::log;

namespace TDM_handler {

    constexpr std::uintptr_t kRva_uTargetLockKey = 0x0000b2114;
    constexpr std::uint32_t expected_TDM_DLL_SIZE = 0xC2000;

    HMODULE TDM;

    
    inline const std::uint32_t* g_uTargetLockKeyAddr = nullptr;

    bool isSafeToRead = false;

    // Basic “is readable” check to avoid obvious crashes if something is wrong.
    bool IsReadableAddress(const void* p, std::size_t bytes) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (::VirtualQuery(p, &mbi, sizeof(mbi)) == 0) {
            return false;
        }

        const auto protect = mbi.Protect;
        const bool readable = (protect & PAGE_READONLY) || (protect & PAGE_READWRITE) || (protect & PAGE_WRITECOPY) ||
                              (protect & PAGE_EXECUTE_READ) || (protect & PAGE_EXECUTE_READWRITE) ||
                              (protect & PAGE_EXECUTE_WRITECOPY);

        if (!readable) {
            return false;
        }

        // Ensure the full range is within the same region and committed.
        if (mbi.State != MEM_COMMIT) {
            return false;
        }

        const auto start = reinterpret_cast<std::uintptr_t>(p);
        const auto regionStart = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
        const auto regionEnd = regionStart + mbi.RegionSize;

        return (start + bytes) <= regionEnd;
    }

    void SetupTDMHandler() {
        TDM = GetModuleHandleW(L"TrueDirectionalMovement.dll");
        if (!TDM) {
            logger::info("TDM .dll not found, aborting TDM DLL Handler setup");
            return;
        }

        const auto base = reinterpret_cast<const std::uint8_t*>(TDM);

        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
            logger::info("TDM .dll IMAGE_DOS_SIGNATURE not found, aborting TDM DLL Handler setup");
            return;
        }

        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) {  // 'PE\0\0'
            logger::info("TDM .dll IMAGE_NT_SIGNATURE not found, aborting TDM DLL Handler setup");
            return;
        }

        if (nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            logger::info("TDM .dll IMAGE_NT_OPTIONAL_HDR64_MAGIC not found, aborting TDM DLL Handler setup");
            return;
        }

        

        std::uint32_t TDM_DLL_SIZE = nt->OptionalHeader.SizeOfImage;
        logger::info("TDM DLL SIZE: {}", TDM_DLL_SIZE);

        if (TDM_DLL_SIZE != expected_TDM_DLL_SIZE) {
            logger::info("TDM SIZE NOT MATCH! No uTargetLockKey reading");
            return;
        }

        g_uTargetLockKeyAddr = reinterpret_cast<const std::uint32_t*>(base + kRva_uTargetLockKey);

        if (!IsReadableAddress(g_uTargetLockKeyAddr, sizeof(std::uint32_t))) {
            logger::info("TDM address is not READABLE! WTF");
            return;
        }

         logger::info("TDM uTargetLockKey can be read!");
         isSafeToRead = true;
    }

    bool IsSafe() { return isSafeToRead; }

    std::uint32_t Read_uTargetLockKey(RE::StaticFunctionTag*) {

        if (isSafeToRead) {
            const auto value = *reinterpret_cast<volatile const std::uint32_t*>(g_uTargetLockKeyAddr);
            return static_cast<std::uint32_t>(value);
        } else {
            return 0;
        }
    }
}