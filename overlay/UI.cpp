#include <Include.hpp>

void UI::elements::check_box(const Math::position_t<float> _position,
	const std::string& text, bool* variable, const std::string& description)
{
	if (variable == nullptr)
		return;

	const float text_width =
		static_cast<float>(renderer->get_text_dimension(text).w) + 15.0f;

	const auto box_color =
		*variable ? D3DCOLOR_ARGB(255, 47, 146, 245) : Shared::Colors::black;

	const float checkbox_center_x = (std::roundf(_position.x) + (10.0f / 2.0f)),
		checkbox_center_y = (std::roundf(_position.y) + (10.0f / 2.0f));

	renderer->draw_filled_square(
		checkbox_center_x - 3.0f, checkbox_center_y - 3.0f,
		checkbox_center_x + 4.0f, checkbox_center_y + 4.0f, box_color);

	renderer->draw_square(checkbox_center_x - 5.0f, checkbox_center_y - 5.0f,
		checkbox_center_x + 5.0f, checkbox_center_y + 5.0f,
		D3DCOLOR_ARGB(255, 18, 60, 99));

	renderer->draw_text(_position.x + 15.0f, _position.y - 2.0f,
		D3DCOLOR_ARGB(255, 255, 255, 255), text);

	if (!Render::is_in_range(input->get_cursor_position(), _position, { text_width, 15.0f }))
		return;

	if (synced_input->was_button_down(VK_LBUTTON))
		*variable = !*variable;
}
