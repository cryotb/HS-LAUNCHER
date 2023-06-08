#include <Include.hpp>

void loader::logic::Think()
{

}

void loader::logic::OnStateChanged(States oldState, States newState)
{
	LOG_DEBUG("loader state has changed from {} to {}.", oldState, newState);

	switch (newState)
	{
	case States::Initializing:
		std::thread(Initialize).detach();
		break;
	case States::Authorizing:
		Authorize();
		break;
	case States::Loading:
		Load();
		break;
	default:
		LOG_DEBUG("warning: un-handled loader state {}.", newState);
		break;
	}
}

void loader::logic::Initialize()
{
	if(G::IsDebugBuild)
		Util::CreateConsole();

	if (!integrity::Initialize())
		return;

	do
	{
		std::this_thread::sleep_for(5s);
	} while (g::init_delta < 6000);

	if (!networking::Initialize())
	{
		SetFailState(0xBBEA, _XS("connection failure"), WSAGetLastError());
		return;
	}

	if (!helloverlay::CheckStartupIntegrity())
		return;

	g::initialized = true;
}

void loader::logic::Authorize()
{
	if(!G::IsDebugBuild)
		Sleep(6000);

	const auto uSocket = g::net_client.get_socket();

	//
	// only use hwid when the hypervisor has not been loaded yet.
	// this is because the hardware identifiers will likely be spoofed after.
	//
	const auto uHwid = helloverlay::IsHypervisorPresent() ? 0 : hwid::compute_hardware_id();

	if (!protocol::helper::attempt_authorize(uSocket, 
		g::auth_data.user_name, g::auth_data.pass_word, uHwid))
	{
		MESSAGE_BOX("authorization failure", MB_ICONERROR);
		ExitProcess( NULL );
	}

	//
	// At this point authorization has succeeded.
	//
	LOG_DEBUG("authorized successfully.");

	BOOLEAN bForceTelemetry = FALSE;

	if (!G::IsDebugBuild || bForceTelemetry)
	{
		if (!Shared::reports::submit(uSocket))
		{
			MESSAGE_BOX("internal error (67).", MB_ICONERROR);
			ExitProcess(NULL);
		}

		if (!helloverlay::CollectSessionInfo(uSocket))
		{
			MESSAGE_BOX("internal error (7).", MB_ICONERROR);
			ExitProcess(NULL);
		}
	}

	UpdateState(States::Selection);

	//
	// Update UserData.
	//
	auto suData = helloverlay::UserData_t{};

	suData.Magic = helloverlay::UD_Magic;
	suData.CryptKey = 0xC9;

	suData.TermsAccepted = TRUE;
	suData.WasAuthorized = TRUE;

	strcpy_s(suData.LastUserName, g::auth_data.user_name.c_str());
	strcpy_s(suData.LastPassWord, g::auth_data.pass_word.c_str());

	auto buffer = std::vector<std::uint8_t>(sizeof(suData));

	std::memcpy(buffer.data(), &suData, buffer.size());

	if (!Util::create_file_from_memory(buffer, SHPR_GET_FILE_PATH("user.bin")))
	{
		MESSAGE_BOX("failed to update user data, check permissions.", MB_ICONERROR);
		ExitProcess(NULL);
	}

	//
	// Verify Session
	//
	/*auto bMapFile = std::vector<std::uint8_t>();
	auto jMapData = nlohmann::json{};

	if (!protocol::helper::retrieve_asset(uSocket, 0xA62, bMapFile))
	{
		MESSAGE_BOX("failed initial asset retrieve.", MB_ICONERROR);
		return;
	}

	jMapData = nlohmann::json::parse(bMapFile, nullptr, false);

	if (jMapData.is_discarded())
	{
		Log::Warning(_XS("data corruption was revealed, please restart."));
		return;
	}

	const uint32_t my_hash = integrity::CalculateImageCodeHash(GetModuleHandle(nullptr));
	const uint32_t supposed_hash = jMapData[_XSS(".text")];

	if (!G::IsDebugBuild && my_hash != supposed_hash)
	{
		Log::Error(_XS("session could not be verified.\n"
			"please download a new copy.'"));

		loader::SetFailState(0x1, _XS("session verification failure"), my_hash, supposed_hash);

		Sleep(5000);

		ShellExecute(0, 0,
			_XS("https://ca.hellscythe.xyz"), 0, 0, SW_SHOW);

		return;
	}*/

	//
	// Download Product Definition Lists (PDLs)
	//
	const auto acquire_json_asset = [](uintptr_t sock, uint32_t id, nlohmann::json& out)
	{
		auto buffer = std::vector<uint8_t>();

		if (!protocol::helper::retrieve_asset(sock, 0x81B, buffer))
			return false;

		auto buffer_readable = reinterpret_cast<char*>(buffer.data());

		out = nlohmann::json::parse(buffer, nullptr, false);

		return !out.is_discarded();
	};

	if (!acquire_json_asset(uSocket, 0x81B, g::product_def_list[_XSS("game_dependant_modules")]))
	{
		MESSAGE_BOX("protocol error on parsing PDL GDM packet.", MB_ICONERROR);
		ExitProcess(NULL);
	}

	networking::_::ready = true;
}

void loader::logic::Load()
{
	const auto load_kernel_module = [](const uint64_t& id)
	{
		LOG_DEBUG("[load_km] id=  {:x}", id);

		if (hxldr::run_headless(g::net_client.get_socket(), id))
		{
			MESSAGE_BOX("YOU CAN NOW EXIT SAFELY.", MB_ICONINFORMATION);
			ExitProcess(0);
		}
		else
		{
			MESSAGE_BOX("FAILED TO LOAD KERNEL MODULE, IT'S NOT SAFE TO EXIT.", MB_ICONERROR);
			ExitProcess(0);
		}
	};

	const auto load_game_dependant_module = [](const uint64_t& id)
	{
		if (options::gdm_load.m_debug_console)
			Util::CreateConsole();

		LOG_DEBUG("[load_gdm] id=  {:x}", id);

		//
		// signalize that we want the overlay to block the loading process.
		// until the user has acknowledged the prompt, of course.
		//
		g::m_load_args.v9 = 1;

		std::thread(helloverlay::RunHeadless, id).detach();
	};

	const auto task_type = g::m_load_args.vcx;
	const auto module_id = g::m_load_args.vdx;

	switch (task_type)
	{
	case LTT_KernelModule:
		load_kernel_module(module_id);
		break;
	case LTT_GameDependantModule:
		load_game_dependant_module(module_id);
		break;
	default:
		LOG_DEBUG("warning: unknown load task type {}.", task_type);
		break;
	}
}
