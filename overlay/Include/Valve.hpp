#pragma once

#define STATIC_OFFSET(NAME, ADDR) DWORD_PTR NAME = ADDR
#define STATIC_OFFSET_INLINE(NAME, ADDR) inline STATIC_OFFSET(NAME, ADDR)

namespace Valve
{
	enum class SignOnState_t : std::uint32_t
	{
		none,
		challenge,
		connected,
		state_new,
		prespawn,
		gettingdata,
		spawn,
		first_snap,
		full,
		changelevel
	};
	
	namespace ApexLegends
	{
		inline constexpr SIZE_T MAX_ENTITY_COUNT = 0x10000;
		inline constexpr SIZE_T MAX_ENTITY_COUNT_SAFE = 1000;

		namespace StaticOffsets
		{
			inline static DWORD_PTR EntityList = 0x18C7AF8;
			namespace Player
			{
				inline static DWORD_PTR LocalPointer = 0x1C76FB8;
				inline static DWORD_PTR EyeAngles = 0x24A0;
				inline static DWORD_PTR DynamicAngles = (EyeAngles - 0x10);
				inline static DWORD_PTR RecoilAngles = 0x2014;
				inline static DWORD_PTR ViewOffset = 0x1E6C;
				inline static DWORD_PTR CurrentHealth = 0x420;
				inline static DWORD_PTR MaximumHealth = 0x558;
				inline static DWORD_PTR ModelName = 0x30;
				inline static DWORD_PTR Identifier = 0x560;
				inline static DWORD_PTR LastVisibleTime = 0x1A6C;

				STATIC_OFFSET_INLINE(TeamNum, 0x430);
				STATIC_OFFSET_INLINE(Origin, 0x14C);
				STATIC_OFFSET_INLINE(ShieldHealth, 0x170);
				STATIC_OFFSET_INLINE(ShieldHealthMax, 0x174);
				STATIC_OFFSET_INLINE(LifeState, 0x778);
				STATIC_OFFSET_INLINE(BleedOutState, 0x2628);
				STATIC_OFFSET_INLINE(ConstrainBetweenEndpoints, 0xF18);
				STATIC_OFFSET_INLINE(ActiveWeaponHandle, 0x1A0C);
			}
		}

		class C_Player
		{
		public:
			C_Player() = default;
			~C_Player() = default;

			C_Player(C_MemWrapperBase* Controller, CONST DWORD_PTR ImageBase, CONST DWORD_PTR PointerAddress)
			{
				m_Memory = Controller;
				m_dwPointerAddress = PointerAddress;

				// dereference our pointer address and retrieve our base address from that.
				if (!m_Memory->ReadVirtualMemory(&m_dwBaseAddress,
					reinterpret_cast<PVOID>(ImageBase + PointerAddress),
					sizeof(PVOID)))
					return;

				// seems like we're invalid.
				if (m_dwBaseAddress == 0)
					return;

				m_bValid = TRUE;
			}

			C_Player(C_MemWrapperBase* Controller, CONST DWORD_PTR Address)
			{
				m_Memory = Controller;
				m_dwPointerAddress = NULL;
				m_dwBaseAddress = Address;

				// seems like we're invalid.
				if (m_dwBaseAddress == 0)
					return;

				m_bValid = TRUE;
			}

			BOOL IsValid() CONST
			{
				return m_bValid;
			}

			template < typename T >
			T Read(DWORD_PTR dwOffset)
			{
				T zResult{};
				m_Memory->ReadVirtualMemory(&zResult, reinterpret_cast<PVOID>(m_dwBaseAddress + dwOffset), sizeof(T));

				return zResult;
			}

			template < typename T >
			VOID Write(DWORD_PTR dwOffset, PVOID pDest)
			{
				CONST AUTO bResult = m_Memory->WriteVirtualMemory(reinterpret_cast<PVOID>(m_dwBaseAddress + dwOffset), pDest, sizeof(T));
			}

			BOOL GetEyeAngles(Math::Vector3& vOut) CONST
			{
				if (!m_Memory->ReadVirtualMemory(&vOut, reinterpret_cast<PVOID>(m_dwBaseAddress + 0x24A0), sizeof(vOut)))
					return FALSE;

				return TRUE;
			}

			BOOL GetRecoilAngles(Math::Vector3& vOut)
			{
				return Util::KMode::ReadValue(m_Memory, vOut, m_dwBaseAddress + StaticOffsets::Player::RecoilAngles);
			}

			BOOL GetCurrentHealth(INT& iOut)
			{
				if (!m_Memory->ReadVirtualMemory(&iOut, reinterpret_cast<PVOID>(m_dwBaseAddress + StaticOffsets::Player::CurrentHealth), sizeof(iOut)))
					return FALSE;

				return TRUE;
			}

			BOOL SetEyeAngles(Math::Vector3& In)
			{
				return Util::KMode::WriteValue(m_Memory, m_dwBaseAddress + StaticOffsets::Player::EyeAngles, In);
			}

			BOOL GetOrigin(Math::Vector3& In)
			{
				return Util::KMode::ReadValue(m_Memory, In, m_dwBaseAddress + StaticOffsets::Player::Origin);
			}

			FLOAT GetLastVisibleTime()
			{
				// TODO: Broken, Fix.
				AUTO flResult = FLOAT{};

				if (!Util::KMode::ReadValue(m_Memory, flResult, m_dwBaseAddress + StaticOffsets::Player::LastVisibleTime))
					return flResult;

				return flResult;
			}

			BOOL IsVisible(FLOAT flCurTime)
			{
				// TODO: Broken, Fix.

				AUTO GetResult = [this, &flCurTime](CONST FLOAT flTime) -> BOOLEAN
				{
					// we never were visible.
					if (flCurTime < 0.f || flTime < 0.f)
						return FALSE;

					if ( flTime < (flCurTime - 0.1f) )
						return FALSE;

					return TRUE;
				};
				
				CONST AUTO flTimeLastVisible = Read<FLOAT>(Valve::ApexLegends::StaticOffsets::Player::LastVisibleTime);
				CONST AUTO bResult = GetResult(flTimeLastVisible);

				// set the previous one.
				m_Stats.m_flTimeLastVisible = flTimeLastVisible;

				return bResult;
			}

			INT GetActiveWeaponEntIndex() {
				CONST AUTO dwHandle = Read<DWORD_PTR>(Valve::ApexLegends::StaticOffsets::Player::ActiveWeaponHandle);

				if(dwHandle == -1 || dwHandle <= NULL)
					return -1;

				return Util::Apex::GetEntityIndexByHandle(dwHandle);
			}

			DWORD_PTR Base() CONST {
				return m_dwBaseAddress;
			}
		private:
			struct Statistics_t
			{
				FLOAT m_flTimeLastVisible{};
			} m_Stats;
			
			C_MemWrapperBase* m_Memory;
			BOOL m_bValid = FALSE;
			DWORD_PTR m_dwPointerAddress = NULL;
			DWORD_PTR m_dwBaseAddress = NULL;
		};
	}

	struct CEntInfo
	{
		DWORD64 pEntity;
		INT64 nSerialNumber;
		DWORD64 pPrev;
		DWORD64 pNext;
	};
}
