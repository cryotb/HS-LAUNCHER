#pragma once

namespace Util
{
	namespace KMode
	{
		template < typename T >
		inline BOOLEAN ReadValue(C_MemWrapperBase* Controller, T& Destination, DWORD_PTR Source)
		{
			return Controller->ReadVirtualMemory(reinterpret_cast<PVOID>(&Destination), reinterpret_cast<PVOID>(Source), sizeof(T));
		}

		template < typename T >
		inline BOOLEAN WriteValue(C_MemWrapperBase* Controller, DWORD_PTR Destination, T& Source)
		{
			return Controller->WriteVirtualMemory(reinterpret_cast<PVOID>(Destination), reinterpret_cast<PVOID>(&Source), sizeof(T));
		}

		template < typename T >
		inline BOOLEAN ReadArray(C_MemWrapperBase* Controller, T* Destination, DWORD_PTR Source, SIZE_T Count, BOOLEAN InChunks = FALSE, INT Frac = 16, BOOLEAN Safe = FALSE)
		{
			if (Count <= NULL)
				return FALSE;

			if(Safe)
				return Controller->ReadVirtualMemoryEx(Destination, reinterpret_cast<PVOID>(Source), Count * sizeof(*Destination), TRUE);

			if (InChunks == FALSE)
				return Controller->ReadVirtualMemory(Destination, reinterpret_cast<PVOID>(Source), Count * sizeof(*Destination));

			CONST AUTO MemorySize = ( Count * sizeof( (*Destination) ) );
			CONST AUTO FracTimes = Frac;
			CONST AUTO SizeFrac = (MemorySize / FracTimes);

			SIZE_T CyclesDone = NULL;
			SIZE_T BytesLeft = MemorySize;
			SIZE_T BytesWritten = NULL;

			for (AUTO nIndex = 0; nIndex < FracTimes; nIndex++) {
				AUTO* pSource = mem::addr(Source).Add(BytesWritten).As<PVOID>();
				AUTO* pDest = mem::addr(Destination).Add(BytesWritten).As<PVOID>();

				if (!Controller->ReadVirtualMemory(pDest, pSource, SizeFrac))
					break;

				BytesLeft -= SizeFrac;
 				BytesWritten += SizeFrac;

				++CyclesDone;
			}

			if (BytesWritten < Count || BytesLeft > NULL)
				return FALSE;

			if (CyclesDone < FracTimes)
				return FALSE;

			return TRUE;
		}

		template < typename T >
		inline BOOLEAN WriteArray(C_MemWrapperBase* Controller, T* Destination, DWORD_PTR Source, SIZE_T Count)
		{
			return Controller->WriteVirtualMemory(Destination, reinterpret_cast<PVOID>(Source), Count * sizeof(*Destination));
		}
		
		inline BOOLEAN ReadData(C_MemWrapperBase* Controller, PVOID Destination, DWORD_PTR Source, SIZE_T Count)
		{
			return Controller->ReadVirtualMemory(Destination, reinterpret_cast<PVOID>(Source), Count);
		}

		inline BOOLEAN WriteData(C_MemWrapperBase* Controller, PVOID Destination, DWORD_PTR Source, SIZE_T Count)
		{
			return Controller->WriteVirtualMemory(Destination, reinterpret_cast<PVOID>(Source), Count);
		}
	}

	namespace Render
	{
		inline bool is_in_range(const Math::position_t<int> cursor_position, const Math::position_t<float> _position,
			const Math::dimension_t<float> _dimension)
		{
			if (cursor_position.x < _position.x || cursor_position.x > _position.x + _dimension.w)
			{
				return false;
			}

			if (cursor_position.y < _position.y || cursor_position.y > _position.y + _dimension.h)
			{
				return false;
			}

			return true;
		}

		inline bool button(const Math::position_t<int> cursor_position, const Math::position_t<float> _position,
			const Math::dimension_t<float> _dimension, const D3DCOLOR& background_color,
			const D3DCOLOR& text_color, const std::string& text, const D3DCOLOR& outline_color = Shared::Colors::white, const float& text_height_divisor = 6.f)
		{
			const auto renderer = G::P::Render;

			renderer->draw_filled_square(_position.x, _position.y, _position.x + _dimension.w, _position.y + _dimension.h,
				background_color);
			renderer->draw_square(_position.x, _position.y, _position.x + _dimension.w, _position.y + _dimension.h,
				outline_color);
			renderer->draw_text(_position.x + _dimension.w / 2.0f - renderer->get_text_dimension(text).w / 2.0f,
				_position.y + _dimension.h / text_height_divisor, text_color, text);

			if (!is_in_range(cursor_position, _position, _dimension))
			{
				return false;
			}

			const bool is_clicked = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

			if(is_clicked)
			{
				Sleep(500);
			}

			return is_clicked;
		}
	}
}
