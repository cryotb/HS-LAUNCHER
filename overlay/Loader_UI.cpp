#include <Include.hpp>

//
// Some Helper Macros for Rendering
//

#define DRAW_TEXT(X, Y, COLOR, FLAGS, FONT, TXT) g::render->draw_text(X, Y, COLOR, TXT, Shared::dt_align_center, FONT)
#define DRAW_TEXT_CENTERED_RAW(X, Y, COLOR, TXT) g::render->draw_text(X, Y, COLOR, TXT, Shared::dt_align_center)
#define DRAW_TEXT_CENTERED(X, Y, COLOR, TXT) g::render->draw_text(X, Y, COLOR, _XS(TXT), Shared::dt_align_center)

//
//
//

void loader::ui::DrawBase()
{

}

void loader::ui::DrawError()
{
	const auto start_x = g::window.w / 2.f, start_y = g::window.h / 4.f;
	auto push_y = 0.f;

	DRAW_TEXT(
		start_x,
		start_y + push_y,
		Shared::Colors::red,
		Shared::dt_align_center,
		g::render->m_fonts.m_verdana_small,
		g::m_error_args.message
	);

	push_y += 45.f;

	DRAW_TEXT_CENTERED_RAW(
		start_x,
		start_y + push_y,
		Shared::Colors::white, fmt::format("[1] =  0x{:x}\n"
										   "[2] =  0x{:x}\n"
										   "[3] =  0x{:x}\n"
										   "[4] =  0x{:x}\n", 
			g::m_error_args.params[0],
			g::m_error_args.params[1],
			g::m_error_args.params[2],
			g::m_error_args.params[3])
	);
	push_y += 95.f;

	DRAW_TEXT_CENTERED(
		start_x,
		start_y + push_y,
		Shared::Colors::white, "the loader has encountered error"
	);
	push_y += 20.f;

	DRAW_TEXT_CENTERED(
		start_x,
		start_y + push_y,
		Shared::Colors::white, "which it cannot recover from."
	);
	push_y += 20.f;

	DRAW_TEXT_CENTERED(
		start_x,
		start_y + push_y,
		Shared::Colors::white, "please contact hellscythe support."
	);
	push_y += 20.f;
}

void loader::ui::DrawInitializing()
{
	static ULONGLONG finished_tick = 0;

	if (!status_slider.IsValid())
	{
		auto& slider = status_slider;

		slider.SetMode(UI::Slider::Modes::LeftToRight);
		slider.SetRenderer(g::render);
		slider.SetColor(Shared::Colors::light_blue);
		slider.SetCoordinates(NULL, g::window.h - 15.f, g::window.w, 15.f);
	}

	status_slider.Start(0.8f);
	status_slider.Render();

	g::render->draw_text(50.f, g::window.h / 12.f * 10.f, Shared::Colors::white, _XS("HELLSCYTHE"), Shared::dt_align_center);

	if (status_slider.IsFull())
	{
		if (finished_tick == 0)
			finished_tick = GetTickCount64();

		g::render->draw_text(95.f, g::window.h / 12.f * 10.f, Shared::Colors::blue, _XS("-SOFTWARE"));

		g::init_delta = std::abs(static_cast<int>(finished_tick - GetTickCount64()));

		if (g::init_delta > 6000 && g::initialized)
		{
			status_slider = {};
			UpdateState(States::Authorizing);
		}
	}
}

void loader::ui::DrawAuthorizing()
{
	if (!status_slider.IsValid())
	{
		auto& slider = status_slider;

		slider.SetMode(UI::Slider::Modes::LeftToRightRlLoop);
		slider.SetRenderer(g::render);
		slider.SetColor(Shared::Colors::light_blue);
		slider.SetCoordinates(NULL, g::window.h - 15.f, g::window.w, 15.f);
		slider.SetSpeed(1.f);
	}

	status_slider.Render();

	g::render->draw_text(5.f, g::window.h / 12.f * 10.f, 
		Shared::Colors::white, fmt::format(_XS("validating info, mr. {}..."), g::auth_data.user_name));
}

void loader::ui::DrawSelection()
{
	const auto cursor_position = g::input->get_cursor_position();

	const auto render_base = []()
	{
		g::render->draw_square(0.0f, 0.0f, g::window.w - 1.0f, g::window.h - 1.0f, Shared::Colors::black);
		g::render->draw_filled_square(0.0f, 0.0f, g::window.w - 1.0f, 25.0f, Shared::Colors::black);

		static const std::string title = _XS("Hellscythe");

		g::render->draw_text(g::window.w / 2.0f - (g::render->get_text_dimension(title, g::render->m_fonts.m_verdana_small).w / 2.0f),
			5.0f, Shared::Colors::white, title, Shared::dt_outline, g::render->m_fonts.m_verdana_small);
	};

	render_base();

	//
	// Draw Products
	//
	float x_start = 5.f, y_start = 25.f;
	float y_push = 0.f;

	static int m_var_selected = 0;

	if (!helloverlay::IsHypervisorPresent())
	{
		//
		// Our hypervisor is NOT loaded.
		// Only Offer HV branches to load here.
		//
		g::render->draw_text(x_start, y_start + y_push, Shared::Colors::yellow, _XS("missing HSK kernel module!"));
		y_push += 102.f;

		auto elements = std::vector<std::string>
		{
			_XS("hellscythe kernel W1909"),
			_XS("hellscythe kernel BETA W1909"),
			_XS("hellscythe kernel EXP W1909"),
			_XS("hellscythe kernel EXP W1709"),
		};

		UI::c_combo_box dlcs(g::render, &m_var_selected, _XS("available components:"), elements, false);
		dlcs.render({ x_start, y_start + y_push }, { g::window.w - 10.f, 50.0f }, cursor_position);

		if (UI::Render::button(cursor_position, { 5.0f, g::window.h - 50.f }, { g::window.w - 10.f, 20.0f },
			Shared::Colors::black, Shared::Colors::white, _XS("load"), Shared::Colors::white, 6.f))
		{
			uint32_t build_asset_id = 0x00;

			switch (m_var_selected)
			{
			case 0:
				build_asset_id = 0xA08;
				break;
			case 1:
				build_asset_id = 0xA20;
				break;
			case 2:
				build_asset_id = 0xA79;
				break;
			case 3:
				build_asset_id = 0x123F;
				break;
			default:
				break;
			}

			if (build_asset_id == 0x00)
			{
				MESSAGE_BOX("internal error: invalid BA-ID was specified.", MB_ICONERROR);
				return;
			}

			g::m_load_args.vcx = LTT_KernelModule;
			g::m_load_args.vdx = build_asset_id;

			UpdateState(States::Loading);
		}

		if (UI::Render::button(cursor_position, { 5.0f, g::window.h - 25.f }, { g::window.w - 10.f, 20.0f },
			Shared::Colors::black, Shared::Colors::white, _XS("exit"), Shared::Colors::white, 6.f))
		{
			return ExitProcess(0);
		}
	}
	else
	{
		//
		// Our hypervisor is LOADED.
		// Here we can offer regular GDMs and other stuff.
		//
		g::render->draw_text(x_start, y_start + y_push, Shared::Colors::green, _XS("found valid HSK kernel module!"));
		y_push += 55.f;

		auto elements = std::vector<std::string>();

		for (const auto& entry : g::product_def_list[_XSS("game_dependant_modules")])
		{
			std::string name = entry["name"];

			elements.push_back(name);
		}

		UI::c_combo_box dlcs(g::render, &m_var_selected, _XS("available gdm's:"), elements, false);
		dlcs.render({ x_start, y_start + y_push }, { g::window.w - 10.f, 50.0f }, cursor_position);

		if (UI::Render::button(cursor_position, { 5.0f, g::window.h - 50.f }, { g::window.w - 10.f, 20.0f },
			Shared::Colors::black, Shared::Colors::white, _XS("load"), Shared::Colors::white, 6.f))
		{
			uint32_t gdm_asset_id = 0x00;

			for (const auto& entry : g::product_def_list[_XSS("game_dependant_modules")])
			{
				std::string name = entry["name"];
				uint32_t asset_id = entry["asset_id"];

				if (name == elements[m_var_selected])
				{
					gdm_asset_id = asset_id;
					break;
				}
			}

			if (gdm_asset_id == 0x00)
			{
				MESSAGE_BOX("internal error: invalid GA-ID was specified.", MB_ICONERROR);
				return;
			}

			g::m_load_args.vcx = LTT_GameDependantModule;
			g::m_load_args.vdx = gdm_asset_id;

			UpdateState(States::Loading);
		}

		if (UI::Render::button(cursor_position, { 5.0f, g::window.h - 25.f }, { g::window.w - 10.f, 20.0f },
			Shared::Colors::black, Shared::Colors::white, _XS("exit"), Shared::Colors::white, 6.f))
		{
			return ExitProcess(0);
		}

		{
			//
			// Options
			//
			const auto _x_start = 5.f, _y_start = g::window.h - 125.f;
			const auto _x_render_start = _x_start + 5.f, _y_render_start = _y_start + 5.f;
			const auto _x_end = (_x_start + (g::window.w - 10.f)), _y_end = (_y_start + 50.f);
			auto _y_push = 0.f;

			g::render->draw_filled_square(_x_start, _y_start + _y_push, 
				_x_end, _y_end, D3DCOLOR_ARGB(255, 15, 15, 15));

			UI::elements::check_box({ _x_render_start, _y_render_start + _y_push }, 
				_XS("dbg console"), &options::gdm_load.m_debug_console);
			_y_push += 20.f;

			UI::elements::check_box({ _x_render_start, _y_render_start + _y_push },
				_XS("in-game overlay"), & options::gdm_load.m_enable_overlay);
			_y_push += 20.f;
		}
	}
}

void loader::ui::DrawLoadingPrompt()
{
	if (!status_slider.IsValid())
	{
		auto& slider = status_slider;

		slider.SetMode(UI::Slider::Modes::LeftToRightRlLoop);
		slider.SetRenderer(g::render);
		slider.SetColor(Shared::Colors::light_blue);
		slider.SetCoordinates(NULL, g::window.h - 15.f, g::window.w, 15.f);
		slider.SetSpeed(0.f);
	}

	status_slider.Render();

	DRAW_TEXT_CENTERED(
		g::window.w / 2.0f,
		g::window.h / 2.f,
		Shared::Colors::whacking_dusty_blue, "cleaning up and exiting now."
	);

	DRAW_TEXT_CENTERED(
		g::window.w / 2.0f,
		g::window.h / 2.f + 20.f,
		Shared::Colors::whacking_dusty_blue, "press CTRL+LSHIFT to load."
	);

	DRAW_TEXT_CENTERED(
		g::window.w / 2.0f,
		g::window.h / 2.f + 60.f,
		Shared::Colors::yellow, "wait until the loader has closed!"
	);

	static bool sent_clean_up_request = false;

	if (!sent_clean_up_request)
	{
		if (UI::Render::button(g::input->get_cursor_position(), { g::window.w / 2.f - 50.f, g::window.h - 50.f }, { 100.0f, 20.0f },
			Shared::Colors::black, Shared::Colors::white, _XS("load"), Shared::Colors::white, 6.f))
		{
			std::thread(ExitSafe).detach();

			auto& slider = status_slider;

			slider.SetMode(UI::Slider::Modes::LeftToRightRlLoop);
			slider.SetRenderer(g::render);
			slider.SetColor(Shared::Colors::light_blue);
			slider.SetCoordinates(NULL, g::window.h - 15.f, g::window.w, 15.f);
			slider.SetSpeed(3.2f);

			sent_clean_up_request = true;
		}
	}
}

void loader::ui::DrawLoading()
{
	if (!status_slider.IsValid())
	{
		auto& slider = status_slider;

		slider.SetMode(UI::Slider::Modes::LeftToRightRlLoop);
		slider.SetRenderer(g::render);
		slider.SetColor(Shared::Colors::light_blue);
		slider.SetCoordinates(NULL, g::window.h - 15.f, g::window.w, 15.f);
		slider.SetSpeed(1.f);
	}

	status_slider.Render();

	const auto& load_status = g::m_load_args.v8;

	switch (load_status)
	{
	case LS_Initializing:
		g::render->draw_text(5.f, g::window.h / 12.f * 10.f,
			Shared::Colors::white, fmt::format(_XS("initializing this procedure..."), g::auth_data.user_name));
		break;
	case LS_Downloading:
		g::render->draw_text(5.f, g::window.h / 12.f * 10.f,
			Shared::Colors::white, fmt::format(_XS("downloading your product..."), g::auth_data.user_name));
		break;
	case LS_Waiting:
		status_slider = {};
		UpdateState(States::LoadingPrompt);
		break;
	default:
		break;
	}
}

bool loader::ui::Destroy()
{
	g::rendering = false;
	g::direct_device->Release();

	return DestroyWindow(g::window_handle);
}

void loader::ui::ExitSafe()
{
	Sleep(7240);
	UpdateState(States::Success);

	//
	// unblock helloverlay's loading of GDMs now.
	//
	g::m_load_args.v9 = 0;
}

void loader::ui::Render()
{
	logic::Think();

	DrawBase();

	switch (g::state)
	{
	case States::Error:
		DrawError();
		break;
	case States::Initializing:
		DrawInitializing();
		break;
	case States::Authorizing:
		DrawAuthorizing();
		break;
	case States::Selection:
		DrawSelection();
		break;
	case States::LoadingPrompt:
		DrawLoadingPrompt();
		break;
	case States::Loading:
		DrawLoading();
		break;
	case States::Success:
		Destroy();
		break;
	}
}
