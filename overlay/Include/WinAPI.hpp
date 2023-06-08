#pragma once

// HIDDEN_API CALL
#define call_hidden_api(TYPE, MOD, FNAME, ...) winapi::dynamic::hidden_api_call<TYPE>(_XS(MOD), _XS(FNAME), __VA_ARGS__)

namespace shared::winapi::dynamic {
	template < typename R, typename ... A>
	inline R hidden_api_call(const std::string& module_name, const std::string& function_name, A&&... arguments) {
		typedef R(WINAPI* hiddenApiFn)(A ...);
		
		auto h_owner_module = HMODULE{};

		h_owner_module = GetModuleHandleA(module_name.c_str());

		if (h_owner_module == nullptr) {
			return R();
		}

		const auto function_addr = GetProcAddress(h_owner_module, function_name.c_str());

		if (function_addr == 0) {
			return R();
		}

		return (reinterpret_cast<hiddenApiFn>(function_addr))(arguments ...);
	}
}

namespace shared::winapi {

}
