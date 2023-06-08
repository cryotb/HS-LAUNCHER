#include <Include.hpp>

VOID C_OverlayCore::OnProgramBegin(PVOID pGameHandlerMem)
{
	CONST AUTO SetupWindowClass = [](HINSTANCE hInst, WNDCLASSEX& sWindowClass, WNDPROC fWndProc, LPCSTR szWindowClass)
	{
		sWindowClass.cbSize = sizeof(WNDCLASSEX);
		sWindowClass.style = CS_HREDRAW | CS_VREDRAW;
		sWindowClass.lpfnWndProc = fWndProc;
		sWindowClass.cbClsExtra = 0;
		sWindowClass.cbWndExtra = 0;
		sWindowClass.hInstance = hInst;
		sWindowClass.hIcon = LoadIcon(hInst, IDI_APPLICATION);
		sWindowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		sWindowClass.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
		sWindowClass.lpszMenuName = nullptr;
		sWindowClass.lpszClassName = szWindowClass;
		sWindowClass.hIconSm = LoadIcon(sWindowClass.hInstance, IDI_APPLICATION);
	};

	CONST AUTO SetupDirectDevice = [](HWND hWindow)
	{
		D3DPRESENT_PARAMETERS dPresentParams{};
		ZeroMemory(&dPresentParams, sizeof(dPresentParams));

		dPresentParams.Windowed = TRUE;
		dPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
		dPresentParams.hDeviceWindow = hWindow;

		dPresentParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		dPresentParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

		if (G::Direct3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		                               &dPresentParams, &G::DirectDevice9) != D3D_OK)
			return FALSE;

		if (G::DirectDevice9 == nullptr)
			return FALSE;

		return TRUE;
	};

	if (pGameHandlerMem == nullptr)
		return;

	AUTO* CONST pGameHandler = static_cast<Games::C_GameBase*>(pGameHandlerMem);

	if (pGameHandler == nullptr)
		return;

	CONST AUTO sGameHandlerChars = pGameHandler->GetCharacteristics();

	if (sGameHandlerChars.m_bSupportsOverlay == FALSE)
		return;

	CONST AUTO szWindowTitle = Util::RandomString(12);
	CONST AUTO szWindowClassName = Util::RandomString();

	SetupWindowClass(G::Inst, m_WindowData.m_wClass, static_cast<WNDPROC>(&CB::WindowProcess), szWindowClassName.c_str());

	if (RegisterClassEx(&m_WindowData.m_wClass) == FALSE)
		return;

	RECT sTargetWindowCoords{};
	BOOLEAN bResizeToTargetWnd = FALSE;

	if(!G::IsInKernelMode())
	{
		CONST AUTO hWindow = FindWindow(nullptr, sGameHandlerChars.m_WindowName);

		if (hWindow != nullptr)
		{
			GetWindowRect(hWindow, &sTargetWindowCoords);
			G::P::Screen = {sTargetWindowCoords.right, sTargetWindowCoords.bottom};
			bResizeToTargetWnd = TRUE;
		}
	}
	
	if (G::IsDebugBuild)
	{
		CONST AUTO dwResult = MESSAGE_BOX("do you want to enable overlay?", MB_ICONQUESTION | MB_YESNO);

		if (dwResult == IDYES)
			loader::options::gdm_load.m_enable_overlay = true;
	}

	if (!loader::options::gdm_load.m_enable_overlay)
	{
		G::IsNoGuiMode = TRUE;
		m_bActive = TRUE;
		return;
	}

	G::IsNoGuiMode = FALSE;

	BOOLEAN bHijackWindow = FALSE;

	if(bHijackWindow)
	{
		auto walker = C_WindWalker{};

		if (!walker.Populate())
			return;

		auto window_opt = walker.FindViableWindow();

		if (window_opt == std::nullopt)
		{
			Log::Error(_XS("failed to find hijackable window."));
			return;
		}
		
		auto window = *window_opt;

		Log::Info(_XS("hijacking window named 'W:{}/C:{}' in '{}'"), window.Name, window.ClassName, window.OwnerProcName);

		m_WindowData.m_Handle = window.Handle;
	} else
	{
		Log::Info(_XS("creating window instead, since we're in debug mode."));

		m_WindowData.m_Handle = CreateWindow(szWindowClassName.c_str(), szWindowTitle.c_str(), WS_OVERLAPPEDWINDOW, 0, 0, G::P::Screen.w, G::P::Screen.h,
			nullptr, nullptr, G::Inst, nullptr);
	}

	if (m_WindowData.m_Handle == nullptr)
		return;

	G::WindowHandle = m_WindowData.m_Handle;

	SetWindowPos(m_WindowData.m_Handle, nullptr/*HWND_TOPMOST*/, 0, 0, G::P::Screen.w, G::P::Screen.h, SWP_NOMOVE | SWP_NOSIZE);
	SetWindowLong(m_WindowData.m_Handle, GWL_EXSTYLE, static_cast<int>(GetWindowLong(m_WindowData.m_Handle, GWL_EXSTYLE)) | WS_EX_LAYERED /*| WS_EX_TOPMOST*/);
	
	//SetLayeredWindowAttributes(m_WindowData.m_Handle, RGB(0, 0, 0), 0, ULW_COLORKEY);
	//SetLayeredWindowAttributes(m_WindowData.m_Handle, 0, 255, LWA_ALPHA); <-- THIS CAUSED THE ISSUE!

	SetWindowLongPtr(m_WindowData.m_Handle, GWL_STYLE, WS_VISIBLE);
	SetWindowLongPtr(m_WindowData.m_Handle, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT);
	SetWindowPos(m_WindowData.m_Handle, nullptr/*HWND_NOTOPMOST*/, 0, 0, G::P::Screen.w, G::P::Screen.h, SWP_SHOWWINDOW);
	
	MARGINS margins = { 0, 0, G::P::Screen.w, G::P::Screen.h };

	FIND_EXPORT("dwmapi.dll", DwmExtendFrameIntoClientArea)(m_WindowData.m_Handle, &margins);

	{
		LONG lStyle = GetWindowLong(m_WindowData.m_Handle, GWL_STYLE);
		lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
		SetWindowLong(m_WindowData.m_Handle, GWL_STYLE, lStyle);
	}

	if (bResizeToTargetWnd)
		SetWindowPos(m_WindowData.m_Handle, HWND_TOPMOST, sTargetWindowCoords.left , sTargetWindowCoords.top - 22, sTargetWindowCoords.right, sTargetWindowCoords.bottom, NULL);
	else
		SetWindowPos(m_WindowData.m_Handle, HWND_TOPMOST, 0, 0, G::P::Screen.w, G::P::Screen.h, NULL);

	ShowWindow(m_WindowData.m_Handle, 0xA);
	UpdateWindow(m_WindowData.m_Handle);

	G::Direct3d9 = FIND_EXPORT("d3d9.dll", Direct3DCreate9)(D3D_SDK_VERSION);

	if (G::Direct3d9 == nullptr)
	{
		MESSAGE_BOX("failed to create d3d device.", MB_ICONERROR);
		return;
	}

	if (!SetupDirectDevice(m_WindowData.m_Handle))
	{
		MESSAGE_BOX("failed to setup d3d device.", MB_ICONERROR);
		return;
	}

	G::P::Render->initialize(G::DirectDevice9);

	m_bActive = TRUE;
}

VOID C_OverlayCore::OnExitRequest()
{
	m_bActive = FALSE;
}

VOID C_OverlayCore::OnTick()
{
	Think();
	Render();
}

VOID C_OverlayCore::Think()
{
	
}

VOID C_OverlayCore::Render()
{
	static bool bInitialized = false;

	if (!G::Direct3d9 || !G::DirectDevice9)
		return;

	if (!bInitialized)
	{
		SetLayeredWindowAttributes(m_WindowData.m_Handle, RGB(0, 0, 0), 0, ULW_COLORKEY);
		bInitialized = true;
	}
	
	G::DirectDevice9->Clear(NULL, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 0.0f, NULL);

	G::DirectDevice9->BeginScene();
	G::P::Render->pre_render(D3DSBT_ALL);

	if (G::P::CurrentGameHandler && G::P::CurrentGameHandler->IsActive())
		G::P::CurrentGameHandler->OnRender();

	if (G::P::Input && G::P::Input->is_ready())
	{
		const auto cursor_pos = G::P::Input->get_cursor_position();

		G::P::Windows->end_scene(cursor_pos, G::P::Input);

		if (G::IsDebugBuild)
			G::P::Render->draw_square(cursor_pos.x - 2.f, cursor_pos.y - 2.f, cursor_pos.x + 2.f, cursor_pos.y + 2.f, Shared::Colors::white);
	}

	G::P::Render->post_render();
	G::DirectDevice9->EndScene();

	G::DirectDevice9->Present(nullptr, nullptr, nullptr, nullptr);

	//Sleep((1000 / 60) );
}

VOID C_OverlayCore::OnProgramEnd()
{
	if (G::DirectDevice9) {
		G::DirectDevice9->Clear(NULL, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 0.0f, NULL);
		G::DirectDevice9->Release();
	}
	
	G::P::Unload = TRUE;
	
	if(G::Direct3d9)
		G::Direct3d9->Release();

	{
		DWORD dwTargetWindowPid = NULL;

		if (!GetWindowThreadProcessId(G::WindowHandle, &dwTargetWindowPid))
			return;

		auto* const hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwTargetWindowPid);

		if (hProcess == nullptr)
			return;

		TerminateProcess(hProcess, EXIT_SUCCESS);
		CloseHandle(hProcess);
	}

	Sleep(1500);
}
