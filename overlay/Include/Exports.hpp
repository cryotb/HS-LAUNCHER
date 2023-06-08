#pragma once

namespace exports
{
	EXTERN BOOLEAN InfoVarsInit();
	EXTERN BOOLEAN RegisterHandler(Games::C_GameBase* pHandler);
	EXTERN PFLOAT GetCurrentTimePointer();

	EXTERN BOOLEAN IsGdmLoadBlocked();
	EXTERN LPCSTR GetCurrentUserName();
	EXTERN INT GetBackendLatency();

	EXTERN BOOLEAN QueryInfoVar(UINT32 dwId, PCHAR pszBuffer, PSIZE_T uLength);
}
