#include <Include.hpp>

void Shared::c_input_manager::init(mem::manager* memory_manager, HWND hWnd)
{
	assert(this);
	assert(memory_manager && hWnd);

	// create our required threads.
	std::thread(&c_input_manager::update, this).detach();

	m_window_handle = hWnd;
	m_ready = true;
}

void Shared::c_input_manager::shutdown(mem::manager* memory_manager)
{
	assert(this);
	assert(memory_manager);

	m_ready = false;
}

void Shared::c_input_manager::update()
{
	auto set_cursor_position = [this](const Math::position_t<int> position)
	{

		m_cursor_position = position;
	};

	const auto grab_cursor_position = [this, set_cursor_position]()
	{
		POINT screen_cursor_position{}, window_cursor_position{};

		if (!GetCursorPos(&screen_cursor_position))
			return false;

		window_cursor_position = screen_cursor_position;

		if (!ScreenToClient(m_window_handle, &window_cursor_position))
			return false;

		set_cursor_position({ window_cursor_position.x, window_cursor_position.y });

		return true;
	};

	Log::Info(_XS("input manager thread has started."));

	while (true)
	{
		if (G::P::Unload)
			break;

		if (!grab_cursor_position())
			continue;

		Sleep(1);
	}

	Log::Info(_XS("input manager thread has shut down."));
}

void Shared::Loader::c_input_manager::init(mem::manager* memory_manager, HWND hWnd)
{
	assert(this);
	assert(memory_manager && hWnd);

	// create our required threads.
	std::thread(&c_input_manager::update, this).detach();

	m_window_handle = hWnd;
	m_ready = true;
}

void Shared::Loader::c_input_manager::shutdown(mem::manager* memory_manager)
{
	assert(this);
	assert(memory_manager);

	m_ready = false;
}

void Shared::Loader::c_input_manager::update()
{
	auto set_cursor_position = [this](const Math::position_t<int> position)
	{

		m_cursor_position = position;
	};

	const auto grab_cursor_position = [this, set_cursor_position]()
	{
		POINT screen_cursor_position{}, window_cursor_position{};

		if (!GetCursorPos(&screen_cursor_position))
			return false;

		window_cursor_position = screen_cursor_position;

		if (!ScreenToClient(m_window_handle, &window_cursor_position))
			return false;

		set_cursor_position({ window_cursor_position.x, window_cursor_position.y });

		return true;
	};

	Log::Info(_XS("input manager thread has started."));

	while (true)
	{
		if (G::P::Unload)
			break;

		if (!grab_cursor_position())
			continue;

		Sleep(1);
	}

	Log::Info(_XS("input manager thread has shut down."));
}
