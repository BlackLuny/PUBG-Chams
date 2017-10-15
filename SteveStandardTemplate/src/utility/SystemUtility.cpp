#include <stdafx.hpp>
#include <utility/SystemUtility.hpp>

namespace SteveBase::Utility {
	vector<UNLINKED_MODULE> UnlinkedModules;

	PPEB SystemUtility::GetPEB() {
#ifdef _WIN64
		return (PPEB)__readgsqword(0x60);
#else
		return (PPEB)__readfsdword(0x30);
#endif
	}

	// copypasted
	void SystemUtility::RelinkModuleToPEB(HMODULE hModule) {
		auto it = find_if(UnlinkedModules.begin(), UnlinkedModules.end(), [&](UNLINKED_MODULE val) { return val.hModule == hModule; });

		if (it == UnlinkedModules.end()) {
			//DBGOUT(TEXT("Module Not Unlinked Yet!);
			return;
		}
		auto m = *it;
		RELINK(m.Entry->InLoadOrderModuleList, m.RealInLoadOrderLinks);
		RELINK(m.Entry->InInitializationOrderModuleList, m.RealInInitializationOrderLinks);
		RELINK(m.Entry->InMemoryOrderModuleList, m.RealInMemoryOrderLinks);
		UnlinkedModules.erase(it);
	}

	// copypasted
	bool SystemUtility::UnlinkModuleFromPEB(HMODULE hModule) {
		auto it = find_if(UnlinkedModules.begin(), UnlinkedModules.end(), [&](UNLINKED_MODULE val) { return val.hModule == hModule; });
		if (it != UnlinkedModules.end()) {
			//DBGOUT(TEXT("Module Already Unlinked!);
			return false;
		}

		auto pPEB = GetPEB();
		auto CurrentEntry = pPEB->Ldr->InLoadOrderModuleList.Flink;

		for (; CurrentEntry != &pPEB->Ldr->InLoadOrderModuleList && CurrentEntry != nullptr; CurrentEntry = CurrentEntry->Flink) {
			PLDR_MODULE Current = CONTAINING_RECORD(CurrentEntry, LDR_MODULE, InLoadOrderModuleList);
			if (Current->BaseAddress == hModule) {
				UNLINKED_MODULE CurrentModule;
				CurrentModule.hModule = hModule;
				CurrentModule.RealInLoadOrderLinks = Current->InLoadOrderModuleList.Blink->Flink;
				CurrentModule.RealInInitializationOrderLinks = Current->InInitializationOrderModuleList.Blink->Flink;
				CurrentModule.RealInMemoryOrderLinks = Current->InMemoryOrderModuleList.Blink->Flink;
				CurrentModule.Entry = Current;
				UnlinkedModules.push_back(CurrentModule);

				UNLINK(Current->InLoadOrderModuleList);
				UNLINK(Current->InInitializationOrderModuleList);
				UNLINK(Current->InMemoryOrderModuleList);

				return true;
			}
		}

		return false;
	}
	
#if 0
	// copypasted
#pragma section(".CRT$XLY", long, read)
	__declspec(thread) int var = 0xDEADBEEF;
	VOID NTAPI TlsCallback(PVOID DllHandle, DWORD Reason, VOID *Reserved) {
		var = 0xB15BADB0; // Required for TLS Callback call
		if (IsDebuggerPresent()) {
			TerminateProcess(GetCurrentProcess(), 0);
		}
	}
	__declspec(allocate(".CRT$XLY")) PIMAGE_TLS_CALLBACK g_tlsCallback = TlsCallback;
#endif
}
