#pragma once

#pragma warning(disable : 4267)

#define SHARED_USER_MODE
#include <Shared/Include.hpp>

#include <random>
#include <iostream>
#include <fstream>
#include <optional>
#include <filesystem>

#define FMT_HEADER_ONLY

#if  !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <winternl.h>
#include <winioctl.h>
#include <winevt.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <ntddndis.h>
#include <iphlpapi.h>
#include <shellapi.h>

#include <fmt/format.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "defs.hpp"
#include "global.hpp"

#include "mapper.hpp"
#include "util.hpp"
#include "hellrunner.hpp"
