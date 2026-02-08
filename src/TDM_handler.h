#pragma once

namespace TDM_handler {
    
    void SetupTDMHandler();
    bool IsSafe();
    std::uint32_t Read_uTargetLockKey(RE::StaticFunctionTag*);

}

