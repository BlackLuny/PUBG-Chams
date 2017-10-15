#pragma once

// #define DEBUG 0
#pragma warning(disable: 4996) // The POSIX name for this item is deprecated
#pragma warning(disable: 4554)
#pragma warning(disable: 4307) // integer overflow
#pragma warning(disable: 4244)
#pragma warning(disable: 4146)
#pragma warning(disable: 4305) // truncation from 'bigger' to 'smaller' 
#pragma warning(disable: 4309) // truncation of constant value
#pragma warning(disable: 4307) // integral constant overflow
#pragma warning(disable: 4308) // negative integral constant converted to unsigned type

#ifdef _MSC_VER
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _USE_MATH_DEFINES
#define NOMINMAX

// #pragma comment(lib, "vendor\\PolyHook-master\\Capstone\\msvc\\x64\\capstone.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shlwapi.lib")

#include <d3dcompiler.h>
#include <d3d11.h>
#include <D3D11Shader.h>
#include <windows.h>
#include <process.h>
#include <psapi.h>
#include <shlwapi.h>
#include <tchar.h>
#include <tlhelp32.h>


#include <WinSock2.h>


#include <array>
#include <algorithm>
#include <any>
#include <cmath>
#include <codecvt>
#include <cstdint>
#include <cstdlib>
#include <ctime>
// #include <experimental/generator>
#include <fstream>
#include <functional>
// #include <future>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include <vendor/debugbreak/debugbreak.h>
#include <vendor/fmt/format.h>
#include <vendor/hash/hash.h>
// #include <vendor/nlohmann/json.hpp>
#include <vendor/rang/include/rang.hpp>
#include <vendor/XorStr/XorStr.h>
#include <vendor/reflective_loader/loader.h>
// #include <vendor/minhook/include/MinHook.h>

#include <misc/DataStructure.hpp>
#include <misc/FunctionalProgramming.hpp>
// #include <misc/IJsonSerializable.hpp>
#include <misc/Macro.hpp>
#include <misc/TemplateMetaprogramming.hpp>
#include <misc/TypeAliases.hpp>

#include <Interface.hpp>
#include <Singleton.hpp>