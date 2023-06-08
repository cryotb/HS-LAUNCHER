#pragma once

namespace integrity
{
	struct violation_info_t
	{
		bool m_severe;
		uint32_t m_id;
	};

	namespace _
	{
		inline bool severe_violation_present{};
		inline std::vector<violation_info_t> violation_queue{};
	}

	extern void ReportViolation(const violation_info_t& info);

	extern bool HideThreadFromDebugger(HANDLE hThread);
	extern bool VerifyImageIntegrity(const std::string& module_name, const std::string& fod_path);

	extern uint32_t CalculateImageCodeHash(HMODULE hModule);

	extern bool CheckForDebugger();
	extern bool CheckForPatches();

	extern bool Initialize();
	extern u_long Think(PVOID pArgs);
}
