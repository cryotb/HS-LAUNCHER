#pragma once

namespace helloverlay
{
	constexpr std::uint8_t UD_Magic = 0xE6;

	struct UserData_t
	{
		std::uint8_t Magic{}; // used for verifying we're accessing correctly formatted data.
		std::uint8_t CryptKey{}; // used to decrypt text based fields in here.
		std::uint64_t TickCreated{}; // determines the tick the config was GENERATED at.
		bool TermsAccepted{}; // did the user accept our tos?
		bool WasAuthorized{}; // did the user have a history of at least one successful login?
		char LastUserName[256] = { 0 }; // if so, save the username here.
		char LastPassWord[256] = { 0 }; // do the same with the password.
	};

	extern BOOLEAN ClearTraces();
	extern BOOLEAN ClearHeaders();
	extern BOOLEAN CollectSessionInfo(uintptr_t sock);

	extern BOOLEAN MakeNewUserData();
	extern BOOLEAN ReadAlreadyPresentUserData();
	extern BOOLEAN IsHypervisorPresent();

	//
	// User Data API V1
	//
	extern BOOLEAN GetUserDataPresent();
	extern BOOLEAN GetUserDataContents(void* out_buffer, size_t out_length);

	extern BOOLEAN PerformInitialSetup_Debug(Games::C_GameBase** pGameHandler);
	extern BOOLEAN PerformInitialSetup_Prod(Games::C_GameBase** pGameHandler, uint32_t gdm_asset_id = 0x00);
	extern BOOLEAN CheckStartupIntegrity();
	extern BOOLEAN Authorize();
	extern BOOLEAN Run();
	extern BOOLEAN RunHeadless(uint64_t gdm_asset_id);
}
