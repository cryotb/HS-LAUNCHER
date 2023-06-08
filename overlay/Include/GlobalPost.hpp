#pragma once

namespace G::P
{
	inline BOOLEAN Unload = FALSE;
	inline Math::dimension_t<INT> Screen{};
	inline C_OverlayCore* Core = nullptr;
	inline Shared::c_render9* Render = nullptr;
	inline Games::C_GameBase* CurrentGameHandler = nullptr;
	inline mem::manager* Memory = nullptr;
	inline Shared::c_input_manager* Input = nullptr;
	inline shared::c_window_manager* Windows = nullptr;
	inline shared::config::c_manager* Config = nullptr;

	namespace SI
	{
		inline Shared::c_synced_input* Present = nullptr;
	}
}

#define CONF_INT(X) G::P::Config->get_integer(_XS( X ))
#define CONF_BOOL(X) G::P::Config->get_boolean(_XS( X ))
#define CONF_FLOAT(X) G::P::Config->get_float(_XS( X ))
#define CONF_STRING(X) G::P::Config->get_string(_XS( X ))
#define CONF_FIELD(NAME, TYPE, DEF) G::P::Config->add_field(_XS(NAME), TYPE, DEF)
