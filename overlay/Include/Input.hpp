#pragma once

namespace Shared::Loader
{
	typedef void (input_handler_fn)(uintptr_t key_code, int state);

	enum button_states
	{
		button_state_invalid = -1,
		button_state_unassigned = 0,

		button_state_released,
		button_state_busy,

		button_state_count,
	};

	struct button_t
	{
		int m_state = -1;
		int m_repeat_count = 0;
		float m_time_pressed = 0.f;
		bool m_pressed = false;
		bool m_was_pressed = false;
		bool m_repeating = false;
	};

	class c_synced_input
	{
	public:
		void sync(std::array<button_t, 255>& buttons)
		{
			assert(this);

			std::unique_lock<std::mutex> lck_grd(m_mutex);

			m_old_buttons = m_buttons;
			m_buttons = buttons;
		}

		bool is_button_down(const uintptr_t key_code)
		{
			assert(this);

			std::unique_lock<std::mutex> lck_grd(m_mutex);

			return (m_buttons[key_code].m_state == button_state_busy);
		}

		bool was_button_down(const uintptr_t key_code)
		{
			assert(this);

			std::unique_lock<std::mutex> lck_grd(m_mutex);

			return (m_old_buttons[key_code].m_state != button_state_busy && m_buttons[key_code].m_state ==
				button_state_busy);
		}

	private:
		std::mutex m_mutex;
		std::array<button_t, 255> m_buttons;
		std::array<button_t, 255> m_old_buttons;
	};

	class c_input_manager
	{
	public:
	public:
		void init(mem::manager* memory_manager, HWND hWnd);
		void shutdown(mem::manager* memory_manager);

		void update();

		void add_handler(input_handler_fn* new_fn)
		{
			assert(this);

			assert(m_ready);

			m_handlers.push_back(new_fn);
		}

		void synchronize(c_synced_input* target)
		{
			assert(this);


			assert(m_ready);
			assert(target);

			target->sync(m_buttons);
		}

		bool is_button_down(const uintptr_t key_code)
		{
			assert(this);


			assert(m_ready);

			return (m_buttons[key_code].m_pressed);
		}

		bool was_button_down(const uintptr_t key_code)
		{
			const auto time_delta = std::fabsf(m_buttons[key_code].m_time_pressed - G::CurrentTime);
			const auto is_mouse = (key_code == VK_LBUTTON || key_code == VK_RBUTTON);
			const auto max_delta = is_mouse ? 0.015f : 0.020f;
			
			return m_buttons[key_code].m_pressed && m_buttons[key_code].m_repeat_count < 2 && time_delta < max_delta;
		}

		Math::position_t<int> get_cursor_position()
		{
			assert(this);

			assert(m_ready);

			return m_cursor_position;
		}

		void on_key_down(uint32_t vk_code)
		{
			//Log::Info(_XS("key down."));

			m_buttons[vk_code].m_pressed = true;

			if (m_old_buttons[vk_code].m_pressed != m_buttons[vk_code].m_pressed)
				m_buttons[vk_code].m_repeating = true;

			if (m_buttons[vk_code].m_repeating)
				++m_buttons[vk_code].m_repeat_count;

			m_buttons[vk_code].m_time_pressed = G::CurrentTime;
		}

		void on_key_up(uint32_t vk_code)
		{
			//Log::Info(_XS("key up."));

			m_buttons[vk_code].m_pressed = false;
			m_buttons[vk_code].m_repeating = false;
			m_buttons[vk_code].m_repeat_count = 0;
		}

		void on_keyboard_msg(int nCode, WPARAM wParam, LPARAM lParam)
		{
			CONST AUTO uMessage = wParam;
			AUTO* CONST pInfo = mem::addr(lParam).As<PKBDLLHOOKSTRUCT>();

			if (pInfo == nullptr)
				return;

			switch(uMessage)
			{
			case WM_KEYDOWN:
				on_key_down(pInfo->vkCode);
				break;
			case WM_KEYUP:
				on_key_up(pInfo->vkCode);
				break;
			default:
				break;
			}
		}

		void on_mouse_msg(int nCode, WPARAM wParam, LPARAM lParam)
		{
			CONST AUTO uMessage = wParam;
			AUTO* CONST pInfo = mem::addr(lParam).As<PMSLLHOOKSTRUCT>();

			if (pInfo == nullptr)
				return;

			switch (uMessage)
			{
			case WM_LBUTTONDOWN:
				on_key_down(VK_LBUTTON);
				break;
			case WM_LBUTTONUP:
				on_key_up(VK_LBUTTON);
				break;
			case WM_RBUTTONDOWN:
				on_key_down(VK_RBUTTON);
				break;
			case WM_RBUTTONUP:
				on_key_up(VK_RBUTTON);
				break;
			default:
				break;
			}
		}

		void wnd_proc(const uintptr_t msg, const WPARAM w_param, const LPARAM l_param)
		{
			assert(m_ready);

			m_old_buttons = m_buttons;

			if (msg == WM_KEYDOWN && !(HIWORD(l_param) & KF_REPEAT))
			{
				internal_handler_key_down(w_param);
			}
			else if (msg == WM_KEYUP)
			{
				internal_handler_key_up(w_param);
			}
			else if (msg == WM_LBUTTONDOWN && !(HIWORD(l_param) & KF_REPEAT))
			{
				internal_handler_key_down(VK_LBUTTON);
			}
			else if (msg == WM_MBUTTONDOWN && !(HIWORD(l_param) & KF_REPEAT))
			{
				internal_handler_key_down(VK_MBUTTON);
			}
			else if (msg == WM_RBUTTONDOWN && !(HIWORD(l_param) & KF_REPEAT))
			{
				internal_handler_key_down(VK_RBUTTON);
			}
			else if (msg == WM_LBUTTONUP)
			{
				internal_handler_key_up(VK_LBUTTON);
			}
			else if (msg == WM_MBUTTONUP)
			{
				internal_handler_key_up(VK_MBUTTON);
			}
			else if (msg == WM_RBUTTONUP)
			{
				internal_handler_key_up(VK_RBUTTON);
			}
		}
		
		AUTO is_ready() CONST {
			return m_ready;
		}
	private:
		void internal_handler_key_down(const uintptr_t key_code)
		{
			if (key_code > m_buttons.size() - 1)
				return;

			m_buttons[key_code].m_state = button_state_busy;
			m_buttons[key_code].m_pressed = true;

			// execute handlers.
			if (m_handlers.empty() == false)
			{
				for (const auto& handler : m_handlers)
				{
					handler(key_code, button_state_busy);
				}
			}
		}

		void internal_handler_key_up(const uintptr_t key_code)
		{
			if (key_code > m_buttons.size() - 1)
				return;

			m_buttons[key_code].m_state = button_state_released;

			// execute handlers.
			if (m_handlers.empty() == false)
			{
				for (const auto& handler : m_handlers)
				{
					handler(key_code, button_state_released);
				}
			}
		}
	private:
		HWND m_window_handle = nullptr;
		bool m_ready = false;
		std::array<button_t, 255> m_buttons{};
		std::array<button_t, 255> m_old_buttons{};
		std::vector<input_handler_fn*> m_handlers{};
		Math::position_t<int> m_cursor_position{};
	};
}

#pragma once

namespace Shared
{
	typedef void (input_handler_fn)(uintptr_t key_code, int state);

	enum button_states
	{
		button_state_invalid = -1,
		button_state_unassigned = 0,

		button_state_released,
		button_state_busy,

		button_state_count,
	};

	struct button_t
	{
		int m_state = -1;
		int m_repeat_count = 0;
		float m_time_pressed = 0.f;
		bool m_pressed = false;
		bool m_was_pressed = false;
		bool m_repeating = false;
	};

	class c_synced_input
	{
	public:
		void sync(std::array<button_t, 255>& buttons)
		{
			assert(this);

			std::unique_lock<std::mutex> lck_grd(m_mutex);

			m_old_buttons = m_buttons;
			m_buttons = buttons;
		}

		bool is_button_down(const uintptr_t key_code)
		{
			assert(this);

			std::unique_lock<std::mutex> lck_grd(m_mutex);

			return (m_buttons[key_code].m_state == button_state_busy);
		}

		bool was_button_down(const uintptr_t key_code)
		{
			assert(this);

			std::unique_lock<std::mutex> lck_grd(m_mutex);

			return m_buttons[key_code].m_repeating == false;
		}

	private:
		std::mutex m_mutex;
		std::array<button_t, 255> m_buttons;
		std::array<button_t, 255> m_old_buttons;
	};

	class c_input_manager
	{
	public:
	public:
		void init(mem::manager* memory_manager, HWND hWnd);
		void shutdown(mem::manager* memory_manager);

		void update();

		void add_handler(input_handler_fn* new_fn)
		{
			assert(this);

			assert(m_ready);

			m_handlers.push_back(new_fn);
		}

		void synchronize(c_synced_input* target)
		{
			assert(this);


			assert(m_ready);
			assert(target);

			target->sync(m_buttons);
		}

		bool is_button_down(const uintptr_t key_code)
		{
			assert(this);


			assert(m_ready);

			return (m_buttons[key_code].m_pressed);
		}

		bool was_button_down(const uintptr_t key_code)
		{
			const auto time_delta = std::fabsf(m_buttons[key_code].m_time_pressed - G::CurrentTime);
			const auto is_mouse = (key_code == VK_LBUTTON || key_code == VK_RBUTTON);
			const auto max_delta = is_mouse ? 0.015f : 0.020f;

			return m_buttons[key_code].m_pressed && m_buttons[key_code].m_repeat_count < 2 && time_delta < max_delta;
		}

		Math::position_t<int> get_cursor_position()
		{
			assert(this);

			assert(m_ready);

			return m_cursor_position;
		}

		void on_key_down(uint32_t vk_code)
		{
			//Log::Info(_XS("key down."));

			m_buttons[vk_code].m_pressed = true;

			if (m_old_buttons[vk_code].m_pressed != m_buttons[vk_code].m_pressed)
				m_buttons[vk_code].m_repeating = true;

			if (m_buttons[vk_code].m_repeating)
				++m_buttons[vk_code].m_repeat_count;

			m_buttons[vk_code].m_time_pressed = G::CurrentTime;
		}

		void on_key_up(uint32_t vk_code)
		{
			//Log::Info(_XS("key up."));

			m_buttons[vk_code].m_pressed = false;
			m_buttons[vk_code].m_repeating = false;
			m_buttons[vk_code].m_repeat_count = 0;
		}

		void on_keyboard_msg(int nCode, WPARAM wParam, LPARAM lParam)
		{
			CONST AUTO uMessage = wParam;
			AUTO* CONST pInfo = mem::addr(lParam).As<PKBDLLHOOKSTRUCT>();

			if (pInfo == nullptr)
				return;

			switch (uMessage)
			{
			case WM_KEYDOWN:
				on_key_down(pInfo->vkCode);
				break;
			case WM_KEYUP:
				on_key_up(pInfo->vkCode);
				break;
			default:
				break;
			}
		}

		void on_mouse_msg(int nCode, WPARAM wParam, LPARAM lParam)
		{
			CONST AUTO uMessage = wParam;
			AUTO* CONST pInfo = mem::addr(lParam).As<PMSLLHOOKSTRUCT>();

			if (pInfo == nullptr)
				return;

			switch (uMessage)
			{
			case WM_LBUTTONDOWN:
				on_key_down(VK_LBUTTON);
				break;
			case WM_LBUTTONUP:
				on_key_up(VK_LBUTTON);
				break;
			case WM_RBUTTONDOWN:
				on_key_down(VK_RBUTTON);
				break;
			case WM_RBUTTONUP:
				on_key_up(VK_RBUTTON);
				break;
			default:
				break;
			}
		}

		void wnd_proc(const uintptr_t msg, const WPARAM w_param, const LPARAM l_param)
		{
			assert(m_ready);

			m_old_buttons = m_buttons;

			if (msg == WM_KEYDOWN && !(HIWORD(l_param) & KF_REPEAT))
			{
				internal_handler_key_down(w_param);
			}
			else if (msg == WM_KEYUP)
			{
				internal_handler_key_up(w_param);
			}
			else if (msg == WM_LBUTTONDOWN && !(HIWORD(l_param) & KF_REPEAT))
			{
				internal_handler_key_down(VK_LBUTTON);
			}
			else if (msg == WM_MBUTTONDOWN && !(HIWORD(l_param) & KF_REPEAT))
			{
				internal_handler_key_down(VK_MBUTTON);
			}
			else if (msg == WM_RBUTTONDOWN && !(HIWORD(l_param) & KF_REPEAT))
			{
				internal_handler_key_down(VK_RBUTTON);
			}
			else if (msg == WM_LBUTTONUP)
			{
				internal_handler_key_up(VK_LBUTTON);
			}
			else if (msg == WM_MBUTTONUP)
			{
				internal_handler_key_up(VK_MBUTTON);
			}
			else if (msg == WM_RBUTTONUP)
			{
				internal_handler_key_up(VK_RBUTTON);
			}
		}

		AUTO is_ready() CONST {
			return m_ready;
		}
	private:
		void internal_handler_key_down(const uintptr_t key_code)
		{
			m_buttons[key_code].m_state = button_state_busy;
			m_buttons[key_code].m_pressed = true;

			// execute handlers.
			if (m_handlers.empty() == false)
			{
				for (const auto& handler : m_handlers)
				{
					handler(key_code, button_state_busy);
				}
			}
		}

		void internal_handler_key_up(const uintptr_t key_code)
		{
			m_buttons[key_code].m_state = button_state_released;

			// execute handlers.
			if (m_handlers.empty() == false)
			{
				for (const auto& handler : m_handlers)
				{
					handler(key_code, button_state_released);
				}
			}
		}
	private:
		HWND m_window_handle = nullptr;
		bool m_ready = false;
		std::array<button_t, 255> m_buttons{};
		std::array<button_t, 255> m_old_buttons{};
		std::vector<input_handler_fn*> m_handlers{};
		Math::position_t<int> m_cursor_position{};
	};
}


