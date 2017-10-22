#include <stdafx.hpp>
#include <utility/GameUtility.hpp>
#include <utility/Logger.hpp>
#include <utility/SystemUtility.hpp>
#include <manager/HookManager.hpp>
#include <manager/PatternManager.hpp>

namespace {
    using namespace fmt::literals;
    using namespace SteveBase;
    // using namespace Manager;
    using namespace Utility;

    void KillHack(HMODULE moduleDll) {
        LoggerNotice("Hack Unloaded");

        // GameHookManager::StopHook();

#if 0
        ShowWindow(GetConsoleWindow(), SW_HIDE);
        FreeConsole();
#endif

#if RELEASE
        SystemUtility::RelinkModuleToPEB(moduleDll);
#endif
        FreeLibraryAndExitThread(moduleDll, EXIT_SUCCESS);

    }

    DWORD WINAPI CheatThread(LPVOID param) {
        //DisableThreadLibraryCalls(thisModule);

#if DEBUG
        // MessageBox(nullptr, "SUCCESS", "", 0);
        AllocConsole() ? freopen(text("CONOUT$").c_str(), "w", stdout) : 0;
#endif

#if RELEASE
        SystemUtility::RemovePeHeader((HANDLE)param);
        // SystemUtility::UnlinkModuleFromPEB((HMODULE)param);
#endif
        // GameUtility::SetHackDirectory(SystemUtility::ProduceModulePath((HINSTANCE)param));

        LoggerNotice("Hello World!");

        Manager::PatternManager::Init();
        Manager::HookManager::Init();

        LoggerNotice("All Set. Have A Nice Day!");
        // GameUtility::SetCheatRunning(true);

#if 0
        while (GameUtility::GetCheatRunning() && !GetAsyncKeyState(VK_DELETE)) {
            Sleep(1000);
        }

        GameUtility::SetCheatRunning(false);
        KillHack((HMODULE)param);
#endif

#if 0
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)[](LPVOID param2) -> DWORD {
            for (; !SystemUtility::IsDebuggerFound(); Sleep(1));
            GameUtility::SetCheatRunning(false);
            KillHack((HMODULE)param2);
        }, param, 0, nullptr);
#endif

        return 0;

    }
}

extern HINSTANCE hAppInstance;

#pragma code_seg("001")  
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved) {

    switch (dwReason) {
        case DLL_QUERY_HMODULE: {
            if (lpReserved != nullptr) {
                *(HMODULE *)lpReserved = hAppInstance;
            }
            break;
        }
        case DLL_PROCESS_ATTACH: {
            hAppInstance = hinstDLL;
            DWORD oldProt;

            /* Cleanup */
            const ReflectiveLoader::NtContinue fnNtContinue = (ReflectiveLoader::NtContinue)(*(uintptr_t*)(0x55550000));
            VirtualProtect(fnNtContinue, 8, PAGE_EXECUTE_READ, &oldProt);
            VirtualFree((LPVOID)0x55550000, 0, MEM_RELEASE);
            VirtualFree((LPVOID)0x55560000, 0, MEM_RELEASE);
            /*End of Cleanup*/

            DisableThreadLibraryCalls(hinstDLL);
            CreateThread(nullptr, 0, CheatThread, hinstDLL, 0, nullptr);
            fnNtContinue((PCONTEXT)lpReserved, false);

            break;
        }
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
        break;
    }

    return TRUE;
}