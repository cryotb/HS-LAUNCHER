#pragma once

namespace UI
{
	class c_combo_box
	{
	public:
		c_combo_box() = default;
		c_combo_box(Shared::c_render9* renderer, int* variable, const std::string& name, const std::vector<std::string>& elements, const bool hide = true)
		{
			m_renderer = renderer;
			m_variable = variable;
			m_name = name;

			m_hide = hide;
			m_visible = false;
			m_elements = elements;
		}

		void render(const Math::position_t<float> position, const Math::dimension_t<float> dimension, Math::position_t<int> cursor_position)
		{
			m_position = position;
			m_dimension = dimension;

			int yaw_index = 0;
			float push_y = 0.0f;
			const float start_x = m_position.x, start_y = m_position.y;

			m_renderer->draw_text(start_x + 2.0f, start_y, Shared::Colors::white, m_name, 2, m_renderer->m_fonts.m_verdana_small);
			push_y += 19.0f;

			for (const auto& element : m_elements)
			{
				if (Render::button(cursor_position, { m_position.x, m_position.y + push_y }, { m_dimension.w, 17.0f }, 
					*m_variable == yaw_index ? Shared::Colors::blue : D3DCOLOR_ARGB(255, 26, 26, 26), 
					Shared::Colors::white, element, Shared::Colors::black, 6.f))
				{
					if (!m_hide || (m_hide && m_visible))
					{
						*m_variable = yaw_index;
					}

					m_visible = true;
				}

				if (!m_visible && m_hide)
				{
					Render::button(cursor_position, { m_position.x, m_position.y + push_y }, { m_dimension.w, 17.0f }, 
						Shared::Colors::blue, Shared::Colors::white, m_elements[*m_variable], Shared::Colors::black, 6.f);
					push_y += 17.0f;

					break;
				}

				++yaw_index;
				push_y += 17.0f;
			}

			if (!Render::is_in_range(cursor_position, { m_position.x, m_position.y }, { m_dimension.w, push_y }))
			{
				m_visible = false;
			}
		}
	private:
		int* m_variable;
		bool m_hide;
		bool m_visible;
		Shared::c_render9* m_renderer;
		std::string m_name{};
		std::vector<std::string> m_elements{};
		Math::position_t<float> m_position{};
		Math::dimension_t<float> m_dimension{};
	};
}
