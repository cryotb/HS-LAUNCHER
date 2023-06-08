#pragma once

namespace UI
{
	namespace Slider
	{
		enum class Modes
		{
			Invalid = -1,
			NotSet = 0,

			LeftToRight,
			LeftToRightRlLoop,
		};
	}

	class C_Slider
	{
	public:
		C_Slider() = default;
		~C_Slider() = default;

		CONST AUTO GetProgress() { return m_Meta.slider_progress; }
		CONST AUTO IsFull() { return m_Meta.slider_progress >= m_Meta.slider_w; }
		CONST AUTO IsRunning() { return m_Meta.started; }
		CONST AUTO IsValid() { return !(m_Render == nullptr); }

		C_Slider(Shared::c_render9* renderer, INT posX, INT posY, INT width, INT height, Slider::Modes Mode, D3DCOLOR color = Shared::Colors::white, BOOLEAN ResetOnFinish = FALSE)
		{
			m_Render = renderer;

			m_Position = { posX, posY };
			m_Dimension = { width, height };

			m_Meta.reset_on_finish = ResetOnFinish;

			m_Meta.mode = Mode;

			m_Meta.slider_x = posX;
			m_Meta.slider_y = posY;
			m_Meta.slider_w = width;
			m_Meta.slider_h = height;

			m_Meta.slider_end_x = m_Meta.slider_x + m_Meta.slider_w;
			m_Meta.slider_end_y = m_Meta.slider_y + m_Meta.slider_h;

			m_Meta.color = color;
		}

		VOID Start(float speed)
		{
			m_Meta.slider_speed = speed;

			m_Meta.started = true;
		}

		VOID Stop()
		{
			m_Meta.started = false;
		}

		VOID SetRenderer(Shared::c_render9* renderer)
		{
			m_Render = renderer;
		}

		VOID SetMode(Slider::Modes mode)
		{
			m_Meta.mode = mode;
		}

		VOID SetCoordinates(INT x, INT y, INT w, INT h)
		{
			m_Position = { x, y };
			m_Dimension = { w, h };

			m_Meta.slider_x = x;
			m_Meta.slider_y = y;
			m_Meta.slider_w = w;
			m_Meta.slider_h = h;

			m_Meta.slider_end_x = m_Meta.slider_x + m_Meta.slider_w;
			m_Meta.slider_end_y = m_Meta.slider_y + m_Meta.slider_h;
		}

		VOID SetColor(D3DCOLOR color)
		{
			m_Meta.color = color;
		}

		VOID SetSpeed(FLOAT speed)
		{
			m_Meta.slider_speed = speed;
		}

		VOID MoveTo(FLOAT value)
		{
			// todo: maybe check if it's a looping one instead of this hard-coded thingy.
			if (m_Meta.mode != Slider::Modes::LeftToRight)
				return;

			m_Meta.move_to = true;
			m_Meta.slider_move_to_value = value;

			if (value > m_Meta.slider_progress)
				m_Meta.move_to_type = 1; // higher than current
			else if (value < m_Meta.slider_progress)
				m_Meta.move_to_type = 2; // lower than current
			else
				m_Meta.move_to_type = -1;
		}

		VOID Reset()
		{
			m_Meta = {};
		}

		VOID Render()
		{
			const auto HandleLeftToRight = [this]()
			{
				const auto render_color = m_Meta.color;

				m_Render->draw_filled_square(m_Meta.slider_x, m_Meta.slider_y, m_Meta.slider_end_x, m_Meta.slider_end_y, D3DCOLOR_ARGB(255, 15, 15, 15));
				m_Render->draw_filled_square(m_Meta.slider_x, m_Meta.slider_y, m_Meta.slider_x + m_Meta.slider_progress, m_Meta.slider_end_y, render_color);

				if (m_Meta.move_to)
				{
					if (m_Meta.move_to_type == 1)
					{
						if (m_Meta.slider_progress < m_Meta.slider_move_to_value)
							m_Meta.slider_progress += m_Meta.slider_speed;
						else {
							m_Meta.move_to = false;
							m_Meta.slider_move_to_value = 0.f;
						}
					}
					else if (m_Meta.move_to_type == 2)
					{
						if (m_Meta.slider_progress > m_Meta.slider_move_to_value)
							m_Meta.slider_progress -= m_Meta.slider_speed;
						else {
							m_Meta.move_to = false;
							m_Meta.slider_move_to_value = 0.f;
						}
					}
				}
				else if (m_Meta.started)
				{
					if (m_Meta.slider_progress < m_Meta.slider_w)
						m_Meta.slider_progress += m_Meta.slider_speed;
					else
						if (m_Meta.reset_on_finish)
							Reset();
				}
			};

			const auto HandleLeftRightRlLoop = [this]()
			{
				auto& sequence = m_Meta.sequence;
				auto& slider_progress = m_Meta.slider_progress;

				const auto slider_x = m_Meta.slider_x, slider_y = m_Meta.slider_y,
					slider_w = m_Meta.slider_w, slider_h = m_Meta.slider_h;

				const auto slider_end_x = m_Meta.slider_end_x, slider_end_y = m_Meta.slider_end_y;

				const auto slider_speed = m_Meta.slider_speed;

				m_Render->draw_filled_square(m_Meta.slider_x, m_Meta.slider_y, m_Meta.slider_end_x, m_Meta.slider_end_y, D3DCOLOR_ARGB(255, 15, 15, 15));

				if (sequence == 0)
				{
					const auto seed = std::max(150, static_cast<int>(slider_progress)) * 255 / 255;
					const auto render_color = D3DCOLOR_ARGB(255, seed / 2, seed, seed);

					m_Render->draw_filled_square(slider_x, slider_y, slider_x + slider_progress, slider_end_y, render_color);

					if (slider_progress < slider_w)
					{
						slider_progress += slider_speed;
					}
					else
					{
						slider_progress = 0.0f;
						sequence = 1;
					}
				}
				else if (sequence == 1) // left <-- right
				{
					const auto seed = std::max(150, static_cast<int>(slider_progress)) * 255 / 255;
					const auto render_color = D3DCOLOR_ARGB(255, seed, seed / 2, seed);

					m_Render->draw_filled_square(slider_x + slider_progress, slider_y, slider_end_x, slider_end_y, render_color);

					if (slider_progress < slider_w)
					{
						slider_progress += slider_speed;
					}
					else
					{
						slider_progress = 0.0f;
						sequence = 0;
					}
				}
			};

			switch (m_Meta.mode)
			{
			case Slider::Modes::LeftToRight:
				HandleLeftToRight();
				break;
			case Slider::Modes::LeftToRightRlLoop:
				HandleLeftRightRlLoop();
				break;
			default:
				break;
			}
		}
	private:
		struct Meta_t
		{
			int sequence{};
			int move_to_type{};

			bool started{};
			bool reset_on_finish{};
			bool move_to{};

			float slider_speed{};
			float slider_progress{};
			float slider_w{}, slider_h{};
			float slider_move_to_value{};

			float slider_x{}, slider_y{};
			float slider_end_x{}, slider_end_y{};

			Slider::Modes mode{};
			D3DCOLOR color{};
		};
	private:
		Math::position_t<INT> m_Position{};
		Math::dimension_t<INT> m_Dimension{};
		Meta_t m_Meta{};
		Shared::c_render9* m_Render{};
	};
}
