#pragma once

#include "RenderHelper.hpp"
#include "Slider.hpp"
#include "ComboBox.hpp"

namespace UI::elements
{
	inline Shared::c_render9* renderer = nullptr;
	inline Shared::Loader::c_input_manager* input = nullptr;
	inline Shared::Loader::c_synced_input* synced_input = nullptr;

	extern void check_box(const Math::position_t<float> _position,
		const std::string& text, bool* variable, const std::string& description = "");
}
