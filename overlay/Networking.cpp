#include <Include.hpp>

bool networking::Initialize()
{
	if (!_::client.start(_XS("hellscythe.xyz"), _XS("823")))
		return false;

	G::NetClient = _::client;
	loader::g::net_client = _::client;

	std::thread(Maintain).detach();

	return true;
}

void networking::Shutdown()
{
	_::client.stop();
}

void networking::Maintain()
{
	VMP_BEGIN_ULTRA("NETWORKING_Maintain");

	const auto check_alive = []()
	{
		protocol::packet::header_t header{};
		std::string message{};

		if (!protocol::helper::write_message(_::client.get_socket(), _XS("KeepAlive")))
			return false;

		if (!protocol::helper::read_message(_::client.get_socket(), message, header))
			return false;

		return ( message == _XS("OK") );
	};

	do
	{
		LOG_DEBUG("networking=  waiting to be ready.");

		std::this_thread::sleep_for(5s);
	} while (!_::ready);

	do
	{
		if (check_alive())
		{
			LOG_DEBUG("networking =  alive check has passed.");
		}
		else
		{
			_::lost = true;
			break;
		}

		std::this_thread::sleep_for(60s);
	} while (_::client.get_active());

	Shutdown();

	VMP_END();
}
