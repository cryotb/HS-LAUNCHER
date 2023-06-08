#include <Include.hpp>

#define DECLARE_EXPORT(X) G::ExportTable[_XSS(#X)] = &X

std::map<std::string, void*> G::ExportTable;

void G::ExportInit()
{
	DECLARE_EXPORT(exports::RegisterHandler);
	DECLARE_EXPORT(exports::GetCurrentTimePointer);
	DECLARE_EXPORT(exports::GetCurrentUserName);
	DECLARE_EXPORT(exports::GetBackendLatency);
	DECLARE_EXPORT(exports::QueryInfoVar);
}
