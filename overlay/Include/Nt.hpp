#pragma once

#define NtCurrentThread ((HANDLE)-2)

namespace nt
{
	template < typename T >
	inline T FindExport(const std::string& name)
	{
		auto* const h_module = GetModuleHandle(_XS("ntdll.dll"));

		if (h_module == nullptr)
			return T{};

		return (T)GetProcAddress(h_module, name.c_str());
	}

	inline NTSTATUS SetInfoThread(
		HANDLE          ThreadHandle, 
		THREADINFOCLASS ThreadInformationClass,
		PVOID           ThreadInformation,
		ULONG           ThreadInformationLength
	)
	{
		auto* const function = 
			FindExport<decltype(SetInfoThread)*>
			("NtSetInformationThread");

		return function(
			ThreadHandle,
			ThreadInformationClass,
			ThreadInformation,
			ThreadInformationLength
		);
	}

	inline NTSTATUS QueryInfoThread(
		HANDLE          ThreadHandle,
		THREADINFOCLASS ThreadInformationClass,
		PVOID           ThreadInformation,
		ULONG           ThreadInformationLength,
		PULONG          ReturnLength
	)
	{
		auto* const function =
			FindExport<decltype(QueryInfoThread)*>
			("NtQueryInformationThread");

		return function(
			ThreadHandle,
			ThreadInformationClass,
			ThreadInformation,
			ThreadInformationLength,
			ReturnLength
		);
	}
}
