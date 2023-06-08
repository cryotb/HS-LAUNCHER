#pragma once

namespace Games {
	class C_GameBase;
}

class C_OverlayCore final
{
public:
	VOID OnProgramBegin(PVOID pGameHandlerMem);
	VOID OnExitRequest();
	VOID OnProgramEnd();

	VOID OnTick();

	VOID Render();
	VOID Think();

	BOOLEAN IsActive() CONST { return m_bActive; }

	VOID AddHandler(Games::C_GameBase* pHandler)
	{
		m_Handlers.emplace_back(pHandler);
	}

	[[nodiscard]] SIZE_T GetNumHandlers() CONST
	{
		return m_Handlers.size();
	}

	[[nodiscard]] Games::C_GameBase* GetLatestHandler() CONST {
		if (GetNumHandlers() <= NULL)
			return nullptr;

		return m_Handlers.front();
	}
private:
	struct WindowData_t
	{
		HWND m_Handle;
		WNDCLASSEX m_wClass{};
	} m_WindowData{};

	std::vector<Games::C_GameBase*> m_Handlers;
	
	BOOLEAN m_bActive = FALSE;
};
