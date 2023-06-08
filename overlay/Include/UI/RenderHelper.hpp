#pragma once

namespace UI::Render
{
	inline Shared::c_render9* renderer = nullptr;
	inline Shared::Loader::c_synced_input* m_synced_input = nullptr;

	inline bool is_in_range(const Math::position_t<int> cursor_position, const Math::position_t<float> _position,
		const Math::dimension_t<float> _dimension)
	{
		if (cursor_position.x < _position.x || cursor_position.x > _position.x + _dimension.w)
			return false;

		if (cursor_position.y < _position.y || cursor_position.y > _position.y + _dimension.h)
			return false;

		return true;
	}

	inline bool button(const Math::position_t<int> cursor_position, const Math::position_t<float> _position,
		const Math::dimension_t<float> _dimension, const D3DCOLOR& background_color,
		const D3DCOLOR& text_color, const std::string& text, const D3DCOLOR& outline_color, const float& text_height_divisor)
	{
		renderer->draw_filled_square(_position.x, _position.y, _position.x + _dimension.w, _position.y + _dimension.h,
			background_color);
		renderer->draw_square(_position.x, _position.y, _position.x + _dimension.w, _position.y + _dimension.h,
			outline_color);
		renderer->draw_text(_position.x + _dimension.w / 2.0f - renderer->get_text_dimension(text).w / 2.0f,
			_position.y + _dimension.h / text_height_divisor, text_color, text);

		if (!is_in_range(cursor_position, _position, _dimension))
			return false;

		const bool is_clicked = m_synced_input->was_button_down(VK_LBUTTON);

		return is_clicked;
	}
}
