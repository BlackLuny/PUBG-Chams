#pragma once

#include <stdafx.hpp>
#include <winternl.h>

#undef or // <ciso646> wtf?

namespace SteveBase::Utility {
	using namespace std;

	// RelinkModuleToPEB, UnlinkModuleFromPEB, RemovePeHeader
	// _UNICODE_STRING, _PEB_LDR_DATA, _PEB, _LDR_MODULE, _UNLINKED_MODULE 
	// => https://gist.github.com/Fonger/15268efb19eb328431b0de7560ddcb53


	typedef struct _PEB_LDR_DATA {
		ULONG      Length;
		BOOLEAN    Initialized;
		PVOID      SsHandle;
		LIST_ENTRY InLoadOrderModuleList;
		LIST_ENTRY InMemoryOrderModuleList;
		LIST_ENTRY InInitializationOrderModuleList;
	} PEB_LDR_DATA, *PPEB_LDR_DATA;

	typedef struct _PEB {
#ifdef _WIN64
		UINT8 _PADDING_[24];
#else
		UINT8 _PADDING_[12];
#endif
		PEB_LDR_DATA* Ldr;
	} PEB, *PPEB;

	typedef struct _LDR_MODULE {
		LIST_ENTRY      InLoadOrderModuleList;
		LIST_ENTRY      InMemoryOrderModuleList;
		LIST_ENTRY      InInitializationOrderModuleList;
		PVOID           BaseAddress;
		PVOID           EntryPoint;
		ULONG           SizeOfImage;
		UNICODE_STRING  FullDllName;
		UNICODE_STRING  BaseDllName;
		ULONG           Flags;
		SHORT           LoadCount;
		SHORT           TlsIndex;
		LIST_ENTRY      HashTableEntry;
		ULONG           TimeDateStamp;
	} LDR_MODULE, *PLDR_MODULE;

	typedef struct _UNLINKED_MODULE {
		HMODULE hModule;
		PLIST_ENTRY RealInLoadOrderLinks;
		PLIST_ENTRY RealInMemoryOrderLinks;
		PLIST_ENTRY RealInInitializationOrderLinks;
		PLDR_MODULE Entry; // =PLDR_DATA_TABLE_ENTRY
	} UNLINKED_MODULE;

#define UNLINK(x)					\
	(x).Flink->Blink = (x).Blink;	\
	(x).Blink->Flink = (x).Flink;

#define RELINK(x, real)			\
	(x).Flink->Blink = (real);	\
	(x).Blink->Flink = (real);	\
	(real)->Blink = (x).Blink;	\
	(real)->Flink = (x).Flink;

	// some generic functions for the lulz
	class SystemUtility {
	public:
        // darkstorm copypasted
		static __forceinline HMODULE SafeGetModuleHandle(string name) {
            HMODULE hModule;
            for (hModule = nullptr; hModule == nullptr; Sleep(1)) {
                hModule = GetModuleHandle(name.data());
            }
            return hModule;
		}
		static __forceinline MODULEINFO GetModuleInfo(string moduleName) {
            MODULEINFO modinfo;
            const auto moduleHandle = SafeGetModuleHandle(moduleName);
            GetModuleInformation(GetCurrentProcess(), moduleHandle, &modinfo, sizeof(MODULEINFO));
            return modinfo;
		}
        // learn_more copypasted
		static __forceinline PBYTE FindPattern(PBYTE dwAddress, DWORD dwLength, string pattern) {
#define INRANGE(x,a,b)		(x >= a && x <= b) 
#define getBits( x )		(INRANGE(x,'0','9') ? (x - '0') : ((x&(~0x20)) - 'A' + 0xa))
#define getByte(x) (getBits(x[0]) << 4 | getBits(x[1]))
            auto oldStr = pattern.data();
            auto pat = oldStr;
            PBYTE firstMatch = nullptr;
            for (auto pCur = dwAddress; pCur < dwAddress + dwLength; pCur++) {
                if (!*pat) return firstMatch;
                if (*(PBYTE)pat == '\?' || *(BYTE*)pCur == getByte(pat)) {
                    if (!firstMatch) firstMatch = pCur;
                    if (!pat[2]) return firstMatch;
                    if (*(PWORD)pat == '\?\?' || *(PBYTE)pat != '\?') pat += 3;
                    else pat += 2;
                } else {
                    pat = oldStr;
                    firstMatch = nullptr;
                }
            }
            return nullptr;
		}
		static __forceinline PBYTE FindModulePattern(string moduleDll, string pattern) {
            const auto info = GetModuleInfo(moduleDll);
            return FindPattern((PBYTE)info.lpBaseOfDll, info.SizeOfImage, pattern);
		}
        // copypasted
		static __forceinline string GetLastErrorAsString() {
            //Get the error message, if any.
            auto errorMessageID = GetLastError();
            if (errorMessageID == 0) {
                return string(); //No error message has been recorded
            }

            LPSTR messageBuffer = nullptr;
            size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

            string message(messageBuffer, size);

            //Free the buffer.
            LocalFree(messageBuffer);

            return message;
		}

		static __forceinline string ProduceModulePath(HINSTANCE dllHandle) {
            char buffer[4096];
            GetModuleFileName(dllHandle, buffer, 4096);
            PathRemoveFileSpec(buffer);
            const size_t len = strlen(buffer);

            buffer[len] = '\\';
            buffer[len + 1] = '\0';
            return string(buffer);
		}

        static PPEB GetPEB();
		static void RelinkModuleToPEB(HMODULE hModule);
		static bool UnlinkModuleFromPEB(HMODULE hModule);

		static __forceinline bool RemovePeHeader(HANDLE GetModuleBase) {
            const auto pDosHeader = (PIMAGE_DOS_HEADER)GetModuleBase;
            const auto pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader + (DWORD)pDosHeader->e_lfanew);

            if (pNTHeader->Signature != IMAGE_NT_SIGNATURE) {
                return false;
            }

            if (pNTHeader->FileHeader.SizeOfOptionalHeader) {
                DWORD Protect;
                auto Size = pNTHeader->FileHeader.SizeOfOptionalHeader;
                VirtualProtect((void*)GetModuleBase, Size, PAGE_EXECUTE_READWRITE, &Protect);
                SecureZeroMemory((void*)GetModuleBase, Size);
                VirtualProtect((void*)GetModuleBase, Size, Protect, &Protect);
            }
            return true;
        }

#if 0
	    static __forceinline array<int, 4> GetCPUID() {
            array<int, 4> ret;
            __cpuid(ret.data(), 0);
            return ret;
        }


        // copypasted
		static __forceinline bool IsTrapFlagRaised() {
            BOOL isDebugged = TRUE;
            __try {
                __asm
                {
                    pushfd
                    or dword ptr[esp], 0x100 // set the Trap Flag 
                    popfd                    // Load the value into EFLAGS register
                    nop
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                // If an exception has been raised – debugger is not present
                isDebugged = FALSE;
            }
            return isDebugged == TRUE; // may be reduntant but BOOL and bool are definitely two different types so i added a check
                                       // BOOL is 32 bits and bool is 8 bits in winapi only BOOLEAN is 8 bits
        }

        // copypasted
		static __forceinline bool IsDebuggerFound() {
            BOOL debuggerPresent = FALSE;
            CheckRemoteDebuggerPresent(GetCurrentProcess(), &debuggerPresent);
            return IsDebuggerPresent() || debuggerPresent || IsTrapFlagRaised();
        }
#endif
	};
}