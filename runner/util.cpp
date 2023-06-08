#include <include.hpp>

bool util::is_debug_build()
{
#if defined(_DEBUG)
	return true;
#else 
	return false;
#endif
}

bool util::create_memory_from_file(const std::string& path, std::vector<uint8_t>& buffer)
{
	if (path.empty() || !std::filesystem::exists(path))
		return false;

	auto length = std::filesystem::file_size(path);
	auto stream = std::ifstream(path, std::ios::binary);

	if (!length || !stream.is_open())
		return false;

	buffer.resize(length);

	memset(buffer.data(), 0, buffer.size());

	stream.read(reinterpret_cast<char*>(buffer.data()), length);
	stream.close();

	return true;
}

bool util::create_process(const std::string& path, uint32_t flags, uint32_t& pid)
{
	auto proc_info = PROCESS_INFORMATION{};
	auto start_info = STARTUPINFO{};

	if (!CreateProcess(path.c_str(), nullptr, nullptr,
		nullptr, FALSE, flags, nullptr,
		nullptr, &start_info, &proc_info))
		return false;

	pid = proc_info.dwProcessId;

	CloseHandle(proc_info.hThread);
	CloseHandle(proc_info.hProcess);

	return true;
}

bool util::create_process_elevated(const std::string& path, uint32_t& pid, std::vector<uint8_t>& backup_exit_process)
{
	auto she_info = SHELLEXECUTEINFO{};

	she_info.cbSize = sizeof(she_info);
	she_info.lpVerb = _XS("runas");
	she_info.lpFile = path.c_str();
	she_info.nShow = SW_SHOWDEFAULT;
	she_info.fMask |= SEE_MASK_NOCLOSEPROCESS;

	const auto she_result = ShellExecuteEx(&she_info);

	if (!she_result || she_info.hProcess == nullptr)
		return false;

	auto process = c_process(GetProcessId(she_info.hProcess));

	auto waiting = true;

	LOG_INFO("I  waiting for all modules to be loaded.");

	do
	{
		auto host_modules = util::get_process_modules(she_info.hProcess);

		if (!host_modules.empty())
		{
			for (const auto& entry : host_modules)
			{
				if (entry.m_name == _XS("kernel32.dll"))
				{
					waiting = false;
					break;
				}
			}
		}
	} while (waiting);

	LOG_INFO("I  all modules are now present.");

	const auto suspend_result = shared::nt::suspend_process(she_info.hProcess);

	if (!NT_SUCCESS(suspend_result))
	{
		LOG_INFO("!  suspend_process has failed with error code {:x}.",
			suspend_result);

		TerminateProcess(she_info.hProcess, EXIT_FAILURE);
		return false;
	}

	uint8_t exit_process_patch[] =
	{
		0x48, 0xB8,
		0x56, 0x34, 0x12, 0xFF, 0x7F, 0x00, 0x00, 0x00,
		0xFF, 0xD0, 0x48, 0x89, 0xC1, 0x48, 0x31, 0xC0,
		0x48, 0xB8,
		0x56, 0x34, 0x12, 0xFF, 0x7F, 0x00, 0x00, 0x00,
		0xFF, 0xD0, 0xC3
	};

	const auto addr_get_current_thread = (uintptr_t)&GetCurrentThread;
	memcpy(&exit_process_patch[2], &addr_get_current_thread, sizeof(uintptr_t));

	const auto addr_suspend_thread = (uintptr_t)&SuspendThread;
	memcpy(&exit_process_patch[20], &addr_suspend_thread, sizeof(uintptr_t));

	backup_exit_process.resize(sizeof(exit_process_patch));

	if (!process.read_data(
		backup_exit_process.data(), (uintptr_t)&ExitProcess,
		backup_exit_process.size()))
	{
		LOG_INFO("!  backup_exit_process has failed with error code {}.", GetLastError());
		return false;
	}

	if (!process.patch(
		(uintptr_t)&ExitProcess, exit_process_patch,
		sizeof(exit_process_patch)))
	{
		LOG_INFO("!  patch_exit_process has failed with error code {}.", GetLastError());
		return false;
	}

	Sleep(2000);

	const auto resume_result = shared::nt::resume_process(she_info.hProcess);

	if (!NT_SUCCESS(resume_result))
	{
		LOG_INFO("!  resume_process has failed with error code {:x}.",
			resume_result);

		TerminateProcess(she_info.hProcess, EXIT_FAILURE);
		return false;
	}

	pid = GetProcessId(she_info.hProcess);

	CloseHandle(she_info.hProcess);

	return true;
}

util::module_list util::get_process_modules(handle process)
{
	auto snapshot = handle{};

	auto result = module_list();
	auto entry = MODULEENTRY32{};

	auto process_id = GetProcessId(process);

	entry.dwSize = sizeof(entry);

	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process_id);

	if (!Module32First(snapshot, &entry))
	{
		CloseHandle(snapshot);
		return result;
	}

	do
	{
		const auto module_name = std::filesystem::path(entry.szExePath).filename().string();

		auto& info = result.emplace_back();

		info.m_name = text_to_lower(module_name);
		info.m_base = mem::addr(entry.modBaseAddr).Base();
		info.m_size = mem::addr(entry.modBaseSize).Base();

	} while (Module32Next(snapshot, &entry));

	CloseHandle(snapshot);

	return result;
}


