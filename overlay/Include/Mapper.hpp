#pragma once

#pragma warning(disable : 4834)

namespace shared::mapping
{
	typedef HMODULE(WINAPI* load_library_multibyte_fn)(LPCSTR);
	typedef FARPROC(WINAPI* get_proc_address_fn)(HMODULE, LPCSTR);

	typedef INT(__stdcall* dllmain)(HMODULE, DWORD, LPVOID);

	struct loader_job_info_t
	{
		UINT16 Magic;
		UINT16 ErrorCode;
		BOOLEAN DidComplete;

		struct Statistics_t
		{
			INT NumRelocsProcd{};
			INT NumImportsProcd{};
		} Stats{};
	};

	struct loader_data_t
	{
		LPVOID TargetBase;
		LPVOID ImageBase;

		PIMAGE_NT_HEADERS NtHeaders;
		PIMAGE_BASE_RELOCATION BaseReloc;
		PIMAGE_IMPORT_DESCRIPTOR ImportDirectory;

		// we can put kernel32 imports here since they're located at the same address for every process.
		// other ones will not work for obvious reasons, so be aware of that please.
		load_library_multibyte_fn fnLoadLibraryA;
		get_proc_address_fn fnGetProcAddress;

		// this can be used to track what the loader has done.
		// fill this out on post completion of the initial loader procedure.
		loader_job_info_t JobInfo{};
	};

	/// <summary>
	/// maps given image buffer into remote process.
	/// </summary>
	/// <param name="file_buffer"></param>
	/// <param name="process_handle"></param>
	/// <returns>zero if successful, error code if not.</returns>
	extern std::uint16_t load_library(void* file_buffer, HANDLE process_handle, uint8_t* loader_bytes, size_t loader_size);
}
