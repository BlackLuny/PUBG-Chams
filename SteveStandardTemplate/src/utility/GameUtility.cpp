#include <stdafx.hpp>
#include <utility/GameUtility.hpp>

namespace SteveBase::Utility {
	string m_hackDllDirectory;
	string m_hackConfigLocation;
	bool m_cheatRunning;

	void GameUtility::SetCheatRunning(bool running) {
		m_cheatRunning = running;
	}

	bool GameUtility::GetCheatRunning() {
		return m_cheatRunning;
	}

	void GameUtility::SetHackDirectory(string modPath) {
		m_hackDllDirectory = modPath;
	}

	string GameUtility::GetHackDirectory() {
		return m_hackDllDirectory;
	}

	void GameUtility::SetHackConfigLocation(string location) {
		m_hackConfigLocation = location;
	}

	string GameUtility::GetHackConfigLocation() {
		return m_hackConfigLocation;
	}
}