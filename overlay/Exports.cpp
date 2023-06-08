#include <Include.hpp>

std::map<uint32_t, std::string> info_vars{};

BOOLEAN exports::InfoVarsInit()
{
	info_vars[Shared::I::ic_storage_directory] = *G::StorageHpr.path();
	info_vars[Shared::I::ic_config_main_path] = SHPR_GET_FILE_PATH("main.conf");

	return TRUE;
}

BOOLEAN exports::RegisterHandler(Games::C_GameBase* pHandler)
{
	if (pHandler == nullptr || G::P::Core == nullptr)
		return FALSE;

	G::P::Core->AddHandler(pHandler);

	return TRUE;
}

PFLOAT exports::GetCurrentTimePointer()
{
	return &G::CurrentTime;
}

LPCSTR exports::GetCurrentUserName()
{
	return G::CurrentUserName.c_str();
}

INT exports::GetBackendLatency()
{
	return G::BackendLatency;
}

BOOLEAN exports::QueryInfoVar(UINT32 dwId, PCHAR pszBuffer, PSIZE_T uLength)
{
	if (pszBuffer == nullptr || uLength == nullptr || 
		info_vars.find(dwId) == info_vars.end())
		return FALSE;

	const auto& var = info_vars[dwId];

	if (*uLength <= NULL || *uLength < var.size())
	{
		*uLength = var.size();
		return FALSE;
	}

	memcpy(pszBuffer, var.data(), *uLength);

	return TRUE;
}
