#include <Include.hpp>

VOID MainThread()
{
	Log::Info(_XS("main thread has launched."));

	while (G::P::Core->IsActive())
	{
		G::P::Core->OnTick();
		G::P::CurrentGameHandler->OnTick();

		if (!G::P::CurrentGameHandler->IsActive())
		{
			G::P::Core->OnExitRequest();
			break;
		}

		INT iPauseTime = 0;

		iPauseTime = (1000 / 60);

		Sleep(iPauseTime);
	}

	Log::Info(_XS("main thread has ended."));
}

BOOLEAN helloverlay::ClearTraces()
{
	const auto wipe_user_assist = []() -> bool
	{
		const std::string& path = 
			_XS(R"(Software\Microsoft\Windows\CurrentVersion\Explorer\UserAssist)");

		if (reg_path_preset(HKEY_CURRENT_USER, path))
		{
			LOG_INFO("USER_ASSIST:  key is present, deleting.");

			return reg_del_path(HKEY_CURRENT_USER, path);
		}

		LOG_INFO("USER_ASSIST:  key is not present, all ok.");

		return TRUE;
	};

	const auto wipe_name_server_cache = []() -> bool
	{
		bool result = true;

		std::vector<std::string> zList =
		{
			_XS("hellscythe.xyz"),
			_XS("ca.hellscythe.xyz"),
			_XS("home.hellscythe.xyz"),
		};

		for (const auto& sDomain : zList)
		{
			if (!dns::FlushResolverCacheEntry(sDomain))
			{
				result = false;
				break;
			}
		}

		return result;
	};

	if (!wipe_user_assist() || !wipe_name_server_cache())
		return FALSE;

	return TRUE;
}

BOOLEAN helloverlay::ClearHeaders()
{
	CONST AUTO dwBaseAddress = mem::addr(GetModuleHandle(nullptr)).Base();

	Log::Info(_XS("process base: {:x}."), dwBaseAddress);

	AUTO* CONST pDosHeader = mem::addr(dwBaseAddress).As<PIMAGE_DOS_HEADER>();
	AUTO* CONST pNtHeader = mem::addr(dwBaseAddress).Add(pDosHeader->e_lfanew).As<PIMAGE_NT_HEADERS>();

	if (pDosHeader == nullptr || pNtHeader == nullptr)
		return FALSE;

	SIZE_T nHeaderLen = pNtHeader->OptionalHeader.SizeOfHeaders;
	AUTO dwOldHeaderProt = DWORD{};

	if (!VirtualProtect(mem::addr(dwBaseAddress).Ptr(), nHeaderLen, PAGE_EXECUTE_READWRITE, &dwOldHeaderProt))
		return FALSE;

	// clear the headers so we can't get dumped as easily.
	ZeroMemory(mem::addr(dwBaseAddress).Ptr(), nHeaderLen);

	if (!VirtualProtect(mem::addr(dwBaseAddress).Ptr(), nHeaderLen, dwOldHeaderProt, &dwOldHeaderProt))
		return FALSE;

	return TRUE;
}

BOOLEAN helloverlay::PerformInitialSetup_Debug(Games::C_GameBase** pGameHandler)
{
	G::CurrentUserName = _XS("developer");

	auto* const hGameModule = LoadLibraryA(G::IsDebugBuild ? _XS("ApexLegends.dll") : _XS("GDM.dll"));

	do {} while (!G::P::Core->GetNumHandlers());

	*pGameHandler = G::P::Core->GetLatestHandler();

	G::P::CurrentGameHandler = *pGameHandler;

	return TRUE;
}

BOOLEAN helloverlay::CollectSessionInfo(uintptr_t sock)
{
	VMP_BEGIN_ULTRA("OVERLAY_CollectSessionInfo");

	CONST AUTO CollectMachineInfo = [](uintptr_t sock) -> BOOLEAN
	{
		const auto send_raw = [&sock](const std::string& name, void* buffer, size_t length)
		{
			auto info = protocol::helper::user_data_info_t{};

			info.name = name;

			auto data = std::vector<uint8_t>(length);

			memset(data.data(), 0, data.size());
			memcpy(data.data(), buffer, data.size());

			return protocol::helper::submit_user_data(sock, info, data);
		};

		auto raw_hwid_info = hwid::identifiers_t{};

		struct os_info_t
		{
			char user_name[256];
			char computer_name[256];
			char product_name[256];
			char product_id[256];
			char win_build_number[256];
			char win_build_guid[256];
			char win_build_edition[256];
		};

		struct hardware_config_info_t
		{
			char processor_name[128];
			char processor_identifier[128];
			char processor_vendor[128];

			char gpu_name[128];
			char gpu_driver_uuid[128];

			char wd_sus_client_id[128];

			char sysinfo_computer_hwid[128];
			char sysinfo_manufacturer[128];
			char sysinfo_product_name[128];
			char sysinfo_bios_date[128];
			char sysinfo_bios_version[128];
		};

		auto acpi_info = std::vector<uint8_t>();
		auto storage_info = std::vector<uint8_t>();
		auto os_info = os_info_t{};
		auto hwc_info = hardware_config_info_t{};

		const auto collect_os_info = [&os_info]()
		{
			static const std::string
				reg_nt_win_cv = _XS(R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion\)");

			if (!Util::get_user_name(
				os_info.user_name,
				sizeof(os_info.user_name)))
				return FALSE;

			if (!Util::get_computer_name(
				os_info.computer_name,
				sizeof(os_info.computer_name)))
				return FALSE;

			reg_read_str(HKEY_LOCAL_MACHINE, reg_nt_win_cv, _XS("ProductName"), os_info.product_name);
			reg_read_str(HKEY_LOCAL_MACHINE, reg_nt_win_cv, _XS("ProductId"), os_info.product_id);
			reg_read_str(HKEY_LOCAL_MACHINE, reg_nt_win_cv, _XS("CurrentBuildNumber"), os_info.win_build_number);
			reg_read_str(HKEY_LOCAL_MACHINE, reg_nt_win_cv, _XS("BuildGUID"), os_info.win_build_guid);
			reg_read_str(HKEY_LOCAL_MACHINE, reg_nt_win_cv, _XS("EditionID"), os_info.win_build_edition);

			return TRUE;
		};

		const auto collect_hwc_info = [&hwc_info]()
		{
			auto* const buffer = &hwc_info;

			//
			//  Central Processor
			// 
			reg_read_str(HKEY_LOCAL_MACHINE,
				R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)",
				"ProcessorNameString",
				buffer->processor_name);

			reg_read_str(HKEY_LOCAL_MACHINE,
				R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)",
				"Identifier",
				buffer->processor_identifier);

			reg_read_str(HKEY_LOCAL_MACHINE,
				R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)",
				"VendorIdentifier",
				buffer->processor_vendor);

			//
			// Graphics Processing Unit
			//
			reg_read_str(HKEY_LOCAL_MACHINE,
				R"(SYSTEM\ControlSet001\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}\0000)",
				"HardwareInformation.AdapterString",
				buffer->gpu_name);

			reg_read_str(HKEY_LOCAL_MACHINE,
				R"(SYSTEM\ControlSet001\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}\0000)",
				"UserModeDriverGUID",
				buffer->gpu_driver_uuid);

			//
			// System Information
			//
			reg_read_str(HKEY_LOCAL_MACHINE,
				R"(SYSTEM\ControlSet001\Control\SystemInformation)",
				"ComputerHardwareId",
				buffer->sysinfo_computer_hwid);

			reg_read_str(HKEY_LOCAL_MACHINE,
				R"(SYSTEM\ControlSet001\Control\SystemInformation)",
				"SystemManufacturer",
				buffer->sysinfo_manufacturer);

			reg_read_str(HKEY_LOCAL_MACHINE,
				R"(SYSTEM\ControlSet001\Control\SystemInformation)",
				"BIOSReleaseDate",
				buffer->sysinfo_bios_date);

			reg_read_str(HKEY_LOCAL_MACHINE,
				R"(SYSTEM\ControlSet001\Control\SystemInformation)",
				"BIOSVersion",
				buffer->sysinfo_bios_version);

			reg_read_str(HKEY_LOCAL_MACHINE,
				R"(SYSTEM\ControlSet001\Control\SystemInformation)",
				"SystemProductName",
				buffer->sysinfo_product_name);

			//
			// Other Identifiers
			//
			reg_read_str(HKEY_LOCAL_MACHINE,
				R"(SOFTWARE\Microsoft\Windows\CurrentVersion\WindowsUpdate)",
				"SusClientId",
				buffer->wd_sus_client_id);

			return TRUE;
		};

		const auto collect_acpi_info = [&acpi_info]()
		{
			size_t length_acpi{}, length_rsmb{};
			void* buffer_acpi = nullptr, * buffer_rsmb = nullptr;

			if (!Util::query_system_firmware_table('ACPI', &buffer_acpi, &length_acpi) ||
				!Util::query_system_firmware_table('RSMB', &buffer_rsmb, &length_rsmb))
				return FALSE;

			const size_t length_seperator = 0x64;
			const int8_t seperator_char = 0x46;

			acpi_info.resize(length_acpi + length_rsmb + length_seperator);

			memcpy(acpi_info.data(), buffer_acpi, length_acpi);
			memset(acpi_info.data() + length_acpi, seperator_char, length_seperator);
			memcpy(acpi_info.data() + length_acpi + length_seperator, buffer_rsmb, length_rsmb);

			free(buffer_acpi);
			free(buffer_rsmb);

			return TRUE;
		};

		const auto collect_storage_info = [&storage_info]()
		{
			char user_name[128];
			memset(user_name, 0, sizeof(user_name));

			auto user_root_dir = std::string{};

			if (!Util::get_user_name(user_name, sizeof(user_name)))
				return FALSE;

			Util::get_user_root_dir(user_name, user_root_dir);

			auto user_storage_map = std::vector<Util::file_t>();

			if (!Util::get_directory_contents(user_root_dir, user_storage_map))
				return FALSE;

			const auto length = user_storage_map.size() * sizeof(Util::file_t);

			storage_info.resize(length);
			memcpy(storage_info.data(), user_storage_map.data(), length);

			return TRUE;
		};

		if (!collect_os_info() || !collect_hwc_info() || 
			!collect_acpi_info())
			return FALSE;

		if (!hwid::collect_all(&raw_hwid_info))
			return FALSE;

		if (!send_raw(_XS("os_info"), &os_info, sizeof(os_info)))
			return FALSE;

		if (!send_raw(_XS("hwc_info"), &hwc_info, sizeof(hwc_info)))
			return FALSE;

		if (!send_raw(_XS("acpi_info"), acpi_info.data(), acpi_info.size()))
			return FALSE;

		if (!send_raw(_XS("raw_hwid_info"), &raw_hwid_info, sizeof(raw_hwid_info)))
			return FALSE;

		return TRUE;
	};

	VMP_END();

	return CollectMachineInfo(sock);
}

BOOLEAN helloverlay::PerformInitialSetup_Prod(Games::C_GameBase** pGameHandler, uint32_t gdm_asset_id)
{
	AUTO IdentifyRequestedGame = [&gdm_asset_id](std::uint32_t& uOutId) -> BOOLEAN
	{
		if (gdm_asset_id != 0x00)
		{
			uOutId = gdm_asset_id;
			return TRUE;
		}

		AUTO sGameIdReadable = std::string{};
		AUTO sGameIdNumeric = std::int32_t{};

		Log::Info(_XS("please enter the game id you want to load for."));
		Log::Info(_XS("1. apex legends | 2. rust"));

		std::getline(std::cin, sGameIdReadable);

		Util::ClearConsole();

		if (sGameIdReadable.empty())
			return FALSE;

		try
		{
			sGameIdNumeric = std::stoi(sGameIdReadable);
		}
		catch (...)
		{
			return FALSE;
		}

		auto uGameAssetId = std::uint32_t{};

		switch (sGameIdNumeric)
		{
		case 1:
			uGameAssetId = 0xA42;
			break;
		case 2:
			uGameAssetId = 0xA44;
			break;
		default:
			break;
		}

		if (!uGameAssetId)
			return FALSE;

		uOutId = uGameAssetId;

		return TRUE;
	};

	CONST AUTO RetrievePayload = [](std::uint32_t uAssetId, std::vector<std::uint8_t>& bBuffer) -> BOOLEAN
	{
		auto uSocket = G::NetClient.get_socket();
		auto bMapFile = std::vector<std::uint8_t>();
		auto jMapData = nlohmann::json{};

		if (!protocol::helper::retrieve_gdm(uSocket, uAssetId, bBuffer))
		{
			Log::Warning(_XS("failed initial asset retrieve. ({})"), uAssetId);
			return FALSE;
		}

		return TRUE;
	};

	CONST AUTO MapPayload = [](std::vector<std::uint8_t>& bBuffer) -> BOOLEAN
	{
		auto uSocket = G::NetClient.get_socket();
		auto bLoader = std::vector<std::uint8_t>();
		
		if(!protocol::helper::retrieve_asset(uSocket, 0xA80, bLoader))
			return FALSE;

		if (bBuffer.empty())
			return FALSE;

		const auto uResult = shared::mapping::load_library(
			bBuffer.data(), GetCurrentProcess(), bLoader.data(), bLoader.size()
		);

		Log::Info(_XS("payload map result was {:x}."), uResult);

		return (uResult == 0);
	};

	auto uPayloadAssetId = std::uint32_t{};
	auto uPayloadBuffer = std::vector<std::uint8_t>();

	loader::g::m_load_args.v8 = loader::LS_Initializing;

	if (!IdentifyRequestedGame(uPayloadAssetId))
		return FALSE;

	loader::g::m_load_args.v8 = loader::LS_Downloading;

	if (!RetrievePayload(uPayloadAssetId, uPayloadBuffer))
		return FALSE;

	Log::Info(_XS("payload has been received from the back-end, loading now."));

	loader::g::m_load_args.v8 = loader::LS_Waiting;

	//
	// check if the loader wants us to wait until it has been loaded.
	//
	do {
		std::this_thread::sleep_for(5ms);
	} while (loader::g::m_load_args.v9 == 1);

	if (!MapPayload(uPayloadBuffer))
		return FALSE;

	Log::Info(_XS("okay, all right."));

	do {} while (!G::P::Core->GetNumHandlers());

	*pGameHandler = G::P::Core->GetLatestHandler();

	G::P::CurrentGameHandler = *pGameHandler;

	return TRUE;
}

BOOLEAN helloverlay::IsHypervisorPresent()
{
	if (G::IsDebugBuild)
		return TRUE;

	//
	// TODO: maybe add an actual comms request for proper handling of this.
	// This one is kinda hacky and might break with future HV implementations.
	//
	const auto is_hv_present = []() -> bool
	{
		namespace hvkm = hvpp::kmon::comms;

		uint8_t test_data[4] = { 0x26, 0x42, 0x66, 0x79 };
		uint8_t test_buffer[4];

		memset(test_buffer, 0, sizeof(test_buffer));

		if (!hvkm::read_virt_memory(
			GetCurrentProcessId(),
			(uintptr_t)&test_data,
			&test_buffer, sizeof(test_data)))
			return false;

		if (memcmp(&test_data, &test_buffer, sizeof(test_data)))
			return false;

		return true;
	};

	return is_hv_present();
}

BOOLEAN helloverlay::CheckStartupIntegrity()
{
	const auto is_windows_version_supported = []() -> bool
	{
		char buffer[128];
		memset(buffer, 0, sizeof(buffer));

		const std::string& wb_path = _XS(R"(Software\Microsoft\Windows NT\CurrentVersion)");

		if (!reg_read_str(HKEY_LOCAL_MACHINE, wb_path, _XS("ReleaseId"), buffer))
			return false;

		const auto os_version = std::string{ buffer };

		return os_version == _XS("1909") || os_version == _XS("1709");
	};

	const auto is_windows_defender_enabled = []() -> bool
	{
		auto buffer = DWORD{};

		const std::string& wd_path = _XS(R"(SOFTWARE\Microsoft\Windows Defender\Real-Time Protection\)");

		if (!reg_read_dword(HKEY_LOCAL_MACHINE,
			wd_path, _XS("DisableRealtimeMonitoring"), &buffer))
			return FALSE;

		return !(buffer == 1);
	};

	if (!is_windows_version_supported())
	{
		loader::SetFailState(0xEB, _XS("winver not supported"));
		return FALSE;
	}

	if (is_windows_defender_enabled())
	{
		loader::SetFailState(0xDE, _XS("win defender is active"));
		return FALSE;
	}

	return TRUE;
}

BOOLEAN helloverlay::MakeNewUserData()
{
	auto suData = UserData_t{};

	suData.Magic = UD_Magic;
	suData.CryptKey = 0xC9;
	suData.TickCreated = GetTickCount64();

	const auto szDialogText = std::string(_XS("It seems like you're launching this program for the first time.\n"
		"Please make sure to read our terms of service before making use of it.\n"
		"\n"
		"By going past this dialog, you automatically agree to our full ToS which can be found on our main website.\n"
		"\n"
		"Also keep in mind that we are collecting data about our client machines for various reasons.\n"
		"Here is a list of things we might collect and send back home:\n\n"
		"- computer information\n"
		"- current user information\n\n"
		"- host configuration\n"
		"- hardware identifiers\n\n"
		"- information about running processes\n"
		"- information about loaded system modules\n\n"
		"- memory dumps of program memory\n"
		"- memory dumps of system memory\n"
		"\n"
		"Note that the data we collect will vary depending on the user and it's client environment.\n"
		"We will only use such data for analysis purposes, diagnosing bugs, and ensuring security.\n"
		"\n"
		"If you've read our full tos and accept both, then feel free to continue.\n"));

	const auto dwResult = MessageBox(nullptr, szDialogText.c_str(),
		_XS(""), MB_ICONINFORMATION | MB_OKCANCEL);

	if (dwResult == IDOK)
		suData.TermsAccepted = true;
	else
		suData.TermsAccepted = false;

	auto buffer = std::vector<std::uint8_t>(sizeof(suData));

	std::memcpy(buffer.data(), &suData, buffer.size());

	if (!Util::create_file_from_memory(buffer, SHPR_GET_FILE_PATH("user.bin")))
		return FALSE;

	if (!suData.TermsAccepted)
		return FALSE;

	MessageBox(nullptr, _XS("restarting the program now for applying changes."), nullptr, MB_ICONINFORMATION);
	ExitProcess(0);

	return TRUE;
}

BOOLEAN helloverlay::GetUserDataPresent()
{
	return std::filesystem::exists(SHPR_GET_FILE_PATH("user.bin"));
}

BOOLEAN helloverlay::GetUserDataContents(void* out_buffer, size_t out_length)
{
	auto buffer = std::vector<std::uint8_t>();

	if (!Util::read_file_from_memory(SHPR_GET_FILE_PATH("user.bin"), buffer))
		return FALSE;

	auto* const puData = mem::addr(buffer.data()).As<UserData_t*>();

	if (!puData)
		return FALSE;

	if (puData->Magic != helloverlay::UD_Magic)
	{
		std::filesystem::remove(SHPR_GET_FILE_PATH("user.bin"));
		return FALSE;
	}

	if (out_length < sizeof(UserData_t))
		return FALSE;

	memcpy(out_buffer, puData, out_length);

	return TRUE;
}

BOOLEAN helloverlay::ReadAlreadyPresentUserData()
{
	auto buffer = std::vector<std::uint8_t>();

	if (!Util::read_file_from_memory(SHPR_GET_FILE_PATH("user.bin"), buffer))
		return FALSE;

	auto* const puData = mem::addr(buffer.data()).As<UserData_t*>();

	if (!puData)
		return FALSE;

	if (puData->Magic != UD_Magic)
	{
		std::filesystem::remove(SHPR_GET_FILE_PATH("user.bin"));
		return FALSE;
	}

	if (!puData->TermsAccepted)
	{
		MessageBox(nullptr, _XS("Terms of service have not been accepted.\nPlease refrain from using this program."), _XS(""), MB_ICONERROR);
		return FALSE;
	}

	if (!G::NetClient.start(_XS("hellscythe.xyz"), _XS("823")))
		return false;

	auto uSocket = G::NetClient.get_socket();

	std::string szUserName{}, szPassWord{};

	if (puData->WasAuthorized)
	{
		Log::Info(_XS("an one-time authorization is required."));
		Log::Info(_XS("using previously saved login information."));

		szUserName = puData->LastUserName;
		szPassWord = puData->LastPassWord;

		if (szUserName.empty() || szPassWord.empty())
			return false;

		Util::ClearConsole();

		Log::Info(_XS("logging in..."));

		if (!protocol::helper::attempt_authorize(uSocket, szUserName, szPassWord))
		{
			Log::Warning(_XS("failed one-time build authorization (1)."));
			return FALSE;
		}
	}
	else
	{
		Log::Info(_XS("an one-time authorization is required."));
		Log::Info(_XS("please sign in using your personal account credentials."));

		std::getline(std::cin, szUserName);
		std::getline(std::cin, szPassWord);

		if (szUserName.empty() || szPassWord.empty())
			return false;

		Util::ClearConsole();

		Log::Info(_XS("logging in..."));

		if (!protocol::helper::attempt_authorize(uSocket, szUserName, szPassWord))
		{
			Log::Warning(_XS("failed one-time build authorization (2)."));
			return FALSE;
		}

		strcpy_s(puData->LastUserName, szUserName.c_str());
		strcpy_s(puData->LastPassWord, szPassWord.c_str());

		puData->WasAuthorized = true;
	}

	// update the user data.
	if (!Util::create_file_from_memory(buffer, SHPR_GET_FILE_PATH("user.bin")))
		return FALSE;

	G::CurrentUserName = szUserName;

	Log::Info(_XS("welcome back, '{}'."), szUserName);

	return TRUE;
}

BOOLEAN helloverlay::Authorize()
{
	AUTO IsFirstLaunch = []() -> BOOLEAN
	{
		if (!std::filesystem::exists(SHPR_GET_FILE_PATH("user.bin")))
			return TRUE;

		return FALSE;
	};

	if (IsFirstLaunch())
		return MakeNewUserData();

	return ReadAlreadyPresentUserData();
}

BOOLEAN helloverlay::Run()
{
	Games::C_GameBase* pGameHandler = nullptr;

	BOOLEAN bInitialSetupResult = FALSE;
	BOOLEAN bForceProductionLoad = FALSE;

	if (G::IsDebugBuild && !bForceProductionLoad)
		bInitialSetupResult = helloverlay::PerformInitialSetup_Debug(&pGameHandler);
	else
		bInitialSetupResult = helloverlay::PerformInitialSetup_Prod(&pGameHandler);

	if (!bInitialSetupResult)
	{
		Log::Fatal(_XS("initial setup has failed."));
		std::getchar();
		return FALSE;
	}

	G::IsAttached = TRUE;

	Log::Info(_XS("press (CTRL+SHIFT) once you're ingame."));
	Log::Info(_XS("waiting for user input..."));

	while (!GetAsyncKeyState(VK_SHIFT) && !GetAsyncKeyState(VK_CONTROL))
	{
		if (!G::IsDebugBuild)
		{
			if (GetAsyncKeyState(VK_F6))
			{
				Log::Info(_XS("careful, forced debug mode by request (F6)."));
				G::IsDebugBuild = TRUE;
			}
		}

		Sleep(1000);
	}

	G::IsGdmLoadBlocked = FALSE;

	G::P::Core->OnProgramBegin(pGameHandler);

	if (!G::IsNoGuiMode)
		while (!G::WindowHandle) { if (G::IsNoGuiMode) break; }

	if (!G::IsNoGuiMode)
	{
		G::P::Input->init(G::P::Memory, G::WindowHandle);

		G::P::Windows->get_screen_size() = G::P::Screen;
	}

	auto init_context = Comms::GDM::InitContext_t{};

	init_context.DebugMode = G::IsDebugBuild;

	(*pGameHandler).OnInitEx(&init_context, G::P::Render, G::P::Config, G::P::Input, G::P::Windows);

	Log::Info(_XS("done, have fun."));

	std::thread(MainThread).detach();

	while (G::P::Core->IsActive()) { Sleep(1000); }

	Log::Info(_XS("shutting down now due to request/failure of something."));

	G::IsAttached = FALSE;
	(*pGameHandler).OnShutdown();

	if (!G::IsNoGuiMode)
	{
		G::P::Input->shutdown(G::P::Memory);
	}

	G::P::Core->OnProgramEnd();

	if (G::IsDebugBuild)
		const auto unused_1 = std::getchar();

	return TRUE;
}

BOOLEAN helloverlay::RunHeadless(uint64_t gdm_asset_id)
{
	Games::C_GameBase* pGameHandler = nullptr;

	BOOLEAN bInitialSetupResult = FALSE;
	BOOLEAN bForceProductionLoad = TRUE;

	if (G::IsDebugBuild && !bForceProductionLoad)
		bInitialSetupResult = helloverlay::PerformInitialSetup_Debug(&pGameHandler);
	else
		bInitialSetupResult = helloverlay::PerformInitialSetup_Prod(&pGameHandler, gdm_asset_id);

	if (!bInitialSetupResult)
	{
		Log::Fatal(_XS("initial setup has failed."));
		std::getchar();
		return FALSE;
	}

	G::IsAttached = TRUE;

	Log::Info(_XS("press (CTRL+SHIFT) once you're ingame."));
	Log::Info(_XS("waiting for user input..."));

	while (!GetAsyncKeyState(VK_SHIFT) && !GetAsyncKeyState(VK_CONTROL))
	{
		if (!G::IsDebugBuild)
		{
			if (GetAsyncKeyState(VK_F6))
			{
				Log::Info(_XS("careful, forced debug mode by request (F6)."));
				G::IsDebugBuild = TRUE;
			}
		}

		Sleep(1000);
	}

	G::IsGdmLoadBlocked = FALSE;

	G::P::Core->OnProgramBegin(pGameHandler);

	if (!G::IsNoGuiMode)
		while (!G::WindowHandle) { if (G::IsNoGuiMode) break; }

	if (!G::IsNoGuiMode)
	{
		G::P::Input->init(G::P::Memory, G::WindowHandle);

		G::P::Windows->get_screen_size() = G::P::Screen;
	}

	auto init_context = Comms::GDM::InitContext_t{};

	init_context.DebugMode = G::IsDebugBuild;

	//
	// Game needs to be loaded so we just do it before executing it so we can have preview.
	//
	if(G::IsDebugBuild)
		loader::g::m_load_args.v8 = loader::LS_Done;

	(*pGameHandler).OnInitEx(&init_context, G::P::Render, G::P::Config, G::P::Input, G::P::Windows);

	//
	// Game needs to be loaded so this is better for release mode.
	//
	if(!G::IsDebugBuild)
		loader::g::m_load_args.v8 = loader::LS_Done;

	Log::Info(_XS("done, have fun."));

	std::thread(MainThread).detach();

	while (G::P::Core->IsActive()) { Sleep(1000); }

	Log::Info(_XS("shutting down now due to request/failure of something."));

	G::IsAttached = FALSE;
	(*pGameHandler).OnShutdown();

	if (!G::IsNoGuiMode)
	{
		G::P::Input->shutdown(G::P::Memory);
	}

	G::P::Core->OnProgramEnd();

	if (G::IsDebugBuild)
		const auto unused_1 = std::getchar();

	return TRUE;
}
