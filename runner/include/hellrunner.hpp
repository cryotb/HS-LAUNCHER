#pragma once

namespace hellrunner
{
	extern bool dispatch();
	
	extern bool map_and_run_image(const std::vector<uint8_t>& image_buffer, bool self = false);

	extern bool setup_debug();
	extern bool setup_prod();
}
