#include <Include.hpp>

std::uint16_t shared::mapping::load_library(void* file_buffer, HANDLE process_handle, uint8_t* loader_bytes, size_t loader_size)
{
	auto* const dos_header = static_cast<PIMAGE_DOS_HEADER>(file_buffer);

	if (dos_header == nullptr)
		return 0xE02;

	auto* nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<LPBYTE>(file_buffer) + dos_header->e_lfanew);
	auto* const executable_image = VirtualAllocEx(process_handle, nullptr, nt_headers->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	if (executable_image == nullptr)
		return 0xE24;

	if (!WriteProcessMemory(process_handle, executable_image, file_buffer,
		nt_headers->OptionalHeader.SizeOfHeaders, nullptr))
	{
		VirtualFreeEx(process_handle, executable_image, 0, MEM_RELEASE);
		return 0xE26;
	}

	auto* const section_header = reinterpret_cast<PIMAGE_SECTION_HEADER>(nt_headers + 1);

	if (section_header == nullptr)
	{
		VirtualFreeEx(process_handle, executable_image, 0, MEM_RELEASE);
		return 0xE04;
	}

	for (auto i = 0; i < nt_headers->FileHeader.NumberOfSections; i++)
	{
		const auto base_ptr = static_cast<PVOID>(static_cast<LPBYTE>(executable_image) + section_header[i].VirtualAddress);
		const auto buffer_ptr = static_cast<PVOID>(static_cast<LPBYTE>(file_buffer) + section_header[i].PointerToRawData);

		const auto result = WriteProcessMemory(process_handle, base_ptr, buffer_ptr,
			section_header[i].SizeOfRawData, nullptr);

		if (!result)
		{
			VirtualFreeEx(process_handle, executable_image, 0, MEM_RELEASE);
			return 0xE28;
		}
	}

	auto* const LoaderMemory = VirtualAllocEx(process_handle, nullptr, 8192, MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE); // Allocate memory for the loader code

	if (LoaderMemory == nullptr)
	{
		VirtualFreeEx(process_handle, executable_image, 0, MEM_RELEASE);
		return 0xE06;
	}

	//fmt::print(" remote loader address: {:x}.\n", Memory::Address(LoaderMemory).Base());

	loader_data_t loader_parameters{};

	const auto initialize_params = [process_handle, executable_image, dos_header, nt_headers](loader_data_t& params)
	{
		params.ImageBase = executable_image;
		params.TargetBase = nullptr; // todo: add functionality.

		params.NtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<LPBYTE>(executable_image) + dos_header->e_lfanew);

		params.BaseReloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(static_cast<LPBYTE>(executable_image)
			+ nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

		params.ImportDirectory = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(static_cast<LPBYTE>(executable_image)
			+ nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

		params.fnLoadLibraryA = LoadLibraryA;
		params.fnGetProcAddress = GetProcAddress;
	};

	initialize_params(loader_parameters);

	if (WriteProcessMemory(process_handle, LoaderMemory, &loader_parameters, sizeof(loader_data_t), nullptr) == FALSE)
	{
		VirtualFreeEx(process_handle, executable_image, 0, MEM_RELEASE);
		VirtualFreeEx(process_handle, LoaderMemory, 0, MEM_RELEASE);

		return 0xE08;
	}

	const auto loader_entry_point_address = 
		reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(LoaderMemory) + sizeof(loader_data_t) + 0x8);

	auto loader_begin = reinterpret_cast<void*>(loader_bytes);
	auto loader_length = loader_size;

	if (WriteProcessMemory(process_handle,
		loader_entry_point_address, loader_begin, loader_length, nullptr) == FALSE)
	{
		VirtualFreeEx(process_handle, executable_image, 0, MEM_RELEASE);
		VirtualFreeEx(process_handle, LoaderMemory, 0, MEM_RELEASE);

		return 0xE10;
	}

	auto* const h_thread = CreateRemoteThread(process_handle, nullptr, 0, 
		reinterpret_cast<LPTHREAD_START_ROUTINE>(loader_entry_point_address), LoaderMemory, NULL, nullptr);

	//
	// thread creation in the remote process has failed.
	//
	if (h_thread == nullptr)
	{
		VirtualFreeEx(process_handle, executable_image, 0, MEM_RELEASE);
		VirtualFreeEx(process_handle, LoaderMemory, 0, MEM_RELEASE);

		return 0xE12;
	}

	//
	// waiting for the loader in the remote thread has failed.
	//
	if (WaitForSingleObject(h_thread, INFINITE) == WAIT_FAILED)
	{
		VirtualFreeEx(process_handle, executable_image, 0, MEM_RELEASE);
		VirtualFreeEx(process_handle, LoaderMemory, 0, MEM_RELEASE);

		return 0xE14;
	}

	//
	// check the job status/result of the loader info.
	//
	if (ReadProcessMemory(process_handle, LoaderMemory, &loader_parameters, sizeof(loader_data_t), nullptr) == FALSE)
	{
		VirtualFreeEx(process_handle, executable_image, 0, MEM_RELEASE);
		VirtualFreeEx(process_handle, LoaderMemory, 0, MEM_RELEASE);

		return 0xE08;
	}

	const auto loader_job_info = loader_parameters.JobInfo;

	if (VirtualFreeEx(process_handle, LoaderMemory, 0, MEM_RELEASE) == FALSE)
	{
		VirtualFreeEx(process_handle, executable_image, 0, MEM_RELEASE);
		VirtualFreeEx(process_handle, LoaderMemory, 0, MEM_RELEASE);

		return 0xE16;
	}

	if (!loader_job_info.DidComplete)
		return loader_job_info.ErrorCode;

	if (!mem::proc::fill(
		process_handle, mem::addr(executable_image).Base(), 
		0, nt_headers->OptionalHeader.SizeOfHeaders))
		return 0xE49;

	return 0x00;
}
