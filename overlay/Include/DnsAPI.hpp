#pragma once

namespace dns
{
    template < typename T >
    T FindExport(const std::string& name)
    {
        auto hModule = LoadLibrary(_XS("dnsapi.dll"));

        if (hModule == nullptr)
            return FALSE;

        auto hProcedure = GetProcAddress(hModule, name.c_str());

        return reinterpret_cast<T>(hProcedure);
    }

    //
    // Removes a single cache entry by the specified domain name.
    //
    INLINE BOOL FlushResolverCacheEntry(const std::string& domain)
    {
        using fn = BOOL(WINAPI*)(LPCSTR);

        auto* const pfnFunction = FindExport<fn>(_XS("DnsFlushResolverCacheEntry_A"));

        if (pfnFunction == nullptr)
            return FALSE;

        return pfnFunction(domain.c_str());
    }
}
