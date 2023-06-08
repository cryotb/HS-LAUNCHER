#pragma once

namespace util
{
	struct module_t
	{
		std::string m_name{};

		std::uintptr_t m_base{};
		std::uintptr_t m_size{};
	};

	using module_list = std::vector<module_t>;


	extern bool is_debug_build();
	extern bool create_memory_from_file(const std::string& path, std::vector<uint8_t>& buffer);
	extern bool create_process(const std::string& path, uint32_t flags, uint32_t& pid);
	extern bool create_process_elevated(const std::string& path, uint32_t& pid, std::vector<uint8_t>& backup_exit_process);

	inline bool get_self_name(std::string& result)
	{
		char buffer[MAX_PATH];

		const auto h_self_module = GetModuleHandle(nullptr);

		const auto status = GetModuleFileName(h_self_module, buffer, sizeof(buffer));

		if (status)
			result = std::string{ buffer };

		return status;
	}

	inline void HexDump(void* ptr, size_t buflen) {
		unsigned char* buf = (unsigned char*)ptr;
		int i, j;
		for (i = 0; i < buflen; i += 16) {
			printf("%06x: ", i);
			for (j = 0; j < 16; j++)
				if (i + j < buflen)
					printf("%02x ", buf[i + j]);
				else
					printf("   ");
			printf(" ");
			for (j = 0; j < 16; j++)
				if (i + j < buflen)
					printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
			printf("\n");
		}
	}

	inline bool get_self_path(std::string& result)
	{
		char buffer[MAX_PATH];

		const auto h_self_module = GetModuleHandle(nullptr);

		const auto status = GetModuleFileName(h_self_module, buffer, sizeof(buffer));

		if (status)
		{
			result = std::string{ buffer };
			result = std::filesystem::path(result).remove_filename().string();
		}

		return status;
	}

	inline std::string text_to_lower(const std::string& input)
	{
		auto output = std::string(input);

		std::transform(output.begin(), output.end(), output.begin(),
			[](const unsigned char c) { return std::tolower(c); });

		return output;
	}

	inline std::string get_random_string(SIZE_T nLen = 6)
	{
		std::string str(_XS("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"));

		std::random_device rd;
		std::mt19937 generator(rd());

		std::ranges::shuffle(str, generator);

		return str.substr(0, nLen);
	}

	extern module_list get_process_modules(handle process);
}
