#include <Include.hpp>

VOID TimerThread()
{
	while(G::IsAttached)
	{
		G::CurrentTime = static_cast<FLOAT>(GetTickCount64()) * 0.001f;

		Sleep(100);
	}
}

VOID WindowProcThread()
{
	while(G::IsAttached)
	{
		MSG mMsg{};
		
		while (G::IsAttached && PeekMessage(&mMsg, nullptr, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&mMsg);
			DispatchMessage(&mMsg);
		}

		Sleep(1);
	}
}

BOOLEAN SetupSingleInstanceLocks()
{
	std::string name{};

	char name_buffer[128];
	memset(name_buffer, 0, sizeof(name_buffer));

	if(!Util::get_user_name(name_buffer, sizeof(name_buffer)))
		return FALSE;

	name = std::string{ name_buffer };

	if (name.empty())
		return FALSE;

	try
	{
		auto h_mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, name.c_str());

		if (h_mutex == nullptr)
		{
			//
			// Create One.
			//
			h_mutex = CreateMutex(nullptr, FALSE, name.c_str());
		}
		else
		{
			//
			// Block execution because we already have an instance.
			//
			return FALSE;
		}
	}
	catch (...)
	{
		return FALSE;
	}

	return TRUE;
}

INT WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInst, _In_ LPSTR pCmdLine, _In_ INT nCmdShow)
{
	const auto handle_file_on_disk = []() -> bool
	{
		std::string my_path{};

		if (Util::GetSelfPath(my_path))
		{
			const auto my_name = std::filesystem::path(my_path).filename().string();

			if (strstr(my_name.c_str(), _XS("HSCL")))
			{
				const auto new_path = fmt::format(_XS("{}.exe"), util::get_random_string(12));
				const auto result = rename(my_path.c_str(), new_path.c_str());

				return !(result == ERROR_SUCCESS);
			}

			return true;
		}

		return true;
	};

#if defined(_DEBUG)
	G::IsDebugBuild = TRUE;
#else
	G::IsDebugBuild = FALSE;
#endif

	if (!SetupSingleInstanceLocks())
	{
		MESSAGE_BOX("you already have an instance of HS running.\n"
			"please close it immediately and then try again.\n\n"
			"if you don't see the program on your screen then close it through task manager.\n"
			"thanks for your understanding.", MB_ICONWARNING);
		return EXIT_FAILURE;
	}

	if (!G::IsDebugBuild && !handle_file_on_disk())
	{
		MessageBox(nullptr, _XS("program has been modified, please restart the program (1)."), nullptr, MB_ICONERROR);
		return EXIT_FAILURE;
	}
	
	if (!G::StorageHpr.initialize(_XS("HsHBE")))
		return EXIT_FAILURE;

	exports::InfoVarsInit();
	G::ExportInit();

	G::Inst = hInst;

	if (!Util::CreateConsole())
		return EXIT_FAILURE;

	if (!hooks::initialize())
	{
		MESSAGE_BOX("unexpected error (1).\n",
			MB_ICONERROR);
		return EXIT_FAILURE;
	}

	if (!helloverlay::ClearTraces())
	{
		MessageBox(nullptr, _XS("failed to clear traces."), nullptr, MB_ICONERROR);
		return EXIT_FAILURE;
	}

	auto hConsoleWindow = FIND_EXPORT("kernel32.dll", GetConsoleWindow)();

	FIND_EXPORT("user32.dll", SetWindowTextA)(hConsoleWindow, _XS(""));

	Log::Info(_XS("hellscythe educational software dated '{} / {}'"), _XS(__DATE__), _XS(__TIME__));
	Log::Info(_XS("starting program."));

	SMART_PTR_ALLOC(G::P::Memory);
	SMART_PTR_ALLOC(G::P::Core);
	SMART_PTR_ALLOC(G::P::Render);
	SMART_PTR_ALLOC(G::P::Input);
	SMART_PTR_ALLOC(G::P::Config);
	SMART_PTR_ALLOC(G::P::Windows);

	G::P::Config->initialize(G::P::Memory);

	std::thread(WindowProcThread).detach();
	std::thread(TimerThread).detach();

	constexpr BOOLEAN bForceNewUI = FALSE;

	BOOLEAN bUseNewUI = !G::IsDebugBuild || bForceNewUI;

	if (bUseNewUI)
	{
		loader::PreAuth();
		loader::Initialize(G::NetClient, hInst);

		do
		{
			if (integrity::_::severe_violation_present)
			{
				loader::g::running = false;
				loader::g::rendering = false;

				MESSAGE_BOX("security violation has been detected.", MB_ICONERROR);
				ExitProcess(EXIT_FAILURE);
			}

			if (networking::Lost())
			{
				loader::g::running = false;
				loader::g::rendering = false;

				MESSAGE_BOX("connection to hellscythe servers was lost.", MB_ICONERROR);
				ExitProcess(EXIT_FAILURE);
			}

			std::this_thread::sleep_for(60s);
		} while (loader::g::running);

		loader::Shutdown();
	}
	else
	{
		if (!helloverlay::IsHypervisorPresent() && G::IsDebugBuild)
		{
			MessageBox(nullptr, _XS("you don't have HSK running on your "
				"system and can't load it from the debug loader.\n"
				"please load it and then start debugging, thanks."),
				nullptr, MB_ICONERROR);

			return EXIT_SUCCESS;
		}

		if (helloverlay::IsHypervisorPresent())
		{
			LOG_INFO("HSK was found and has been validated, you can now load external GDMs.");

			if (!helloverlay::Run())
			{
				Log::Error(_XS("helloverlay->run failed!"));

				return std::getchar();
			}
		}
		else
		{
			LOG_WARN("HSK could not be found and needs to be loaded, please do that right now.");

			if (!hxldr::run(hConsoleWindow, G::NetClient.get_socket()))
			{
				Log::Error(_XS("helloverlay->run failed!"));

				return std::getchar();
			}

			std::getchar();
		}
	}

	return EXIT_SUCCESS;
}

extern "C"
{
	VOID MainThread(HINSTANCE hinstDLL)
	{
		WinMain(hinstDLL, nullptr, nullptr, SW_SHOWDEFAULT);

		do {
			std::this_thread::sleep_for(10s);
		} while (true);
	}

	BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
	{
		switch (fdwReason)
		{
		case DLL_PROCESS_ATTACH:
			std::thread(MainThread, hinstDLL).detach();
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
		}

		return TRUE;
	}
}
