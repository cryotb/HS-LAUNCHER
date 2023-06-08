#include <Include.hpp>
#include <windowsx.h>

LRESULT CALLBACK loader::WindowProcess(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	const auto HandleNchitTest = [h_wnd, msg, w_param, l_param]() -> LRESULT
	{
		const LRESULT result = ::DefWindowProc(h_wnd, msg, w_param, l_param);

		POINT pt;
		pt.x = GET_X_LPARAM(l_param);
		pt.y = GET_Y_LPARAM(l_param);

		RECT rcWindow{};
		RECT rcDraggable{};

		if (GetWindowRect(h_wnd, &rcWindow)) {
			rcDraggable = rcWindow;
			rcDraggable.bottom = rcDraggable.top + 25;
		}

		if ((result == HTCLIENT) && (PtInRect(&rcDraggable, pt)))
		{
			return HTCAPTION;
		}

		return result;
	};

	if(g::input->is_ready())
		g::input->wnd_proc(msg, l_param, w_param);

	switch (msg)
	{
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
		return FALSE;
	default:
		break;
	}

	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(h_wnd);
		ExitProcess(0);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_NCHITTEST:
		return HandleNchitTest();
		break;
	default:
		return DefWindowProcA(h_wnd, msg, w_param, l_param);
	}

	return 0;
}

void loader::PreAuth()
{
	std::string user_name{}, pass_word{};

	const auto request_auth_input = [&user_name, &pass_word]() -> bool
	{
		const auto prompt_for_data = [&user_name, &pass_word]()
		{
			LOG_INFO("please enter your hellscythe login.");

			std::getline(std::cin, user_name);
			std::getline(std::cin, pass_word);

			if (user_name.empty() || pass_word.empty())
				return false;

			return true;
		};

		const auto can_use_user_data_for_auth = [&user_name, &pass_word]() -> bool
		{
			auto userData = helloverlay::UserData_t{};

			if (!helloverlay::GetUserDataContents(&userData, sizeof(userData)))
				return false;

			if (!userData.TermsAccepted || !userData.WasAuthorized)
				return false;

			user_name = userData.LastUserName;
			pass_word = userData.LastPassWord;

			if (user_name.empty() || pass_word.empty())
				return false;

			return true;
		};

		if (helloverlay::GetUserDataPresent())
		{
			//
			// User data is present, check if authorized.
			//
			if (!can_use_user_data_for_auth())
				prompt_for_data();
		}
		else
		{
			//
			// User data is NOT present, prompt for data.
			//
			prompt_for_data();
		}

		return true;
	};

	if (!request_auth_input())
		return;

	loader::g::auth_data = { user_name, pass_word };
	
	Util::DeleteConsole();
}

void loader::Initialize(net::c_client& net_client, HINSTANCE h_instance)
{
	g::net_client = net_client;
	g::input = new Shared::Loader::c_input_manager();
	g::sinput = new Shared::Loader::c_synced_input();
	g::render = new Shared::c_render9();

	if (!g::input || !g::sinput || !g::render)
	{
		MESSAGE_BOX("fatal error: UI memory allocation failure.", MB_ICONERROR);
		return;
	}

	RegisterStateChangeCallback(&logic::OnStateChanged);

	g::running = true;
	g::rendering = true;

	std::thread(WindowEventThread, h_instance).detach();
}

void loader::WindowEventThread(HINSTANCE h_instance)
{
	if (!bulk_initialize_components(nullptr, &WindowProcess, 0xA, 250, 300))
	{
		MESSAGE_BOX("fatal error: bulk initialization of UI components have failed.", MB_ICONERROR);
		return;
	}

	UI::Render::renderer = g::render;
	UI::Render::m_synced_input = g::sinput;

	UI::elements::renderer = g::render;
	UI::elements::input = g::input;
	UI::elements::synced_input = g::sinput;

	MSG my_message{};

	UpdateState(States::Initializing);

	do
	{
		g::input->synchronize(g::sinput);

		if (my_message.message == WM_QUIT)
			break;

		if (PeekMessageA(&my_message, g::window_handle, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&my_message);
			DispatchMessageA(&my_message);
		}
		else
		{
			if(g::rendering)
				Render();
		}

		std::this_thread::sleep_for(5ms);
	} while (g::running);
}

void loader::Render()
{
	g::direct_device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 10, 10, 10), 1.0f, 0);
	g::direct_device->BeginScene();

	//
	//
	//
	loader::ui::Render();

	g::direct_device->EndScene();
	g::direct_device->Present(nullptr, nullptr, nullptr, nullptr);
}

void loader::Shutdown()
{
	g::running = false;
}
