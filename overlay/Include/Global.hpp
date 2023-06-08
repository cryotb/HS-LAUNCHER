#pragma once

namespace G
{
	namespace H
	{
		inline HHOOK Keyboard = nullptr;
		inline HHOOK Mouse = nullptr;
	}
	
	inline INT BackendLatency = 0;
	inline FLOAT CurrentTime = 0.0f;
	inline BOOLEAN IsDebugBuild = FALSE;
	inline BOOLEAN KernelMode = FALSE;
	inline BOOLEAN IsAttached = FALSE;
	inline BOOLEAN IsGdmLoadBlocked = TRUE;
	inline BOOLEAN IsNoGuiMode = FALSE;
	inline HINSTANCE Inst = nullptr;
	inline HWND WindowHandle = nullptr;
	inline IDirect3D9* Direct3d9 = nullptr;
	inline IDirect3DDevice9* DirectDevice9 = nullptr;
	inline net::c_client NetClient{};
	inline shared::storage::c_helper StorageHpr{};

	inline std::string CurrentUserName{};

	extern void ExportInit();
	extern std::map<std::string, void*> ExportTable;

	// used to check if kernel mode aka hardened restrictions apply to the program's exection flow.
	INLINE BOOLEAN IsInKernelMode()
	{
		return KernelMode;
	}
}
