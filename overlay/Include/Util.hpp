#pragma once

class c_memory_buffer
{
public:
	c_memory_buffer(size_t length, bool zero = true)
	{
		m_length = length;
		m_data = malloc(length);

		if (m_data && m_length)
		{
			if (zero)
				memset(m_data, 0, m_length);
		}
	}

	~c_memory_buffer()
	{
		if (!(m_data == nullptr))
			free(m_data);
	}

	auto ptr() const { return m_data; }
	auto length() const { return m_length; }
	auto valid() const { return !(m_data == nullptr); }
private:
	size_t m_length;
	void* m_data;
};

namespace Util
{
	struct NamedModule_t
	{
		std::string m_name{};
		HMODULE m_handle{};
		MODULEINFO m_info{};
	};

	template < typename T >
	inline T* find_export(const std::string& mod, const std::string& name)
	{
		auto* h_mod = GetModuleHandle(mod.c_str());

		if (h_mod == nullptr)
			return nullptr;

		return mem::addr(GetProcAddress(h_mod, name.c_str())).As<T*>();
	}
	
	inline BOOL CreateConsole()
	{
		auto bResult = AllocConsole();

		// it was already present, however we've cleared it so it should be working as supposed.
		if (bResult == FALSE && !(GetConsoleWindow() == nullptr))
			bResult = TRUE;

		freopen_s(reinterpret_cast<FILE**>(stdin), _XS("CONIN$"), _XS("r"), stdin);
		freopen_s(reinterpret_cast<FILE**>(stdout), _XS("CONOUT$"), _XS("w"), stdout);

		return bResult;
	}

	inline VOID DeleteConsole()
	{
		std::fclose(stdin);
		std::fclose(stdout);
		std::fclose(stderr);

		FreeConsole();
	}

	inline VOID ClearConsole()
	{
		// may look ghetto, but it's in fact okay to do.
		system(_XS("cls"));
	}

	inline bool GetSelfPath(std::string& result)
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

	inline DWORD GetProcIdByName(const std::string& name)
	{
		PROCESSENTRY32 entry{};
		entry.dwSize = sizeof(PROCESSENTRY32);

		DWORD result = NULL;
		auto* const snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

		if (Process32First(snapshot_handle, &entry) == FALSE)
		{
			CloseHandle(snapshot_handle);
			return result;
		}

		while (Process32Next(snapshot_handle, &entry))
		{
			std::string entry_name(entry.szExeFile);

			std::transform(entry_name.begin(), entry_name.end(), entry_name.begin(), [](wchar_t character)
				{
					return std::tolower(character);
				});

			if (entry_name.empty() || entry_name[0] == 0)
			{
				continue;
			}

			//printf("%ws.\n", entry_name.c_str());

			if (entry_name == name)
			{
				result = entry.th32ProcessID;
				break;
			}
		}

		CloseHandle(snapshot_handle);
		return result;
	}

	inline std::string RandomString(SIZE_T nLen = 6)
	{
		std::string str(_XS("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"));

		std::random_device rd;
		std::mt19937 generator(rd());

		std::ranges::shuffle(str, generator);

		return str.substr(0, nLen);     
	}

	inline std::optional<std::vector<NamedModule_t>> ProcessGrabAllModules(HANDLE proc_handle)
	{
		const auto module_handles_memory = std::make_unique<HMODULE[]>(2048);

		auto* module_handles = module_handles_memory.get();
		auto array_fields_used = DWORD{};
		auto result = std::vector<NamedModule_t>();

		if (!EnumProcessModulesEx(proc_handle, module_handles, 2048, &array_fields_used, LIST_MODULES_64BIT) && GetLastError() != ERROR_PARTIAL_COPY)
			return {};

		// the other one just indicates how much memory the function has written into the buffer we've passed to it.
		// that means we obviously have to divide it by the actual size of an individual object to get the real one.
		const auto actual_fields_used = (array_fields_used / sizeof(HMODULE));

		for (uintptr_t mod_idx = 0; mod_idx < actual_fields_used; mod_idx++)
		{
			auto* mod_handle = module_handles[mod_idx];

			if (mod_handle == nullptr || mod_handle == INVALID_HANDLE_VALUE)
				continue;

			auto& mod_entry = result.emplace_back();

			mod_entry.m_handle = mod_handle;

			if (!GetModuleInformation(proc_handle, mod_handle, &mod_entry.m_info, sizeof(MODULEINFO)))
				continue;

			std::vector<char> mod_name_buffer(MAX_PATH);

			if (!GetModuleBaseName(proc_handle, mod_handle, mod_name_buffer.data(), static_cast<DWORD>(mod_name_buffer.size())))
				continue;

			mod_entry.m_name = mod_name_buffer.data();
		}

		return std::optional<decltype(result)>(result);
	}

	inline bool read_file_from_memory(const std::string& path, std::vector<uint8_t>& buffer)
	{
		if (path.empty())
			return false;

		auto stream = std::ifstream(path, std::ios::binary | std::ios::beg);

		if (!stream.is_open())
			return false;

		auto size = size_t();

		stream.seekg(0, std::ios::end);

		size = static_cast<size_t>(stream.tellg());

		stream.seekg(0, std::ios::beg);

		buffer.resize(size);

		auto* const buffer_ptr = reinterpret_cast<char*>(buffer.data());

		stream.read(buffer_ptr, buffer.size());
		stream.close();

		return true;
	}

	inline bool read_file_from_memory_ex(const std::string& path, size_t len, std::vector<uint8_t>& buffer)
	{
		if (path.empty())
			return false;

		auto size = len;

		if (!size)
			return false;

		auto stream = std::ifstream(path, std::ios::binary | std::ios::beg);

		if (!stream.is_open())
			return false;

		buffer.resize(size);

		auto* const buffer_ptr = reinterpret_cast<char*>(buffer.data());

		stream.read(buffer_ptr, buffer.size());
		stream.close();

		return true;
	}

	inline bool get_user_name(char* buffer, size_t length)
	{
		u_long capacity = (u_long)length;

		return GetUserName(buffer, &capacity);
	}

	inline void get_user_root_dir(const std::string& name, std::string& buffer)
	{
		buffer = fmt::format(_XS(R"(C:\Users\{}\)"), name);
	}

	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

	struct file_t
	{
		char name[MAX_PATH];
		size_t length;
	};

	inline bool get_directory_contents(const std::string& path, std::vector<file_t>& buffer)
	{
		if (path.empty() || !std::filesystem::exists(path))
			return false;

		std::error_code error{};
		std::filesystem::recursive_directory_iterator walker(path);

		for (auto i = std::filesystem::begin(walker); i != std::filesystem::end(walker); i = i.increment(error))
		{
			try
			{
				if (error)
					continue;

				const auto dirName = i->path().string();

				auto& newEntry = buffer.emplace_back();

				if (dirName.size() > (MAX_PATH - 1))
					continue;

				newEntry.length = std::filesystem::file_size(dirName);

				strcpy_s(newEntry.name, (char*)dirName.data());
			}
			catch (...)
			{
				continue;
			}
		}

		return true;
	}

	inline bool get_computer_name(char* buffer, size_t length)
	{
		u_long capacity = (u_long)length;

		return GetComputerName(buffer, &capacity);
	}

	inline bool create_file_from_memory(std::vector<std::uint8_t>& buffer, const std::string& path)
	{
		if (path.empty() || buffer.empty())
			return false;

		auto stream = std::ofstream(path, std::ios::binary);

		if (!stream.is_open())
			return false;

		stream.write(mem::addr(buffer.data()).As<LPCSTR>(), buffer.size());

		stream.close();

		return true;
	}

	inline BOOLEAN calculate_segment_hashes(std::vector< std::pair<std::string, std::uint32_t> >& buffer)
	{
		const auto dwMyBase = mem::addr(GetModuleHandle(nullptr)).Base();

		auto* const pMyDh = mem::addr(dwMyBase)
			.As<PIMAGE_DOS_HEADER>();

		if (!dwMyBase || pMyDh == nullptr || pMyDh->e_magic != IMAGE_DOS_SIGNATURE)
			return FALSE;

		auto* const pMyNth = mem::addr(dwMyBase)
			.Add(pMyDh->e_lfanew)
			.As<PIMAGE_NT_HEADERS>();

		if (pMyNth == nullptr || pMyNth->Signature != IMAGE_NT_SIGNATURE)
			return FALSE;

		auto* const pMySh = IMAGE_FIRST_SECTION(pMyNth);

		if (pMySh == nullptr)
			return FALSE;

		SIZE_T nMySecs = pMyNth->FileHeader.NumberOfSections;

		if (nMySecs <= 0)
			return FALSE;

		for (SIZE_T i = 0; i < nMySecs; i++)
		{
			const auto& sSecInfo = pMySh[i];
			const auto uSecLength = sSecInfo.Misc.VirtualSize;
			const auto dwSecBegin = mem::addr(dwMyBase)
				.Add(sSecInfo.VirtualAddress)
				.Base();

			const auto pszSecName = mem::addr((void*)&sSecInfo.Name[0])
				.As<LPCSTR>();

			if (std::string(pszSecName) != _XS(".text"))
				continue;

			auto bSecBuffer = std::vector<std::uint8_t>(uSecLength);

			RtlZeroMemory(bSecBuffer.data(), bSecBuffer.size());

			RtlCopyMemory(bSecBuffer.data(),
				mem::addr(dwSecBegin).Ptr(), bSecBuffer.size());

			const auto uSecHash = CRC::Calculate(bSecBuffer.data(), bSecBuffer.size(), CRC::CRC_32());

			buffer.emplace_back( std::make_pair(pszSecName, uSecHash) );
		}

		return TRUE;
	}

	inline bool query_system_firmware_table(uint32_t signature, void** buffer, size_t* size)
	{
		if (!signature || !buffer)
			return false;

		const auto length = GetSystemFirmwareTable(signature, 0, nullptr, 0);

		if (length <= 0)
			return false;

		*buffer = malloc(length);

		if (!(*buffer))
			return false;

		if (!GetSystemFirmwareTable(signature, 0, *buffer, length))
			return false;

		*size = length;

		return true;
	}

	namespace Apex
	{
		inline INT GetEntityIndexByHandle(DWORD_PTR eHandle) {
			return static_cast<int>(eHandle & 0xFFFF);
		}

		inline BOOLEAN WorldToScreen(D3DMATRIX vMatrix, Math::Vector3& vIn, Math::Vector3& vOut, CONST Math::dimension_t<INT>& Screen)
		{
			vOut.x = vMatrix.m[0][0] * vIn.x + vMatrix.m[0][1] * vIn.y + vMatrix.m[0][2] * vIn.z + vMatrix.m[0][3];
			vOut.y = vMatrix.m[1][0] * vIn.x + vMatrix.m[1][1] * vIn.y + vMatrix.m[1][2] * vIn.z + vMatrix.m[1][3];

			float w = vMatrix.m[3][0] * vIn.x + vMatrix.m[3][1] * vIn.y + vMatrix.m[3][2] * vIn.z + vMatrix.m[3][3];

			if (w < 0.01)
				return FALSE;

			float invw = 1.0f / w;

			vOut.x *= invw;
			vOut.y *= invw;

			CONST AUTO Cl_width = Screen.w;
			CONST AUTO Cl_height = Screen.h;

			auto x = static_cast<float>(Cl_width) / 2.f;
			auto y = static_cast<float>(Cl_height) / 2.f;

			x += 0.5f * vOut.x * Cl_width + 0.5f;
			y -= 0.5f * vOut.y * Cl_height + 0.5f;

			vOut.x = x;
			vOut.y = y;

			return TRUE;
		}
	}

	namespace pe
	{
		struct section_info_t
		{
			std::uintptr_t m_address{};
			std::uintptr_t m_length{};
		};

		struct reloc_info_t
		{
			std::uint16_t* m_item{};
			std::uint64_t m_address{};
			std::uint32_t m_count{};
		};

		struct import_function_info_t
		{
			std::uint64_t* m_address{};
			std::string m_name{};
		};

		struct import_info_t
		{
			std::string m_module_name{};
			std::vector<import_function_info_t> m_data{};
		};

		using reloc_list = std::vector<reloc_info_t>;
		using import_list = std::vector<import_info_t>;

		inline auto find_section_by_name(PIMAGE_NT_HEADERS header, const std::string& name) -> std::optional<section_info_t>
		{
			if (!header || name.empty())
				return {};

			auto* const section_head = IMAGE_FIRST_SECTION(header);

			std::optional<section_info_t> result = std::nullopt;
			std::vector<std::uint8_t> needle(0x8);

			std::memset(needle.data(), 0x00, needle.size());
			std::memcpy(needle.data(), name.data(), needle.size());

			size_t index = 0;

			for (auto& bit : needle)
			{
				if (index > name.length() && bit != 0x00)
					bit = 0x00;

				++index;
			}

			for (auto index = 0; index < header->FileHeader.NumberOfSections; index++)
			{
				const auto& section = section_head[index];

				if (!std::memcmp(needle.data(), &section.Name[0], 0x8))
				{
					auto sec_info = section_info_t{};

					sec_info.m_address = section.VirtualAddress;
					sec_info.m_length = section.Misc.VirtualSize;

					result = std::make_optional(sec_info);
				}
			}

			return result;
		}

		inline auto resolve_rva(PIMAGE_NT_HEADERS nt_header, std::uintptr_t base, std::uintptr_t target) -> std::uintptr_t
		{
			auto* const section_header = IMAGE_FIRST_SECTION(nt_header);

			fmt::print("  target rva: {:x}.\n", target);

			for (SIZE_T Index = 0; Index < nt_header->FileHeader.NumberOfSections; Index++)
			{
				const auto& current_section_header = section_header[Index];

				const auto current_section_begin_rva = section_header[Index].VirtualAddress;
				const auto current_section_end_rva =
					(current_section_begin_rva + section_header[Index].SizeOfRawData);

				fmt::print("  processing section ({:x} - {:x}).\n", current_section_begin_rva, current_section_end_rva);

				if ((target >= current_section_begin_rva) &&
					(target < current_section_end_rva))
					return (current_section_header.PointerToRawData +
						(target - current_section_begin_rva));
			}

			return 0;
		}

		inline auto find_relocations(void* head, PIMAGE_NT_HEADERS nt_header)
		{
			auto result = reloc_list();

			auto current_base_relocation = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<uint64_t>(head) +
				RVA_TO_VA_IMAGE(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress, IMAGE_FIRST_SECTION(nt_header), nt_header->FileHeader.NumberOfSections).Base());

			const auto reloc_end = reinterpret_cast<uint64_t>(current_base_relocation) + nt_header->OptionalHeader.
				DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

			while (current_base_relocation->VirtualAddress && current_base_relocation->VirtualAddress < reloc_end &&
				current_base_relocation->SizeOfBlock)
			{
				reloc_info_t reloc_info{};

				reloc_info.m_address = reinterpret_cast<uint64_t>(head) + current_base_relocation->VirtualAddress;
				reloc_info.m_item = reinterpret_cast<uint16_t*>(reinterpret_cast<uint64_t>(current_base_relocation) + sizeof(
					IMAGE_BASE_RELOCATION));
				reloc_info.m_count = (current_base_relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);

				result.push_back(reloc_info);

				current_base_relocation = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<uint64_t>(
					current_base_relocation) + current_base_relocation->SizeOfBlock);
			}

			return result;
		}

		inline auto find_imports(void* head, PIMAGE_NT_HEADERS nt_header)
		{
			auto result = import_list();

			auto* const section_header = IMAGE_FIRST_SECTION(nt_header);
			const auto section_count = nt_header->FileHeader.NumberOfSections;

			const auto import_directory_va = RVA_TO_VA_IMAGE(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
				section_header, section_count);

			auto* current_import_descriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(reinterpret_cast<uint64_t>(head) +
				import_directory_va.Base());

			while (current_import_descriptor->FirstThunk)
			{
				import_info_t import_info{};

				auto name_va_opt = RVA_TO_VA_IMAGE_EX(current_import_descriptor->Name, IMAGE_FIRST_SECTION(nt_header), nt_header->FileHeader.NumberOfSections);
				auto name_va = *name_va_opt;

				if (name_va_opt == std::nullopt || !name_va.Base())
					break;

				import_info.m_module_name = std::string(
					reinterpret_cast<char*>(reinterpret_cast<uint64_t>(head) + name_va.Base()));

				auto current_first_thunk = reinterpret_cast<PIMAGE_THUNK_DATA64>(reinterpret_cast<uint64_t>(head) +
					RVA_TO_VA_IMAGE(current_import_descriptor->FirstThunk, section_header, section_count).Base());

				auto current_originalFirstThunk = reinterpret_cast<PIMAGE_THUNK_DATA64>(reinterpret_cast<uint64_t>(head) +
					RVA_TO_VA_IMAGE(current_import_descriptor->OriginalFirstThunk, section_header, section_count).Base());

				while (current_originalFirstThunk->u1.Function)
				{
					import_function_info_t import_function_data{};

					auto thunk_data_va = RVA_TO_VA_IMAGE(current_originalFirstThunk->u1.AddressOfData, section_header, section_count);

					if (!thunk_data_va.Base())
						break;

					auto thunk_data = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(reinterpret_cast<uint64_t>(head) + thunk_data_va.Base());

					import_function_data.m_name = thunk_data->Name;
					import_function_data.m_address = &current_first_thunk->u1.Function;

					import_info.m_data.push_back(import_function_data);

					++current_originalFirstThunk;
					++current_first_thunk;
				}

				result.push_back(import_info);
				++current_import_descriptor;
			}

			return result;
		}
	}

	extern bool map_and_run_image(const std::string& path, uint8_t* loader, size_t loader_length);
}
