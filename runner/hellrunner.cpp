#include <include.hpp>

bool hellrunner::dispatch()
{
	if (g::is_debug_build)
	{
		if (std::filesystem::exists(_XS("debug.conf")))
		{
			auto debug_config = nlohmann::json{};
			auto debug_config_data = std::vector<uint8_t>();

			if (util::create_memory_from_file(_XS("debug.conf"), debug_config_data))
			{
				struct debug_config_t
				{
					bool m_force_production_load;
				} dbg_cfg;

				const auto load_debug_cfg = [&dbg_cfg, &debug_config_data, &debug_config]()
				{
					debug_config = nlohmann::json::parse(debug_config_data, nullptr, false);

					if (debug_config.is_discarded() == false)
					{
						try
						{
							dbg_cfg =
							{
								debug_config[_XSS("force_production_load")]
							};
						}
						catch (std::exception ex)
						{
							return false;
						}
					}
					else
						return false;

					return true;
				};

				if (load_debug_cfg())
				{
					//
					// a valid debug config has been found.
					// we can now make use of and parse it.
					//

					if (dbg_cfg.m_force_production_load)
					{
						return setup_prod();
					}
				}
			}
		}

		return setup_debug();
	}

	return setup_prod();
}

bool hellrunner::map_and_run_image(const std::vector<uint8_t>& image_buffer, bool self)
{
	handle host_handle{};
	uint32_t host_pid{};
	uint16_t result{};

	auto backup_exit_process = std::vector<uint8_t>();

	if (self)
	{
		//
		// Map the image into ourselves.
		//

		host_handle = GetCurrentProcess();
		host_pid = GetCurrentProcessId();
	}
	else
	{
		//
		// Map the image into a slave process.
		//

		if (!util::create_process_elevated(R"(C:\Windows\System32\rundll32.exe)",
			host_pid, backup_exit_process))
			return false;

		host_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, host_pid);

		if (host_handle == nullptr)
		{
			LOG_INFO("failed to create slave process.");
			return false;
		}
	}

	result = mapper::load_library((void*)image_buffer.data(),
		host_handle, mapper::loader_default, sizeof(mapper::loader_default));

	//
	// If it's a slave process, backup the exit process patch.
	//
	if (!self)
	{
		auto process = c_process(GetProcessId(host_handle));

		if (!process.patch(
			(uintptr_t)&ExitProcess, backup_exit_process.data(),
			backup_exit_process.size()))
		{
			LOG_INFO("failed to patch process.");

			return false;
		}
	}

	CloseHandle(host_handle);

	if (result != NULL)
	{
		LOG_INFO("mapper seems to have failed.");
		LOG_INFO("result:  {:x}", result);

		return false;
	}

	return true;
}

bool hellrunner::setup_debug()
{
	LOG_DEBUG("hellrunner:  setting up debug.");

	//
	// If we're in debug mode then just map the overlay into ourselves.
	// This way we won't cripple debugging abilities and many other things.
	//
	const std::string& overlay_path = FMT_FORMAT("{}{}", g::my_path, _XS("Helloverlay.dll"));

	return ( LoadLibrary(overlay_path.c_str()) != nullptr );
}

bool hellrunner::setup_prod()
{
	LOG_DEBUG("hellrunner:  setting up prod.");

	auto image_buffer = std::vector<uint8_t>{};
	auto client = net::c_client{};

	if (!client.start(_XS("hellscythe.xyz"), _XS("823")))
	{
		MESSAGE_BOX("failed to connect to HS.", MB_ICONERROR);
		return false;
	}

	if (!protocol::helper::retrieve_asset(client.get_socket(), 0xFA0, image_buffer))
	{
		MESSAGE_BOX("connection failure (1).", MB_ICONERROR);
		return false;
	}

	client.stop();

	if (!map_and_run_image(image_buffer, g::is_compat_mode))
	{
		MESSAGE_BOX("internal error (1).", MB_ICONERROR);
		return false;
	}

	return true;
}
