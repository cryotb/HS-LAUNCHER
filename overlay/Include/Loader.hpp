#pragma once

namespace loader
{
	namespace options
	{
		struct gdm_load_t
		{
			bool m_enable_overlay;
			bool m_debug_console;
		} inline gdm_load{};
	}

	enum class States
	{
		Error = -1, // the loader has failed to it's job, for whatever reason that is.
		NotSet = 0, // no status could be determined because it hasn't been set.

		Initializing, // the loader is initially setting up, as the name suggests.
		Authorizing, // the loader is authorizing the client using the backend.

		Selection, // the loader is at the selection of products, it's basically ready from here on.
		LoadingPrompt,
		Loading, // the loader has determined the requested product and is loading it right now.

		Success, // the loader has finished it's job and is going to disappear in no time.
	};

	enum LoadTaskType
	{
		LTT_KernelModule,
		LTT_GameDependantModule,
	};

	enum LoadStates
	{
		LS_Initializing,
		LS_Downloading,
		LS_Waiting,
		LS_Done,
	};

	using fn_state_change_callback = void(__fastcall*)(States, States);

	namespace g
	{
		struct error_arguments_t
		{
			uint32_t id; // the id of the error.
			uint64_t params[4]; // parameters to help for debugging the problem.
			char message[256]; // a very short message which describes the error.
		} inline m_error_args{};

		struct load_arguments_t
		{
			uint64_t vcx;
			uint64_t vdx;
			uint64_t v8;
			uint64_t v9;
		} inline m_load_args{};

		inline uint64_t init_delta{};

		inline bool initialized{};

		inline bool running{};
		inline bool rendering{};

		inline States state = States::NotSet;
		inline States previous_state = States::NotSet;

		inline HWND window_handle = nullptr;

		inline Math::dimension_t<int> screen{};
		inline Math::dimension_t<int> window{};
		
		inline Shared::Loader::c_input_manager* input = nullptr;
		inline Shared::Loader::c_synced_input* sinput = nullptr;
		inline Shared::c_render9* render = nullptr;
		inline net::c_client net_client{};

		inline IDirect3DDevice9* direct_device = nullptr;

		inline std::vector<fn_state_change_callback> state_change_callbacks{};
		inline std::map<std::string, nlohmann::json> product_def_list{};

		struct auth_t
		{
			std::string user_name{};
			std::string pass_word{};
		} inline auth_data{};
	}

	inline void RegisterStateChangeCallback(fn_state_change_callback cb)
	{
		if (cb == nullptr)
			return;

		g::state_change_callbacks.emplace_back(cb);
	}

	inline void UpdateState(States newState)
	{
		g::previous_state = g::state;
		g::state = newState;

		if (!g::state_change_callbacks.empty())
		{
			for (auto* const sc_callback : g::state_change_callbacks)
			{
				if (sc_callback == nullptr)
					continue;

				std::thread(sc_callback,
					g::previous_state, 
					g::state)
					.detach();
			}
		}
	}

	inline void SetFailState(uint32_t id, const std::string& msg,
		uint64_t param1 = 0, uint64_t param2 = 0, uint64_t param3 = 0, uint64_t param4 = 0)
	{
		UpdateState(States::Error);

		g::m_error_args.id = id;

		strcpy_s(g::m_error_args.message,
			fmt::format(_XS("{} ({})"), msg.substr(0,
				sizeof(g::m_error_args.message)), id).c_str());

		auto& params = g::m_error_args.params;

		params[0] = param1;
		params[1] = param2;
		params[2] = param3;
		params[3] = param4;
	}

	inline bool setup_window(HINSTANCE h_instance, WNDPROC wnd_proc, HWND& h_wnd, const std::string& class_name, const std::string& window_name, int width, int height)
	{
		WNDCLASSEX w_class = {};

		auto* const my_window_icon = LoadIcon(nullptr, IDI_APPLICATION);
		auto* const my_default_cursor = LoadCursor(nullptr, IDC_ARROW);

		w_class.cbSize = sizeof(WNDCLASSEX);
		w_class.lpfnWndProc = wnd_proc;
		w_class.hInstance = h_instance;
		w_class.hIcon = my_window_icon;
		w_class.hCursor = my_default_cursor;
		w_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		w_class.lpszMenuName = nullptr;
		w_class.lpszClassName = class_name.c_str();
		w_class.hIconSm = my_window_icon;

		if (RegisterClassEx(&w_class) == false)
		{
			return false;
		}

		h_wnd = CreateWindowEx(0, class_name.c_str(), window_name.c_str(), 
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
			g::screen.w / 2 - width / 2, g::screen.h / 2 - height / 2,
			width, height, nullptr, nullptr, h_instance, nullptr);

		if (h_wnd == nullptr)
		{
			return false;
		}

		SetWindowLongA(h_wnd, GWL_STYLE, 
			GetWindowLongA(h_wnd, GWL_STYLE) & ~(WS_CAPTION | 
				WS_THICKFRAME | WS_MINIMIZEBOX
				| WS_MAXIMIZEBOX | WS_SYSMENU));

		return true;
	}

	inline bool initialize_direct_classes9(D3DPRESENT_PARAMETERS& present_params, IDirect3DDevice9*& direct_device9, const HWND& h_wnd, int width, int height)
	{
		auto* const direct_api9 = Direct3DCreate9(D3D_SDK_VERSION);

		if (direct_api9 == nullptr)
			return false;

		std::memset(&present_params, 0, sizeof(D3DPRESENT_PARAMETERS));

		present_params.BackBufferWidth = width;
		present_params.BackBufferHeight = height;

		present_params.BackBufferFormat = D3DFMT_X8R8G8B8;
		present_params.BackBufferCount = 1;

		present_params.MultiSampleType = D3DMULTISAMPLE_NONE;
		present_params.MultiSampleQuality = 0;

		present_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
		present_params.hDeviceWindow = h_wnd;

		present_params.Windowed = true;
		present_params.EnableAutoDepthStencil = true;

		present_params.AutoDepthStencilFormat = D3DFMT_D24S8;
		present_params.Flags = D3DPRESENT_INTERVAL_ONE;

		present_params.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		present_params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

		IDirect3DDevice9* direct_device9_ex = nullptr;

		if (FAILED(direct_api9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, h_wnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_params, &direct_device9_ex)))
			return false;

		direct_device9 = direct_device9_ex;

		return true;
	}

	inline bool bulk_initialize_components(const HINSTANCE h_instance, const WNDPROC wnd_proc, const int n_cmd_show, int window_width, int window_height)
	{
		g::screen.w = GetSystemMetrics(SM_CXSCREEN);
		g::screen.h = GetSystemMetrics(SM_CYSCREEN);

		g::window.w = window_width;
		g::window.h = window_height;

		HWND my_window_handle = nullptr;

		D3DPRESENT_PARAMETERS present_parameters{};

		if (setup_window(h_instance, wnd_proc, my_window_handle, _XS("Window"), util::get_random_string(24), g::window.w, g::window.h) == false ||
			initialize_direct_classes9(present_parameters, g::direct_device, my_window_handle, g::window.w, g::window.h) == false)
		{
			return false;
		}

		assert(my_window_handle);

		// initialize input and renderer.
		g::input->init(G::P::Memory, my_window_handle);
		g::render->initialize(g::direct_device);

		// todo: synced input.

		// show window.
		ShowWindow(my_window_handle, n_cmd_show);
		UpdateWindow(my_window_handle);

		g::window_handle = my_window_handle;

		return true;
	}

	extern void PreAuth();
	extern void Initialize(net::c_client& net_client, HINSTANCE h_instance);
	extern void Shutdown();
	extern void WindowEventThread(HINSTANCE h_instance);
	extern void Render();

	extern LRESULT CALLBACK WindowProcess(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);
}
