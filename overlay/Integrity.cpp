#include <Include.hpp>

namespace integrity::config
{
	inline bool check_self = true;
	inline bool check_import_modules = false;
}

//
// Utility Functions
//
void integrity::ReportViolation(const violation_info_t& info)
{
	_::violation_queue.emplace_back( info );
}

bool integrity::HideThreadFromDebugger(HANDLE hThread)
{
	const auto result = nt::SetInfoThread(
		hThread,
		(THREADINFOCLASS)0x11,
		nullptr,
		NULL
	);

	return NT_SUCCESS(result);
}

uint32_t integrity::CalculateImageCodeHash(HMODULE hModule)
{
	_image_dos_header* dh = nullptr;
	_image_nt_header* nth = nullptr;
	_image_section_header th{};

	auto module_base = mem::addr(hModule).Base();

	if (!module_base)
		return 0;

	if (!Shared::PE::GetHeaders(module_base, &dh, &nth))
		return 0;

	if (!Shared::PE::FindSection(_XS(".text"), nth, &th))
		return 0;

	const auto buffer = c_memory_buffer(th.Misc.VirtualSize);

	if (!buffer.valid())
		return 0;

	if (!Shared::PE::DumpSection(_XS(".text"), module_base,
		nth, buffer.ptr(),
		buffer.length(), FALSE))
		return 0;

	return CRC::Calculate(buffer.ptr(), buffer.length(), CRC::CRC_32());
}

bool integrity::VerifyImageIntegrity(const std::string& module_name, const std::string& fod_path)
{
	_image_dos_header* dh = nullptr;
	_image_nt_header* nth = nullptr;
	_image_section_header th{};

	auto h_module = GetModuleHandle(module_name.c_str());
	auto module_base = mem::addr(h_module).Base();

	if (h_module == nullptr || !std::filesystem::exists(fod_path))
		return false;

	if (!Shared::PE::GetHeaders(module_base, &dh, &nth))
		return false;

	if (!Shared::PE::FindSection(_XS(".text"), nth, &th))
		return false;

	const auto td_buffer_fod = c_memory_buffer(th.Misc.VirtualSize);
	const auto td_buffer_fom = c_memory_buffer(th.Misc.VirtualSize);

	if (!td_buffer_fod.valid() || !td_buffer_fom.valid())
		return false;

	if (!Shared::PE::DumpSection(_XS(".text"), module_base,
		nth, td_buffer_fom.ptr(), td_buffer_fom.length(), FALSE))
		return FALSE;

	auto fod_raw_buffer = std::vector<uint8_t>();

	if (!Util::read_file_from_memory_ex(fod_path, 
		std::filesystem::file_size(fod_path),
		fod_raw_buffer))
		return false;

	auto* const fod_raw_ptr = fod_raw_buffer.data();
	const auto fod_raw_base = mem::addr(fod_raw_ptr).Base();

	if (fod_raw_ptr == nullptr)
		return false;

	_image_dos_header* fod_dh = nullptr;
	_image_nt_header* fod_nth = nullptr;

	if (!Shared::PE::GetHeaders(fod_raw_base, &fod_dh, &fod_nth))
		return false;

	if (!Shared::PE::DumpSection(_XS(".text"), fod_raw_base,
		fod_nth, td_buffer_fod.ptr(),
		td_buffer_fod.length(), TRUE))
		return FALSE;

	const auto fod_hash = CRC::Calculate(td_buffer_fod.ptr(), td_buffer_fod.length(), CRC::CRC_32());
	const auto fom_hash = CRC::Calculate(td_buffer_fom.ptr(), td_buffer_fom.length(), CRC::CRC_32());

	if (fod_hash != fom_hash)
		return false;

	return true;
}

//
// Checking Routines
//
bool integrity::CheckForDebugger()
{
	//
	// IDBP API
	//
	if (IsDebuggerPresent())
		return true;

	BOOL bRdbPresent = FALSE;

	//
	// RDBP API
	//
	if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &bRdbPresent))
	{
		if (bRdbPresent == TRUE)
			return true;
	}

	//
	//
	//


	return false;
}

bool integrity::CheckForPatches()
{
	if (config::check_import_modules)
	{
		const auto check_module = [](const std::string& name, bool self = false) -> bool
		{
			if (name.empty())
				return false;

			auto* const h_module = GetModuleHandle(self ? nullptr : name.c_str());

			if (h_module == nullptr)
				return false;

			char path[MAX_PATH];

			if (!GetModuleFileNameA(h_module, path, sizeof(path)))
				return false;

			LOG_DEBUG("module path =  {}", path);

			return VerifyImageIntegrity(name, path);
		};

		//
		// check important and commonly altered modules.
		//
		if (!check_module(_XS("ntdll.dll")) ||
			!check_module(_XS("kernel32.dll")) ||
			!check_module(_XS("user32.dll")) ||
			!check_module(_XS("mswsock.dll")))
			return true;
	}

	return false;
}

//
// Main Stuff
//
bool integrity::Initialize()
{
	VMP_BEGIN_ULTRA("Integrity_Initialize");

	constexpr bool bForceActive = false;

	//
	// we don't want this to run in debug mode.
	// unless we want to test stuff, of course.
	//
	if (G::IsDebugBuild && !bForceActive)
		return true;

	auto h_thread = 
		CreateThread(
			nullptr, NULL, 
			integrity::Think, 
			nullptr, NULL, nullptr
		);

	if (h_thread == nullptr)
		return false;

	if (!HideThreadFromDebugger(h_thread))
		return false;

	VMP_END();

	return true;
}

u_long integrity::Think(PVOID pArgs)
{
	VMP_BEGIN_ULTRA("Integrity_Think");

	do
	{
		std::this_thread::sleep_for(60ms);
	} while (!networking::_::ready);

	do
	{
		//
		// Debugger Check
		//
		if (CheckForDebugger())
		{
			auto info = violation_info_t{};
			info.m_severe = true;
			info.m_id = 0xB00;
			ReportViolation(info);
		}

		//
		// Tamper/Patching Check
		//
		if (CheckForPatches())
		{
			auto info = violation_info_t{};
			info.m_severe = true;
			info.m_id = 0xB02;
			ReportViolation(info);
		}

		//
		// Process the potential violation(s) here.
		// Remember that there can be external violations as well.
		//
		const auto sock = G::NetClient.get_socket();

		if (!sock || !G::NetClient.get_active())
			ExitProcess(EXIT_FAILURE);
		else
		{
			//
			// What we're doing here is first of all process and report all queued violations.
			// If in that queue there was a severe violation, we will terminate process AFTER completing.
			//
			if (_::violation_queue.empty() == false)
			{
				auto exit_after_processing = false;

				for (const auto& violation : _::violation_queue)
				{
					Shared::reports::submit_runtime_violation(sock, violation.m_id);

					if (violation.m_severe && !_::severe_violation_present)
						_::severe_violation_present = true;
				}
			}
		}

		std::this_thread::sleep_for(15s);
	} while (true);

	VMP_END();

	return NULL;
}
