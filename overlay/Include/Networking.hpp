#pragma once

namespace networking
{
	namespace _
	{
		inline bool lost{};
		inline bool ready{};
		inline net::c_client client{};
	}

	inline bool Lost() { return _::lost; }
	inline bool Active() { return _::client.get_active(); }
	inline uintptr_t Socket() { return _::client.get_socket(); }

	extern bool Initialize();
	extern void Shutdown();

	extern void Maintain();
}
