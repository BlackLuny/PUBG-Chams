#pragma once

#include <stdafx.hpp>

namespace SteveBase::Utility {
	using namespace std;

	class GameUtility {
	public:
		static void SetCheatRunning(bool running);
		static bool GetCheatRunning();

		static void SetHackDirectory(string modPath);
		static string GetHackDirectory();

		static void SetHackConfigLocation(string location);
		static string GetHackConfigLocation();
	};
}