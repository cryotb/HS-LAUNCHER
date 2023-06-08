#include <include.hpp>

int main(int arg_c, char* arg_v[])
{
	const auto handle_file_on_disk = []() -> bool
	{
		std::string my_path{};

		if (util::get_self_name(my_path))
		{
			const auto my_name = std::filesystem::path(my_path).filename().string();

			if (strstr(my_name.c_str(), _XS("Hellrunner")))
			{
				const auto new_path = fmt::format(_XS("{}.exe"), util::get_random_string(12));
				const auto result = rename(my_path.c_str(), new_path.c_str());

				return !(result == ERROR_SUCCESS);
			}

			return true;
		}

		return true;
	};

	if (arg_c >= 2)
	{
		LOG_INFO("starting in compatibility mode.");
		g::is_compat_mode = strstr(arg_v[1], _XS("-compat"));
	}

	g::is_debug_build = util::is_debug_build();

	if (!g::is_debug_build && !g::is_compat_mode)
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	else
		ShowWindow(GetConsoleWindow(), SW_SHOWDEFAULT);

	if (!util::get_self_path(g::my_path))
		return EXIT_FAILURE;

	if (!g::is_debug_build && !handle_file_on_disk())
	{
		MessageBox(nullptr, _XS("program has been modified, please restart the program (1)."), nullptr, MB_ICONERROR);
		return EXIT_FAILURE;
	}

	if (!hellrunner::dispatch())
		return EXIT_FAILURE;

	if (g::is_compat_mode)
	{
		do
		{
			std::this_thread::yield();
		} while (true);
	}

	if (g::is_debug_build)
		return std::getchar();

	return EXIT_SUCCESS;
}
