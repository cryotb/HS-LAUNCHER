#pragma once

#define SHARED_USER_MODE

#pragma warning(disable : 4302)

#include <Shared/Include.hpp>
#include <dwmapi.h>
#include <random>
#include <deque>
#include <shellapi.h>

#include <ntddndis.h>
#include <iphlpapi.h>


#include <nlohmann/json.hpp>
#include <Zydis/Zydis.h>

#include <hxldr/inc.hpp>

#include <VMProtectSDK.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Static_User.lib")
#pragma comment(lib, "hxldr.lib")
#pragma comment (lib, "iphlpapi.lib") 
#pragma comment(lib, "VMProtectSDK64.lib")


#include "Defines.hpp"
#include "VMProtect.hpp"
#include "WinAPI.hpp"
#include "Global.hpp"
#include "Device.hpp"
#include "Nt.hpp"
#include "DnsAPI.hpp"
#include "Hwid.hpp"
#include "Callbacks.hpp"
#include "Core.hpp"
#include "Util.hpp"
#include "Mapper.hpp"
#include "Input.hpp"
#include "Feature.hpp"
#include "WindWalker.hpp"
#include "GlobalPost.hpp"
#include "UtilPost.hpp"
#include "Valve.hpp"
#include "Exports.hpp"
#include "UI/UI.hpp"
#include "Loader.hpp"
#include "Loader_UI.hpp"
#include "Loader_LG.hpp"
#include "Networking.hpp"
#include "Helloverlay.hpp"
#include "Integrity.hpp"
#include "Hooks.hpp"
